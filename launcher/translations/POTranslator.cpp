#include "POTranslator.h"

#include <QDebug>
#include "FileSystem.h"

struct POEntry
{
    QString text;
    bool fuzzy;
};

struct POTranslatorPrivate
{
    QString filename;
    QHash<QByteArray, POEntry> mapping;
    QHash<QByteArray, POEntry> mapping_disambiguatrion;
    bool loaded = false;

    void reload();
};

class ParserArray : public QByteArray
{
public:
    ParserArray(const QByteArray &in) : QByteArray(in)
    {
    }
    bool chomp(const char * data, int length)
    {
        if(startsWith(data))
        {
            remove(0, length);
            return true;
        }
        return false;
    }
    bool chompString(QByteArray & appendHere)
    {
        QByteArray msg;
        bool escape = false;
        if(size() < 2)
        {
            qDebug() << "String fragment is too short";
            return false;
        }
        if(!startsWith('"'))
        {
            qDebug() << "String fragment does not start with \"";
            return false;
        }
        if(!endsWith('"'))
        {
            qDebug() << "String fragment does not end with \", instead, there is" << at(size() - 1);
            return false;
        }
        for(int i = 1; i < size() - 1; i++)
        {
            char c = operator[](i);
            if(escape)
            {
                switch(c)
                {
                    case 'r':
                        msg += '\r';
                        break;
                    case 'n':
                        msg += '\n';
                        break;
                    case 't':
                        msg += '\t';
                        break;
                    case 'v':
                        msg += '\v';
                        break;
                    case 'a':
                        msg += '\a';
                        break;
                    case 'b':
                        msg += '\b';
                        break;
                    case 'f':
                        msg += '\f';
                        break;
                    case '"':
                        msg += '"';
                        break;
                    case '\\':
                        msg.append('\\');
                        break;
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    {
                        int octal_start = i;
                        while ((c = operator[](i)) >= '0' && c <= '7')
                        {
                            i++;
                            if (i == length() - 1)
                            {
                                qDebug() << "Something went bad while parsing an octal escape string...";
                                return false;
                            }
                        }
                        msg += mid(octal_start, i - octal_start).toUInt(0, 8);
                        break;
                    }
                    case 'x':
                    {
                        // chomp the 'x'
                        i++;
                        int hex_start = i;
                        while (isxdigit(operator[](i)))
                        {
                            i++;
                            if (i == length() - 1)
                            {
                                qDebug() << "Something went bad while parsing a hex escape string...";
                                return false;
                            }
                        }
                        msg += mid(hex_start, i - hex_start).toUInt(0, 16);
                        break;
                    }
                    default:
                    {
                        qDebug() << "Invalid escape sequence character:" << c;
                        return false;
                    }
                }
                escape = false;
            }
            else if(c == '\\')
            {
                escape = true;
            }
            else
            {
                msg += c;
            }
        }
        if(escape)
        {
            qDebug() << "Unterminated escape sequence...";
            return false;
        }
        appendHere += msg;
        return true;
    }
};

