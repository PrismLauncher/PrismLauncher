#pragma once

/// where is a version from?
enum VersionSource
{
	Builtin, //!< version loaded from the internal resources.
	Local, //!< version loaded from a file in the cache.
	Remote, //!< incomplete version on a remote server.
};