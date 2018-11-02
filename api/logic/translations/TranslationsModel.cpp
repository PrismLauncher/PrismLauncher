#include "TranslationsModel.h"

#include <QCoreApplication>
#include <QTranslator>
#include <QLocale>
#include <QDir>
#include <QLibraryInfo>
#include <QDebug>
#include <FileSystem.h>
#include <net/NetJob.h>
#include <Env.h>
#include <net/URLConstants.h>

const static QLatin1Literal defaultLangCode("en");

struct Language
{
    QString key;
    QLocale locale;
    bool updated;
};

struct TranslationsModel::Private
{
    QDir m_dir;

    // initial state is just english
    QVector<Language> m_languages = {{defaultLangCode, QLocale(defaultLangCode), false}};
    QString m_selectedLanguage = defaultLangCode;
    std::unique_ptr<QTranslator> m_qt_translator;
    std::unique_ptr<QTranslator> m_app_translator;

    std::shared_ptr<Net::Download> m_index_task;
    QString m_downloadingTranslation;
    NetJobPtr m_dl_job;
    NetJobPtr m_index_job;
    QString m_nextDownload;
};

TranslationsModel::TranslationsModel(QString path, QObject* parent): QAbstractListModel(parent)
{
    d.reset(new Private);
    d->m_dir.setPath(path);
    loadLocalIndex();
}

TranslationsModel::~TranslationsModel()
{
}

QVariant TranslationsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int row = index.row();

    if (row < 0 || row >= d->m_languages.size())
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
        return d->m_languages[row].locale.nativeLanguageName();
    case Qt::UserRole:
        return d->m_languages[row].key;
    default:
        return QVariant();
    }
}

int TranslationsModel::rowCount(const QModelIndex& parent) const
{
    return d->m_languages.size();
}

Language * TranslationsModel::findLanguage(const QString& key)
{
    auto found = std::find_if(d->m_languages.begin(), d->m_languages.end(), [&](Language & lang)
    {
        return lang.key == key;
    });
    if(found == d->m_languages.end())
    {
        return nullptr;
    }
    else
    {
        return found;
    }
}

