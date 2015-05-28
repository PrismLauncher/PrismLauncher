#pragma once

#include <QString>
#include <QMap>
#include <QVariant>
#include <functional>
#include <memory>

#include "ResourceObserver.h"

class ResourceHandler;

namespace Detail
{
template <typename T> struct Function : public Function<decltype(&T::operator())> {};
template <typename Ret, typename Arg> struct Function<Ret(*)(Arg)> : public Function<Ret(Arg)> {};
template <typename Ret, typename Arg> struct Function<Ret(Arg)>
{
	using ReturnType = Ret;
	using Argument = Arg;
};
template <class C, typename Ret, typename Arg> struct Function<Ret(C::*)(Arg)> : public Function<Ret(Arg)> {};
template <class C, typename Ret, typename Arg> struct Function<Ret(C::*)(Arg) const> : public Function<Ret(Arg)> {};
template <typename F> struct Function<F&> : public Function<F> {};
template <typename F> struct Function<F&&> : public Function<F> {};
}

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
 *	\note Always pass resource around using ResourcePtr! Copy and move constructors are disabled for a reason.
 */
class Resource : public std::enable_shared_from_this<Resource>
{
	explicit Resource(const QString &resource);
	Resource(const Resource &) = delete;
	Resource(Resource &&) = delete;
public:
	using Ptr = std::shared_ptr<Resource>;

	~Resource();

	/// The returned pointer needs to be stored until either Resource::then is called, or it is used as the argument to Resource::placeholder.
	static Ptr create(const QString &resource);

	/// This can e.g. be used to set a local icon as the placeholder while a slow (remote) icon is fetched
	Ptr placeholder(Ptr other);

	/// Use these functions to specify what should happen when e.g. the resource changes
	Ptr applyTo(ResourceObserver *observer);
	Ptr applyTo(QObject *target, const char *property = nullptr);
	template<typename Func>
	Ptr then(Func &&func)
	{
		using Arg = typename std::remove_cv<
			typename std::remove_reference<typename Detail::Function<Func>::Argument>::type
		>::type;
		return applyTo(new FunctionResourceObserver<
					   typename Detail::Function<Func>::ReturnType,
					   Arg, Func
					   >(std::forward<Func>(func)));
	}

	/// Retrieve the currently active resource. If it's type is different from T a conversion will be attempted.
	template<typename T>
	T getResource() const { return getResourceInternal(qMetaTypeId<T>()).template value<T>(); }
	QVariant getResourceInternal(const int typeId) const;

	template<typename T>
	static void registerHandler(const QString &id)
	{
		m_handlers.insert(id, [](const QString &res) { return std::make_shared<T>(res); });
	}
	template<typename Func>
	static void registerTransformer(Func &&func)
	{
		using Out = typename Detail::Function<Func>::ReturnType;
		using In = typename std::remove_cv<typename std::remove_reference<typename Detail::Function<Func>::Argument>::type>::type;
		static_assert(!std::is_same<Out, In>::value, "It does not make sense to transform a value to itself");
		m_transfomers.insert(qMakePair(qMetaTypeId<In>(), qMetaTypeId<Out>()), [func](const QVariant &in)
		{
			return QVariant::fromValue<Out>(func(in.value<In>()));
		});
	}

private:
	friend class ResourceHandler;
	void reportResult();
	void reportFailure(const QString &reason);
	void reportProgress(const int progress);

	friend class ResourceObserver;
	void notifyObserverDeleted(ResourceObserver *observer);

private:
	QList<ResourceObserver *> m_observers;
	std::shared_ptr<ResourceHandler> m_handler = nullptr;
	Ptr m_placeholder = nullptr;

	// a list of resource handler factories, registered using registerHandler
	static QMap<QString, std::function<std::shared_ptr<ResourceHandler>(const QString &)>> m_handlers;
	// a list of resource transformers, registered using registerTransformer
	static QMap<QPair<int, int>, std::function<QVariant(QVariant)>> m_transfomers;
	static QMap<QString, std::weak_ptr<Resource>> m_resources;
};
