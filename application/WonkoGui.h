#pragma once

#include <memory>

class QWidget;
class QString;

using WonkoIndexPtr = std::shared_ptr<class WonkoIndex>;
using WonkoVersionListPtr = std::shared_ptr<class WonkoVersionList>;
using WonkoVersionPtr = std::shared_ptr<class WonkoVersion>;

namespace Wonko
{
enum UpdateType
{
	AlwaysUpdate,
	UpdateIfNeeded
};

/// Ensures that the index has been loaded, either from the local cache or remotely
WonkoIndexPtr ensureIndexLoaded(QWidget *parent);
/// Ensures that the given uid exists. Returns a nullptr if it doesn't.
WonkoVersionListPtr ensureVersionListExists(const QString &uid, QWidget *parent);
/// Ensures that the given uid exists and is loaded, either from the local cache or remotely. Returns nullptr if it doesn't exist or couldn't be loaded.
WonkoVersionListPtr ensureVersionListLoaded(const QString &uid, QWidget *parent);
WonkoVersionPtr ensureVersionExists(const QString &uid, const QString &version, QWidget *parent);
WonkoVersionPtr ensureVersionLoaded(const QString &uid, const QString &version, QWidget *parent, const UpdateType update = UpdateIfNeeded);
}
