#pragma once

#include <QString>
#include <QMap>
#include <QVariant>
#include <functional>
#include <memory>

#include "ResourceObserver.h"
#include "TypeMagic.h"

#include "multimc_logic_export.h"

class ResourceHandler;

/** Frontend class for resources
 *
 * Usage:
 *	Resource::create("icon:noaccount")->applyTo(accountsAction);
 *	Resource::create("web:http://asdf.com/image.png")->applyTo(imageLbl)->placeholder(Resource::create("icon:loading"));
 *
 * Memory management:
 *	Resource caches ResourcePtrs using weak pointers, so while a resource is still existing
 *	when a new resource is created the resources will be the same (including the same handler).
 *
 *	ResourceObservers keep a shared pointer to the resource, as does the Resource itself to it's
 *	placeholder (if present). This means a resource stays valid while it's still used ("applied to" etc.)
 *	by something. When nothing uses it anymore it gets deleted.
 *
 *	@note Always pass resource around using Resource::Ptr! Copy and move constructors are disabled for a reason.
 */
class MULTIMC_LOGIC_EXPORT Resource : public std::enable_shared_from_this<Resource>
{
	// only allow creation from Resource::create and disallow passing around non-pointers
	explicit Resource(const QString &resource);
	Resource(const Resource &) = delete;
	Resource(Resource &&) = delete;
public:
	using Ptr = std::shared_ptr<Resource>;

	~Resource();

	/// The returned pointer needs to be stored until either Resource::applyTo or Resource::then is called, or it is passed as
	/// a placeholder to Resource::create itself.
	static Ptr create(const QString &resource, Ptr placeholder = nullptr);

	/// Use these functions to specify what should happen when e.g. the resource changes
	Ptr applyTo(ResourceObserver *observer);
	Ptr applyTo(QObject *target, const char *property = nullptr);
	template<typename Func>
	Ptr then(Func &&func)
	{
		// Arg will be the functions argument with references and cv-qualifiers (const, volatile) removed
		using Arg = TypeMagic::CleanType<typename TypeMagic::Function<Func>::Argument>;
		// Ret will be the functions return type
		using Ret = typename TypeMagic::Function<Func>::ReturnType;

		// FunctionResourceObserver<ReturnType, ArgumentType, FunctionSignature>
		return applyTo(new FunctionResourceObserver<Ret, Arg, Func>(std::forward<Func>(func)));
	}

	/// Retrieve the currently active resource. If it's type is different from T a conversion will be attempted.
	template<typename T>
	T getResource() const { return getResourceInternal(qMetaTypeId<T>()).template value<T>(); }

	/// @internal Used by ResourceObserver and ResourceProxyModel
	QVariant getResourceInternal(const int typeId) const;

	/** Register a new ResourceHandler. T needs to inherit from ResourceHandler
	 * Usage: Resource::registerHandler<MyResourceHandler>("myid");
	 */
	template<typename T>
	static void registerHandler(const QString &id)
	{
		m_handlers.insert(id, [](const QString &res) { return std::make_shared<T>(res); });
	}
	/** Register a new resource transformer
	 * Resource transformers are functions that are responsible for converting between different types,
	 * for example converting from a QByteArray to a QPixmap. They are registered "externally" because not
	 * all types might be available in this library, for example gui types like QPixmap.
	 *
	 * Usage: Resource::registerTransformer([](const InputType &type) { return OutputType(type); });
	 *   This assumes that OutputType has a constructor that takes InputType as an argument. More
	 *   complicated transformers can of course also be registered.
	 *
	 * When a ResourceObserver requests a type that's different from the actual resource type, a matching
	 * transformer will be looked up from the list of transformers.
	 * @note Only one-stage transforms will be performed (you can't registerTransformers for QString => int
	 *       and int => float and expect QString to automatically be transformed into a float.
	 */
	template<typename Func>
	static void registerTransformer(Func &&func)
	{
		using Out = typename TypeMagic::Function<Func>::ReturnType;
		using In = TypeMagic::CleanType<typename TypeMagic::Function<Func>::Argument>;
		static_assert(!std::is_same<Out, In>::value, "It does not make sense to transform a value to itself");
		m_transfomers.insert(qMakePair(qMetaTypeId<In>(), qMetaTypeId<Out>()), [func](const QVariant &in)
		{
			return QVariant::fromValue<Out>(func(in.value<In>()));
		});
	}

private: // half private, implementation details
	friend class ResourceHandler;
	// the following three functions are called by ResourceHandlers
	/** Notifies the observers. They will call Resource::getResourceInternal which will call ResourceHandler::result
	 * or delegate to it's placeholder.
	 */
	void reportResult();
	void reportFailure(const QString &reason);
	void reportProgress(const int progress);

	friend class ResourceObserver;
	/// Removes observer from the list of observers so that we don't attempt to notify something that doesn't exist
	void notifyObserverDeleted(ResourceObserver *observer);

private: // truly private
	QList<ResourceObserver *> m_observers;
	std::shared_ptr<ResourceHandler> m_handler = nullptr;
	Ptr m_placeholder = nullptr;
	const QString m_resource;

	static QString storageIdentifier(const QString &id, Ptr placeholder = nullptr);
	QString storageIdentifier() const;

	// a list of resource handler factories, registered using registerHandler
	static QMap<QString, std::function<std::shared_ptr<ResourceHandler>(const QString &)>> m_handlers;
	// a list of resource transformers, registered using registerTransformer
	static QMap<QPair<int, int>, std::function<QVariant(QVariant)>> m_transfomers;
	// a list of resources so that we can reuse them
	static QMap<QString, std::weak_ptr<Resource>> m_resources;
};