void POTranslatorPrivate::reload()
{
    QFile file(filename);
    if(!file.open(QFile::OpenMode::enum_type::ReadOnly | QFile::OpenMode::enum_type::Text))
    {
        qDebug() << "Failed to open PO file:" << filename;
        return;
    }

    QByteArray context;
    QByteArray disambiguation;
    QByteArray id;
    QByteArray str;
    bool fuzzy = false;
    bool nextFuzzy = false;

    enum class Mode
    {
        First,
        MessageContext,
        MessageId,
        MessageString
    } mode = Mode::First;

    int lineNumber = 0;
    QHash<QByteArray, POEntry> newMapping;
    QHash<QByteArray, POEntry> newMapping_disambiguation;
    auto endEntry = [&]() {
        auto strStr = QString::fromUtf8(str);
        // NOTE: PO header has empty id. We skip it.
        if(!id.isEmpty())
        {
            auto normalKey = context + "|" + id;
            newMapping.insert(normalKey, {strStr, fuzzy});
            if(!disambiguation.isEmpty())
            {
                auto disambiguationKey = context + "|" + id + "@" + disambiguation;
                newMapping_disambiguation.insert(disambiguationKey, {strStr, fuzzy});
            }
        }
        context.clear();
        disambiguation.clear();
        id.clear();
        str.clear();
        fuzzy = nextFuzzy;
        nextFuzzy = false;
    };
    while (!file.atEnd())
    {
        ParserArray line = file.readLine();
        if(line.endsWith('\n'))
        {
            line.resize(line.size() - 1);
        }
        if(line.endsWith('\r'))
        {
            line.resize(line.size() - 1);
        }

        if(!line.size())
        {
            // NIL
        }
        else if(line[0] == '#')
        {
            if(line.contains(", fuzzy"))
            {
                nextFuzzy = true;
            }
        }
        else if(line.startsWith('"'))
        {
            QByteArray temp;
            QByteArray *out = &temp;

            switch(mode)
            {
                case Mode::First:
                    qDebug() << "Unexpected escaped string during initial state... line:" << lineNumber;
                    return;
                case Mode::MessageString:
                    out = &str;
                    break;
                case Mode::MessageContext:
                    out = &context;
                    break;
                case Mode::MessageId:
                    out = &id;
                    break;
            }
            if(!line.chompString(*out))
            {
                qDebug() << "Badly formatted string on line:" << lineNumber;
                return;
            }
        }
        else if(line.chomp("msgctxt ", 8))
        {
            switch(mode)
            {
                case Mode::First:
                    break;
                case Mode::MessageString:
                    endEntry();
                    break;
                case Mode::MessageContext:
                case Mode::MessageId:
                    qDebug() << "Unexpected msgctxt line:" << lineNumber;
                    return;
            }
            if(line.chompString(context))
            {
                auto parts = context.split('|');
                context = parts[0];
                if(parts.size() > 1 && !parts[1].isEmpty())
                {
                    disambiguation = parts[1];
                }
                mode = Mode::MessageContext;
            }
        }
        else if (line.chomp("msgid ", 6))
        {
            switch(mode)
            {
                case Mode::MessageContext:
                case Mode::First:
                    break;
                case Mode::MessageString:
                    endEntry();
                    break;
                case Mode::MessageId:
                    qDebug() << "Unexpected msgid line:" << lineNumber;
                    return;
            }
            if(line.chompString(id))
            {
                mode = Mode::MessageId;
            }
        }
        else if (line.chomp("msgstr ", 7))
        {
            switch(mode)
            {
                case Mode::First:
                case Mode::MessageString:
                case Mode::MessageContext:
                    qDebug() << "Unexpected msgstr line:" << lineNumber;
                    return;
                case Mode::MessageId:
                    break;
            }
            if(line.chompString(str))
            {
                mode = Mode::MessageString;
            }
        }
        else
        {
            qDebug() << "I did not understand line: " << lineNumber << ":" << QString::fromUtf8(line);
        }
        lineNumber++;
    }
    endEntry();
    mapping = std::move(newMapping);
    mapping_disambiguatrion = std::move(newMapping_disambiguation);
    loaded = true;
}

POTranslator::POTranslator(const QString& filename, QObject* parent) : QTranslator(parent)
{
    d = new POTranslatorPrivate;
    d->filename = filename;
    d->reload();
}

POTranslator::~POTranslator()
{
    delete d;
}

QString POTranslator::translate(const char* context, const char* sourceText, const char* disambiguation, int n) const
{
    if(disambiguation)
    {
        auto disambiguationKey = QByteArray(context) + "|" + QByteArray(sourceText) + "@" + QByteArray(disambiguation);
        auto iter = d->mapping_disambiguatrion.find(disambiguationKey);
        if(iter != d->mapping_disambiguatrion.end())
        {
            auto & entry = *iter;
            if(entry.text.isEmpty())
            {
                qDebug() << "Translation entry has no content:" << disambiguationKey;
            }
            if(entry.fuzzy)
            {
                qDebug() << "Translation entry is fuzzy:" << disambiguationKey << "->" << entry.text;
            }
            return entry.text;
        }
    }
    auto key = QByteArray(context) + "|" + QByteArray(sourceText);
    auto iter = d->mapping.find(key);
    if(iter != d->mapping.end())
    {
        auto & entry = *iter;
        if(entry.text.isEmpty())
        {
            qDebug() << "Translation entry has no content:" << key;
        }
        if(entry.fuzzy)
        {
            qDebug() << "Translation entry is fuzzy:" << key << "->" << entry.text;
        }
        return entry.text;
    }
    return QString();
}

bool POTranslator::isEmpty() const
{
    return !d->loaded;
}
