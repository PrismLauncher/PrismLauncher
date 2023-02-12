#ifndef QDCSS_H
#define QDCSS_H

#include <QMap>
#include <QString>
#include <QStringList>
#include <optional>

class QDCSS {
    // these are all we need to parse a couple string values out of a css string
    // lots more in the original code, yet to be ported
    // https://github.com/unascribed/NilLoader/blob/trunk/src/main/java/nilloader/api/lib/qdcss/QDCSS.java
   public:
    QDCSS(QString);
    std::optional<QString>* get(QString);

   private:
    QMap<QString, QStringList> m_data;
};

#endif  // QDCSS_H
