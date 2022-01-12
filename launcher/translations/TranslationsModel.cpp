#include "TranslationsModel.h"

#include <QCoreApplication>
#include <QTranslator>
#include <QLocale>
#include <QDir>
#include <QLibraryInfo>
#include <QDebug>

#include "FileSystem.h"
#include "net/NetJob.h"
#include "net/ChecksumValidator.h"
#include "BuildConfig.h"
#include "Json.h"

#include "POTranslator.h"

#include "Application.h"

const static QLatin1Literal defaultLangCode("en_US");

enum class FileType
{
    NONE,
    QM,
    PO
};

struct Language
{
    Language()
    {
        updated = true;
    }
    Language(const QString & _key)
    {
        key = _key;
        locale = QLocale(key);
        updated = (key == defaultLangCode);
    }

    QString languageName() const {
        QString result;
        if(key == "ja_KANJI") {
            result = locale.nativeLanguageName() + u8" (漢字)";
        }
        else if(key == "es_UY") {
            result = u8"español de Latinoamérica";
        }
        else {
            result = locale.nativeLanguageName();
        }
        return result;
    }

    float percentTranslated() const
    {
        if (total == 0)
        {
            return 100.0f;
        }
        return 100.0f * float(translated) / float(total);
    }

    void setTranslationStats(unsigned _translated, unsigned _untranslated, unsigned _fuzzy)
    {
        translated = _translated;
        untranslated = _untranslated;
        fuzzy = _fuzzy;
        total = translated + untranslated + fuzzy;
    }

    bool isOfSameNameAs(const Language& other) const
    {
        return key == other.key;
    }

    bool isIdenticalTo(const Language& other) const
    {
        return
        (
            key == other.key &&
            file_name == other.file_name &&
            file_size == other.file_size &&
            file_sha1 == other.file_sha1 &&
            translated == other.translated &&
            fuzzy == other.fuzzy &&
            total == other.fuzzy &&
            localFileType == other.localFileType
        );
    }

    Language & apply(Language & other)
    {
        if(!isOfSameNameAs(other))
        {
            return *this;
        }
        file_name = other.file_name;
        file_size = other.file_size;
        file_sha1 = other.file_sha1;
        translated = other.translated;
        fuzzy = other.fuzzy;
        total = other.total;
        localFileType = other.localFileType;
        return *this;
    }

    QString key;
    QLocale locale;
    bool updated;

    QString file_name = QString();
    std::size_t file_size = 0;
    QString file_sha1 = QString();

    unsigned translated = 0;
    unsigned untranslated = 0;
    unsigned fuzzy = 0;
    unsigned total = 0;

    FileType localFileType = FileType::NONE;
};



struct TranslationsModel::Private
{
    QDir m_dir;

    // initial state is just english
    QVector<Language> m_languages = {Language (defaultLangCode)};

    QString m_selectedLanguage = defaultLangCode;
    std::unique_ptr<QTranslator> m_qt_translator;
    std::unique_ptr<QTranslator> m_app_translator;

    Net::Download::Ptr m_index_task;
    QString m_downloadingTranslation;
    NetJob::Ptr m_dl_job;
    NetJob::Ptr m_index_job;
    QString m_nextDownload;

    std::unique_ptr<POTranslator> m_po_translator;
    QFileSystemWatcher *watcher;

    bool no_language_set = false;
};

TranslationsModel::TranslationsModel(QString path, QObject* parent): QAbstractListModel(parent)
{
    d.reset(new Private);
    d->m_dir.setPath(path);
    FS::ensureFolderPathExists(path);
    reloadLocalFiles();

    d->watcher = new QFileSystemWatcher(this);
    connect(d->watcher, &QFileSystemWatcher::directoryChanged, this, &TranslationsModel::translationDirChanged);
    d->watcher->addPath(d->m_dir.canonicalPath());
}

TranslationsModel::~TranslationsModel()
{
}

void TranslationsModel::translationDirChanged(const QString& path)
{
    qDebug() << "Dir changed:" << path;
    reloadLocalFiles();

    if (d->no_language_set)
    {
        auto bcp47Name = QLocale::system().name();
        if (!findLanguage(bcp47Name))
        {
            bcp47Name = bcp47Name.split('_').front();
        }
        selectLanguage(bcp47Name);
        if (selectedLanguage() != defaultLangCode)
        {
            updateLanguage(selectedLanguage());
            APPLICATION->settings()->set("Language", selectedLanguage());
        }
        d->no_language_set = false;
    }
    else
    {
        selectLanguage(selectedLanguage());
    }
}

