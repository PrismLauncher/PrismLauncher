#pragma once

#include <memory>

/**
 * A pointer class with the usual shared pointer semantics intended for derivates of QObject
 * Calls deleteLater() instead of destroying the contained object immediately
 */
template <typename T>
class QObjectPtr
{
public:
	QObjectPtr(){}
	QObjectPtr(T * wrap)
	{
		reset(wrap);
	}
	QObjectPtr(const QObjectPtr<T>& other)
	{
		m_ptr = other.m_ptr;
	}
	template<typename Derived>
	QObjectPtr(const QObjectPtr<Derived> &other)
	{
		m_ptr = other.unwrap();
	}

public:
	void reset(T * wrap)
	{
		using namespace std::placeholders;
		m_ptr.reset(wrap, std::bind(&QObject::deleteLater, _1));
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
