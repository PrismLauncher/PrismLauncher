
#pragma once

template <typename T>
inline void clamp(T& current, T min, T max)
{
    if (current < min)
    {
        current = min;
    }
    else if(current > max)
    {
        current = max;
    }
}

// List of numbers from min to max. Next is exponent times bigger than previous.

class ExponentialSeries
{
public:
    ExponentialSeries(unsigned min, unsigned max, unsigned exponent = 2)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_current = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_min = min;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_max = max;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_exponent = exponent;
    }
    void reset()
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_current = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_min;
    }
    unsigned operator()()
    {
        unsigned retval = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_current;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_current *= hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_exponent;
        clamp(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_current, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_min, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_max);
        return retval;
    }
    unsigned hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_current;
    unsigned hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_min;
    unsigned hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_max;
    unsigned hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_exponent;
};