void TranslationsModel::indexReceived()
{
    qDebug() << "Got translations index!";
    d->m_index_job.reset();
    if(d->m_selectedLanguage != defaultLangCode)
    {
        downloadTranslation(d->m_selectedLanguage);
    }
}

namespace {
void readIndex(const QString & path, QMap<QString, Language>& languages)
{
    QByteArray data;
    try
    {
        data = FS::read(path);
    }
    catch (const Exception &e)
    {
        qCritical() << "Translations Download Failed: index file not readable";
        return;
    }

    int index = 1;
    try
    {
        auto toplevel_doc = Json::requireDocument(data);
        auto doc = Json::requireObject(toplevel_doc);
        auto file_type = Json::requireString(doc, "file_type");
        if(file_type != "MMC-TRANSLATION-INDEX")
        {
            qCritical() << "Translations Download Failed: index file is of unknown file type" << file_type;
            return;
        }
        auto version = Json::requireInteger(doc, "version");
        if(version > 2)
        {
            qCritical() << "Translations Download Failed: index file is of unknown format version" << file_type;
            return;
        }
        auto langObjs = Json::requireObject(doc, "languages");
        for(auto iter = langObjs.begin(); iter != langObjs.end(); iter++)
        {
            Language lang(iter.key());

            auto langObj =  Json::requireObject(iter.value());
            lang.setTranslationStats(
                Json::ensureInteger(langObj, "translated", 0),
                Json::ensureInteger(langObj, "untranslated", 0),
                Json::ensureInteger(langObj, "fuzzy", 0)
            );
            lang.file_name = Json::requireString(langObj, "file");
            lang.file_sha1 = Json::requireString(langObj, "sha1");
            lang.file_size = Json::requireInteger(langObj, "size");

            languages.insert(lang.key, lang);
            index++;
        }
    }
    catch (Json::JsonException & e)
    {
        qCritical() << "Translations Download Failed: index file could not be parsed as json";
    }
}
}

void TranslationsModel::reloadLocalFiles()
{
    QMap<QString, Language> languages = {{defaultLangCode, Language(defaultLangCode)}};

    readIndex(d->m_dir.absoluteFilePath("index_v2.json"), languages);
    auto entries = d->m_dir.entryInfoList({"mmc_*.qm", "*.po"}, QDir::Files | QDir::NoDotAndDotDot);
    for(auto & entry: entries)
    {
        auto completeSuffix = entry.completeSuffix();
        QString langCode;
        FileType fileType = FileType::NONE;
        if(completeSuffix == "qm")
        {
            langCode = entry.baseName().remove(0,4);
            fileType = FileType::QM;
        }
        else if(completeSuffix == "po")
        {
            langCode = entry.baseName();
            fileType = FileType::PO;
        }
        else
        {
            continue;
        }

        auto langIter = languages.find(langCode);
        if(langIter != languages.end())
        {
            auto & language = *langIter;
            if(int(fileType) > int(language.localFileType))
            {
                language.localFileType = fileType;
            }
        }
        else
        {
            if(fileType == FileType::PO)
            {
                Language localFound(langCode);
                localFound.localFileType = FileType::PO;
                languages.insert(langCode, localFound);
            }
        }
    }

    // changed and removed languages
    for(auto iter = d->m_languages.begin(); iter != d->m_languages.end();)
    {
        auto &language = *iter;
        auto row = iter - d->m_languages.begin();

        auto updatedLanguageIter = languages.find(language.key);
        if(updatedLanguageIter != languages.end())
        {
            if(language.isIdenticalTo(*updatedLanguageIter))
            {
                languages.remove(language.key);
            }
            else
            {
                language.apply(*updatedLanguageIter);
                emit dataChanged(index(row), index(row));
                languages.remove(language.key);
            }
            iter++;
        }
        else
        {
            beginRemoveRows(QModelIndex(), row, row);
            iter = d->m_languages.erase(iter);
            endRemoveRows();
        }
    }
    // added languages
    if(languages.isEmpty())
    {
        return;
    }
    beginInsertRows(QModelIndex(), 0, d->m_languages.size() + languages.size() - 1);
    for(auto & language: languages)
    {
        d->m_languages.append(language);
    }
    std::sort(d->m_languages.begin(), d->m_languages.end(), [](const Language& a, const Language& b) {
        return a.key.compare(b.key) < 0;
    });
    endInsertRows();
}

