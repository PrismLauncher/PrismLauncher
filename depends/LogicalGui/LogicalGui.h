/* Copyright 2014 Jan Dalheimer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QObject>
#include <QMetaMethod>
#include <QThread>
#include <QCoreApplication>
#include <QSemaphore>
#include <memory>

#if QT_VERSION == QT_VERSION_CHECK(5, 1, 1)
#include <5.1.1/QtCore/private/qobject_p.h>
#elif QT_VERSION == QT_VERSION_CHECK(5, 2, 0)
#include <5.2.0/QtCore/private/qobject_p.h>
#elif QT_VERSION == QT_VERSION_CHECK(5, 2, 1)
#include <5.2.1/QtCore/private/qobject_p.h>
#elif QT_VERSION == QT_VERSION_CHECK(5, 3, 0)
#include <5.3.0/QtCore/private/qobject_p.h>
#elif QT_VERSION == QT_VERSION_CHECK(5, 3, 1)
#include <5.3.1/QtCore/private/qobject_p.h>
#else
#error Please add support for this version of Qt
#endif

class Bindable
{
	friend class tst_LogicalGui;

public:
	Bindable(Bindable *parent = 0) : m_parent(parent)
	{
	}
	virtual ~Bindable()
	{
	}

	void setBindableParent(Bindable *parent)
	{
		m_parent = parent;
	}

	void bind(const QString &id, const QObject *receiver, const char *methodSignature)
	{
		auto mo = receiver->metaObject();
		Q_ASSERT_X(mo, "Bindable::bind",
				   "Invalid metaobject. Did you forget the QObject macro?");
		const QMetaMethod method = mo->method(mo->indexOfMethod(
			QMetaObject::normalizedSignature(methodSignature + 1).constData()));
		Q_ASSERT_X(method.isValid(), "Bindable::bind", "Invalid method signature");
		m_bindings.insert(id, Binding(receiver, method));
	}
	template <typename Func>
	void bind(const QString &id,
			  const typename QtPrivate::FunctionPointer<Func>::Object *receiver, Func slot)
	{
		typedef QtPrivate::FunctionPointer<Func> SlotType;
		m_bindings.insert(
			id,
			Binding(receiver, new QtPrivate::QSlotObject<Func, typename SlotType::Arguments,
														 typename SlotType::ReturnType>(slot)));
	}
	template <typename Func> void bind(const QString &id, Func slot)
	{
		typedef QtPrivate::FunctionPointer<Func> SlotType;
		m_bindings.insert(
			id,
			Binding(nullptr, new QtPrivate::QSlotObject<Func, typename SlotType::Arguments,
														typename SlotType::ReturnType>(slot)));
	}
	void unbind(const QString &id)
	{
		m_bindings.remove(id);
	}

private:
	struct Binding
	{
		Binding(const QObject *receiver, const QMetaMethod &method)
			: receiver(receiver), method(method)
		{
		}
		Binding(const QObject *receiver, QtPrivate::QSlotObjectBase *object)
			: receiver(receiver), object(object)
		{
		}
		Binding()
		{
		}
		const QObject *receiver;
		QMetaMethod method;
		QtPrivate::QSlotObjectBase *object = nullptr;
	};
	QMap<QString, Binding> m_bindings;

	Bindable *m_parent;

private:
	inline Qt::ConnectionType connectionType(const QObject *receiver)
	{
		return receiver == nullptr ? Qt::DirectConnection
								   : (QThread::currentThread() == receiver->thread()
										  ? Qt::DirectConnection
										  : Qt::BlockingQueuedConnection);
	}

protected:
	template <typename Ret, typename... Params> Ret wait(const QString &id, Params... params)
	{
		static_assert(!std::is_same<Ret, void>::value, "You need to use Bindable::waitVoid");

		if (!m_bindings.contains(id) && m_parent)
		{
			return m_parent->wait<Ret, Params...>(id, params...);
		}
		Q_ASSERT(m_bindings.contains(id));
		const auto binding = m_bindings[id];
		const Qt::ConnectionType type = connectionType(binding.receiver);
		Ret ret;
		if (binding.object)
		{
			void *args[] = {&ret,
							const_cast<void *>(reinterpret_cast<const void *>(&params))...};
			if (type == Qt::BlockingQueuedConnection)
			{
				QSemaphore semaphore;
				QMetaCallEvent *ev =
					new QMetaCallEvent(binding.object, nullptr, -1, 0, 0, args, &semaphore);
				QCoreApplication::postEvent(const_cast<QObject *>(binding.receiver), ev);
				semaphore.acquire();
			}
			else
			{
				binding.object->call(const_cast<QObject *>(binding.receiver), args);
			}
		}
		else
		{
			const QMetaMethod method = binding.method;
			Q_ASSERT_X(method.parameterCount() == sizeof...(params), "Bindable::wait",
					   qPrintable(QString("Incompatible argument count (expected %1, got %2)")
									  .arg(method.parameterCount(), sizeof...(params))));
			Q_ASSERT_X(
				qMetaTypeId<Ret>() != QMetaType::UnknownType, "Bindable::wait",
				"Requested return type is not registered, please use the Q_DECLARE_METATYPE "
				"macro to make it known to Qt's meta-object system");
			Q_ASSERT_X(
				method.returnType() == qMetaTypeId<Ret>() ||
					QMetaType::hasRegisteredConverterFunction(method.returnType(),
															  qMetaTypeId<Ret>()),
				"Bindable::wait",
				qPrintable(
					QString(
						"Requested return type (%1) is incompatible method return type (%2)")
						.arg(QMetaType::typeName(qMetaTypeId<Ret>()),
							 QMetaType::typeName(method.returnType()))));
			const auto retArg = QReturnArgument<Ret>(
				QMetaType::typeName(qMetaTypeId<Ret>()),
				ret); // because Q_RETURN_ARG doesn't work with templates...
			method.invoke(const_cast<QObject *>(binding.receiver), type, retArg,
						  Q_ARG(Params, params)...);
		}
		return ret;
	}
	template <typename... Params>
	typename std::enable_if<sizeof...(Params) != 0, void>::type waitVoid(const QString &id,
																		 Params... params)
	{
		if (!m_bindings.contains(id) && m_parent)
		{
			m_parent->waitVoid<Params...>(id, params...);
			return;
		}
		Q_ASSERT(m_bindings.contains(id));
		const auto binding = m_bindings[id];
		const Qt::ConnectionType type = connectionType(binding.receiver);
		if (binding.object)
		{
			void *args[] = {0, const_cast<void *>(reinterpret_cast<const void *>(&params))...};
			if (type == Qt::BlockingQueuedConnection)
			{
				QSemaphore semaphore;
				QMetaCallEvent *ev =
					new QMetaCallEvent(binding.object, nullptr, -1, 0, 0, args, &semaphore);
				QCoreApplication::postEvent(const_cast<QObject *>(binding.receiver), ev);
				semaphore.acquire();
			}
			else
			{
				binding.object->call(const_cast<QObject *>(binding.receiver), args);
			}
		}
		else
		{
			const QMetaMethod method = binding.method;
			Q_ASSERT_X(method.parameterCount() == sizeof...(params), "Bindable::wait",
					   qPrintable(QString("Incompatible argument count (expected %1, got %2)")
									  .arg(method.parameterCount(), sizeof...(params))));
			method.invoke(const_cast<QObject *>(binding.receiver), type,
						  Q_ARG(Params, params)...);
		}
	}
	void waitVoid(const QString &id)
	{
		if (!m_bindings.contains(id) && m_parent)
		{
			m_parent->waitVoid(id);
			return;
		}
		Q_ASSERT(m_bindings.contains(id));
		const auto binding = m_bindings[id];
		const Qt::ConnectionType type = connectionType(binding.receiver);
		if (binding.object)
		{
			void *args[] = {0};
			if (type == Qt::BlockingQueuedConnection)
			{
				QSemaphore semaphore;
				QMetaCallEvent *ev =
					new QMetaCallEvent(binding.object, nullptr, -1, 0, 0, args, &semaphore);
				QCoreApplication::postEvent(const_cast<QObject *>(binding.receiver), ev);
				semaphore.acquire();
			}
			else
			{
				binding.object->call(const_cast<QObject *>(binding.receiver), args);
			}
		}
		else
		{
			const QMetaMethod method = binding.method;
			Q_ASSERT_X(method.parameterCount() == 0, "Bindable::wait",
					   qPrintable(QString("Incompatible argument count (expected %1, got %2)")
									  .arg(method.parameterCount(), 0)));
			method.invoke(const_cast<QObject *>(binding.receiver), type);
		}
	}
};

// used frequently
Q_DECLARE_METATYPE(bool *)
