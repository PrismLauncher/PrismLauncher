#pragma once

#include <QString>

#include "Library.h"

struct Agent {
    /// The library pointing to the jar this Java agent is contained within
    LibraryPtr library;

    /// The argument to the Java agent, passed after an = if present
    QString argument;
};
