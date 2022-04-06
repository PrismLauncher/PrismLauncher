#pragma once

#include <QString>

#include "Library.h"

class Agent;

typedef std::shared_ptr<Agent> AgentPtr;

class Agent {
public:
    Agent(LibraryPtr library, QString &argument)
    {
        m_library = library;
        m_argument = argument;
    }

public: /* methods */

    LibraryPtr library() {
        return m_library;
    }
    QString argument() {
        return m_argument;
    }

protected: /* data */

    /// The library pointing to the jar this Java agent is contained within
    LibraryPtr m_library;

    /// The argument to the Java agent, passed after an = if present
    QString m_argument;

};
