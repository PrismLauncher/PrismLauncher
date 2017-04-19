#pragma once

#include <functional>
#include <memory>
#include <QObject>

namespace details
{
struct DeleteQObjectLater
{
	void operator()(QObject *obj) const
	{
		obj->deleteLater();
	}
};
}
/**
 * A unique pointer class with unique pointer semantics intended for derivates of QObject
 * Calls deleteLater() instead of destroying the contained object immediately
 */
template<typename T> using unique_qobject_ptr = std::unique_ptr<T, details::DeleteQObjectLater>;

/**
 * A shared pointer class with shared pointer semantics intended for derivates of QObject
 * Calls deleteLater() instead of destroying the contained object immediately
 */
template <typename T>
class shared_qobject_ptr
{
public:
	shared_qobject_ptr(){}
	shared_qobject_ptr(T * wrap)
	{
		reset(wrap);
	}
	shared_qobject_ptr(const shared_qobject_ptr<T>& other)
	{
		m_ptr = other.m_ptr;
	}
	template<typename Derived>
	shared_qobject_ptr(const shared_qobject_ptr<Derived> &other)
	{
		m_ptr = other.unwrap();
	}

public:
	void reset(T * wrap)
	{
		using namespace std::placeholders;
		m_ptr.reset(wrap, std::bind(&QObject::deleteLater, _1));
	}
	void reset(const shared_qobject_ptr<T> &other)
	{
		m_ptr = other.m_ptr;
	}
	void reset()
	{
		m_ptr.reset();
	}
	T * get() const
	{
		return m_ptr.get();
	}
	T * operator->() const
	{
		return m_ptr.get();
	}
	T & operator*() const
	{
		return *m_ptr.get();
	}
	operator bool() const
	{
		return m_ptr.get() != nullptr;
	}
	const std::shared_ptr <T> unwrap() const
	{
		return m_ptr;
	}

private:
	std::shared_ptr <T> m_ptr;
};
