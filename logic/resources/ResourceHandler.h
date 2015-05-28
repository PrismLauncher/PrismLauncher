#pragma once

#include <QVariant>
#include <memory>

class Resource;

/** Base class for things that can retrieve a resource.
 *
 * Subclass, provide a constructor that takes a single QString as argument, and
 * call Resource::registerHandler<MyResourceHandler>("<id>"), where <id> is the
 * prefix of the resource ("web", "icon", etc.)
 */
class ResourceHandler
{
public:
	virtual ~ResourceHandler() {}

	void setResource(Resource *resource) { m_resource = resource; }
	// reimplement this if you need to do something after you have been put in a shared pointer
	virtual void init(std::shared_ptr<ResourceHandler>&) {}

	QVariant result() const { return m_result; }

protected: // use these methods to notify the resource of changes
	void setResult(const QVariant &result);
	void setFailure(const QString &reason);
	void setProgress(const int progress);

private:
	QVariant m_result;
	Resource *m_resource = nullptr;
};
