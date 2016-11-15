#pragma once

#include <cstddef>
#include <memory>

class Usable;

/**
 * Base class for things that can be used by multiple other things and we want to track the use count.
 *
 * @see UseLock
 */
class Usable
{
	friend class UseLock;
public:
	std::size_t useCount()
	{
		return m_useCount;
	}
	bool isInUse()
	{
		return m_useCount > 0;
	}
protected:
	virtual void decrementUses()
	{
		m_useCount--;
	}
	virtual void incrementUses()
	{
		m_useCount++;
	}
private:
	std::size_t m_useCount = 0;
};

/**
 * Lock class to use for keeping track of uses of other things derived from Usable
 *
 * @see Usable
 */
class UseLock
{
public:
	UseLock(std::shared_ptr<Usable> usable)
		: m_usable(usable)
	{
		// this doesn't use shared pointer use count, because that wouldn't be correct. this count is separate.
		m_usable->incrementUses();
	}
	~UseLock()
	{
		m_usable->decrementUses();
	}
private:
	std::shared_ptr<Usable> m_usable;
};
