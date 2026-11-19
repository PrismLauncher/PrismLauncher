#include <QScopeGuard>

#define DEFER_JOIN_(a, b) a##b
#define DEFER_VARNAME_(name) DEFER_JOIN_(scope_guard_, name)
#define DEFER(f) auto DEFER_VARNAME_(__COUNTER__) = qScopeGuard(f)