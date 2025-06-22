#pragma once

#include <QString>

// NOTE: apparently the GNU C library pollutes the global namespace with these... undef them.
#ifdef major
#undef major
#endif
#ifdef minor
#undef minor
#endif

class JavaVersion {
    friend class JavaVersionTest;

   public:
    JavaVersion() {}
    JavaVersion(const QString& rhs);
    JavaVersion(int major, int minor, int security, int build = 0, QString name = "");

    JavaVersion& operator=(const QString& rhs);

    bool operator<(const JavaVersion& rhs);
    bool operator==(const JavaVersion& rhs);
    bool operator>(const JavaVersion& rhs);

    bool requiresPermGen() const;
    bool defaultsToUtf8() const;
    bool isModular() const;

    QString toString() const;

    int major() const { return m_major; }
    int minor() const { return m_minor; }
    int security() const { return m_security; }
    QString build() const { return m_prerelease; }
    QString name() const { return m_name; }

   private:
    QString m_string;
    int m_major = 0;
    int m_minor = 0;
    int m_security = 0;
    QString m_name = "";
    bool m_parseable = false;
    QString m_prerelease;
};