namespace {
enum class Column
{
    Language,
    Completeness
};
}


QVariant TranslationsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int row = index.row();
    auto column = static_cast<Column>(index.column());

    if (row < 0 || row >= d->m_languages.size())
        return QVariant();

    auto & lang = d->m_languages[row];
    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch(column)
        {
            case Column::Language:
            {
                return lang.languageName();
            }
            case Column::Completeness:
            {
                QString text;
                text.sprintf("%3.1f %%", lang.percentTranslated());
                return text;
            }
        }
    }
    case Qt::ToolTipRole:
    {
        return tr("%1:\n%2 translated\n%3 fuzzy\n%4 total").arg(lang.key, QString::number(lang.translated), QString::number(lang.fuzzy), QString::number(lang.total));
    }
    case Qt::UserRole:
        return lang.key;
    default:
        return QVariant();
    }
}

QVariant TranslationsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    auto column = static_cast<Column>(section);
    if(role == Qt::DisplayRole)
    {
        switch(column)
        {
            case Column::Language:
            {
                return tr("Language");
            }
            case Column::Completeness:
            {
                return tr("Completeness");
            }
        }
    }
    else if(role == Qt::ToolTipRole)
    {
        switch(column)
        {
            case Column::Language:
            {
                return tr("The native language name.");
            }
            case Column::Completeness:
            {
                return tr("Completeness is the percentage of fully translated strings, not counting automatically guessed ones.");
            }
        }
    }
    return QAbstractListModel::headerData(section, orientation, role);
}

int TranslationsModel::rowCount(const QModelIndex& parent) const
{
    return d->m_languages.size();
}

int TranslationsModel::columnCount(const QModelIndex& parent) const
{
    return 2;
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

    if (langCode.length() == 0)
    {
        d->no_language_set = true;
    }

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
    QLocale locale = QLocale(langCode);
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

    if(langPtr->localFileType == FileType::PO)
    {
        qDebug() << "Loading Application Language File for" << langCode.toLocal8Bit().constData() << "...";
        auto poTranslator = new POTranslator(FS::PathCombine(d->m_dir.path(), langCode + ".po"));
        if(!poTranslator->isEmpty())
        {
            if (!QCoreApplication::installTranslator(poTranslator))
            {
                delete poTranslator;
                qCritical() << "Installing Application Language File failed.";
            }
            else
            {
                d->m_app_translator.reset(poTranslator);
                successful = true;
            }
        }
        else
        {
            qCritical() << "Loading Application Language File failed.";
            d->m_app_translator.reset();
        }
    }
    else if(langPtr->localFileType == FileType::QM)
    {
        d->m_app_translator.reset(new QTranslator());
        if (d->m_app_translator->load("mmc_" + langCode, d->m_dir.path()))
        {
            qDebug() << "Loading Application Language File for" << langCode.toLocal8Bit().constData() << "...";
            if (!QCoreApplication::installTranslator(d->m_app_translator.get()))
            {
                qCritical() << "Installing Application Language File failed.";
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
    d->m_index_job = new NetJob("Translations Index", APPLICATION->network());
    MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry("translations", "index_v2.json");
    entry->setStale(true);
    d->m_index_task = Net::Download::makeCached(QUrl("https://files.multimc.org/translations/index_v2.json"), entry);
    d->m_index_job->addNetAction(d->m_index_task);
    connect(d->m_index_job.get(), &NetJob::failed, this, &TranslationsModel::indexFailed);
    connect(d->m_index_job.get(), &NetJob::succeeded, this, &TranslationsModel::indexReceived);
    d->m_index_job->start();
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
    auto lang = findLanguage(key);
    if(!lang)
    {
        qWarning() << "Will not download an unknown translation" << key;
        return;
    }

    d->m_downloadingTranslation = key;
    MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry("translations", "mmc_" + key + ".qm");
    entry->setStale(true);

    auto dl = Net::Download::makeCached(QUrl(BuildConfig.TRANSLATIONS_BASE_URL + lang->file_name), entry);
    auto rawHash = QByteArray::fromHex(lang->file_sha1.toLatin1());
    dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, rawHash));
    dl->m_total_progress = lang->file_size;

    d->m_dl_job = new NetJob("Translation for " + key, APPLICATION->network());
    d->m_dl_job->addNetAction(dl);

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
