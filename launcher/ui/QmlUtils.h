#pragma once

#include <QtGlobal>

/** The return value of 'expr' is checked in debug mode, giving an assertion error if it fails. It does nothing otherwise. */
#ifndef QT_NO_DEBUG
#define MUST_SUCCEED(EXPR) Q_ASSERT(EXPR)
#else
#define MUST_SUCCEED(EXPR) Q_UNUSED(EXPR)
#endif
