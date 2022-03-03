#pragma once

#include "modplatform/ModAPI.h"

class ModrinthAPI : public ModAPI {
   public:
    inline QString getModSearchURL(int offset, QString query, QString sort, bool fabricCompatible, QString version) const override 
    { 
        return QString("https://api.modrinth.com/v2/search?"
                "offset=%1&"   "limit=25&"   "query=%2&"   "index=%3&"
                "facets=[[\"categories:%4\"],[\"versions:%5\"],[\"project_type:mod\"]]")
                  .arg(offset)
                  .arg(query)
                  .arg(sort)
                  .arg(fabricCompatible ? "fabric" : "forge")
                  .arg(version);
    };

    inline QString getVersionsURL(const QString& addonId) const override
    {
        return QString("https://api.modrinth.com/v2/project/%1/version").arg(addonId);
    };

    inline QString getAuthorURL(const QString& name) const override { return "https://modrinth.com/user/" + name; };
};
