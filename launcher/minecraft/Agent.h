#pragma once

#include <QString>

#include "Library.h"

class Agent;

typedef std::shared_ptr<Agent> AgentPtr;

class Agent {
public:
    Agent(LibraryPtr library, const QString &argument)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_library = library;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_argument = argument;
    }

public: /* methods */

    LibraryPtr library() {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_library;
    }
    QString argument() {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_argument;
    }

protected: /* data */

    /// The library pointing to the jar this Java agent is contained within
    LibraryPtr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_library;

    /// The argument to the Java agent, passed after an = if present
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_argument;

};