bool TranslationsModel::selectLanguage(QString key)
{
    QString &langCode = key;
    auto langPtr = findLanguage(key);
    if(!langPtr)
    {
        qWarning() << "Selected invalid language" << key << ", defaulting to" << defaultLangCode;
        langCode = defaultLangCode;
    }
    else
    {
        langCode = langPtr->key;
    }

    // uninstall existing translators if there are any
    if (d->m_app_translator)
    {
        QCoreApplication::removeTranslator(d->m_app_translator.get());
        d->m_app_translator.reset();
    }
    if (d->m_qt_translator)
    {
        QCoreApplication::removeTranslator(d->m_qt_translator.get());
        d->m_qt_translator.reset();
    }

    /*
     * FIXME: potential source of crashes:
     * In a multithreaded application, the default locale should be set at application startup, before any non-GUI threads are created.
     * This function is not reentrant.
     */
    QLocale locale(langCode);
    QLocale::setDefault(locale);

    // if it's the default UI language, finish
    if(langCode == defaultLangCode)
    {
        d->m_selectedLanguage = langCode;
        return true;
    }

    // otherwise install new translations
    bool successful = false;
    // FIXME: this is likely never present. FIX IT.
    d->m_qt_translator.reset(new QTranslator());
    if (d->m_qt_translator->load("qt_" + langCode, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
    {
        qDebug() << "Loading Qt Language File for" << langCode.toLocal8Bit().constData() << "...";
        if (!QCoreApplication::installTranslator(d->m_qt_translator.get()))
        {
            qCritical() << "Loading Qt Language File failed.";
            d->m_qt_translator.reset();
        }
        else
        {
            successful = true;
        }
    }
    else
    {
        d->m_qt_translator.reset();
    }

    d->m_app_translator.reset(new QTranslator());
    if (d->m_app_translator->load("mmc_" + langCode, d->m_dir.path()))
    {
        qDebug() << "Loading Application Language File for" << langCode.toLocal8Bit().constData() << "...";
        if (!QCoreApplication::installTranslator(d->m_app_translator.get()))
        {
            qCritical() << "Loading Application Language File failed.";
            d->m_app_translator.reset();
        }
        else
        {
            successful = true;
        }
    }
    else
    {
        d->m_app_translator.reset();
    }
    d->m_selectedLanguage = langCode;
    return successful;
}

QModelIndex TranslationsModel::selectedIndex()
{
    auto found = findLanguage(d->m_selectedLanguage);
    if(found)
    {
        // QVector iterator freely converts to pointer to contained type
        return index(found - d->m_languages.begin(), 0, QModelIndex());
    }
    return QModelIndex();
}

QString TranslationsModel::selectedLanguage()
{
    return d->m_selectedLanguage;
}

void TranslationsModel::downloadIndex()
{
    if(d->m_index_job || d->m_dl_job)
    {
        return;
    }
    qDebug() << "Downloading Translations Index...";
    d->m_index_job.reset(new NetJob("Translations Index"));
    MetaEntryPtr entry = ENV.metacache()->resolveEntry("translations", "index");
    d->m_index_task = Net::Download::makeCached(QUrl("https://files.multimc.org/translations/index"), entry);
    d->m_index_job->addNetAction(d->m_index_task);
    connect(d->m_index_job.get(), &NetJob::failed, this, &TranslationsModel::indexFailed);
    connect(d->m_index_job.get(), &NetJob::succeeded, this, &TranslationsModel::indexRecieved);
    d->m_index_job->start();
}

void TranslationsModel::indexRecieved()
{
    qDebug() << "Got translations index!";
    d->m_index_job.reset();
    loadLocalIndex();
    if(d->m_selectedLanguage != defaultLangCode)
    {
        downloadTranslation(d->m_selectedLanguage);
    }
}

void TranslationsModel::loadLocalIndex()
{
    QByteArray data;
    try
    {
        data = FS::read(d->m_dir.absoluteFilePath("index"));
    }
    catch (const Exception &e)
    {
        qCritical() << "Translations Download Failed: index file not readable";
        return;
    }
    QVector<Language> languages;
    QList<QByteArray> lines = data.split('\n');
    // add the default english.
    languages.append({defaultLangCode, QLocale(defaultLangCode), true});
    for (const auto line : lines)
    {
        if(!line.isEmpty())
        {
            auto str = QString::fromLatin1(line);
            str.remove(".qm");
            languages.append({str, QLocale(str), false});
        }
    }
    beginResetModel();
    d->m_languages.swap(languages);
    endResetModel();
}

void TranslationsModel::updateLanguage(QString key)
{
    if(key == defaultLangCode)
    {
        qWarning() << "Cannot update builtin language" << key;
        return;
    }
    auto found = findLanguage(key);
    if(!found)
    {
        qWarning() << "Cannot update invalid language" << key;
        return;
    }
    if(!found->updated)
    {
        downloadTranslation(key);
    }
}

void TranslationsModel::downloadTranslation(QString key)
{
    if(d->m_dl_job)
    {
        d->m_nextDownload = key;
        return;
    }
    d->m_downloadingTranslation = key;
    MetaEntryPtr entry = ENV.metacache()->resolveEntry("translations", "mmc_" + key + ".qm");
    entry->setStale(true);
    d->m_dl_job.reset(new NetJob("Translation for " + key));
    d->m_dl_job->addNetAction(Net::Download::makeCached(QUrl(URLConstants::TRANSLATIONS_BASE_URL + key + ".qm"), entry));
    connect(d->m_dl_job.get(), &NetJob::succeeded, this, &TranslationsModel::dlGood);
    connect(d->m_dl_job.get(), &NetJob::failed, this, &TranslationsModel::dlFailed);
    d->m_dl_job->start();
}

void TranslationsModel::downloadNext()
{
    if(!d->m_nextDownload.isEmpty())
    {
        downloadTranslation(d->m_nextDownload);
        d->m_nextDownload.clear();
    }
}

void TranslationsModel::dlFailed(QString reason)
{
    qCritical() << "Translations Download Failed:" << reason;
    d->m_dl_job.reset();
    downloadNext();
}

void TranslationsModel::dlGood()
{
    qDebug() << "Got translation:" << d->m_downloadingTranslation;

    if(d->m_downloadingTranslation == d->m_selectedLanguage)
    {
        selectLanguage(d->m_selectedLanguage);
    }
    d->m_dl_job.reset();
    downloadNext();
}

void TranslationsModel::indexFailed(QString reason)
{
    qCritical() << "Translations Index Download Failed:" << reason;
    d->m_index_job.reset();
}
