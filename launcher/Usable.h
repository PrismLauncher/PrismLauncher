#pragma once

#include <cstddef>
#include <memory>

#include "QObjectPtr.h"

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
    std::size_t useCount() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_useCount;
    }
    bool isInUse() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_useCount > 0;
    }
protected:
    virtual void decrementUses()
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_useCount--;
    }
    virtual void incrementUses()
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_useCount++;
    }
private:
    std::size_t hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_useCount = 0;
};

/**
 * Lock class to use for keeping track of uses of other things derived from Usable
 *
 * @see Usable
 */
class UseLock
{
public:
    UseLock(shared_qobject_ptr<Usable> usable)
        : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_usable(usable)
    {
        // this doesn't use shared pointer use count, because that wouldn't be correct. this count is separate.
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_usable->incrementUses();
    }
    ~UseLock()
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_usable->decrementUses();
    }
private:
    shared_qobject_ptr<Usable> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_usable;
};
