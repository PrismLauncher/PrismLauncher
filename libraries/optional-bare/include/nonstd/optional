//
// Copyright 2017-2019 by Martin Moene
//
// https://github.com/martinmoene/optional-bare
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef NONSTD_OPTIONAL_BARE_HPP
#define NONSTD_OPTIONAL_BARE_HPP

#define optional_bare_MAJOR  1
#define optional_bare_MINOR  1
#define optional_bare_PATCH  0

#define optional_bare_VERSION  optional_STRINGIFY(optional_bare_MAJOR) "." optional_STRINGIFY(optional_bare_MINOR) "." optional_STRINGIFY(optional_bare_PATCH)

#define optional_STRINGIFY(  x )  optional_STRINGIFY_( x )
#define optional_STRINGIFY_( x )  #x

// optional-bare configuration:

#define optional_OPTIONAL_DEFAULT  0
#define optional_OPTIONAL_NONSTD   1
#define optional_OPTIONAL_STD      2

#if !defined( optional_CONFIG_SELECT_OPTIONAL )
# define optional_CONFIG_SELECT_OPTIONAL  ( optional_HAVE_STD_OPTIONAL ? optional_OPTIONAL_STD : optional_OPTIONAL_NONSTD )
#endif

// Control presence of exception handling (try and auto discover):

#ifndef optional_CONFIG_NO_EXCEPTIONS
# if _MSC_VER
#  include <cstddef>    // for _HAS_EXCEPTIONS
# endif
# if _MSC_VER
#  include <cstddef>    // for _HAS_EXCEPTIONS
# endif
# if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || (_HAS_EXCEPTIONS)
#  define optional_CONFIG_NO_EXCEPTIONS  0
# else
#  define optional_CONFIG_NO_EXCEPTIONS  1
# endif
#endif

// C++ language version detection (C++20 is speculative):
// Note: VC14.0/1900 (VS2015) lacks too much from C++14.

#ifndef   optional_CPLUSPLUS
# if defined(_MSVC_LANG ) && !defined(__clang__)
#  define optional_CPLUSPLUS  (_MSC_VER == 1900 ? 201103L : _MSVC_LANG )
# else
#  define optional_CPLUSPLUS  __cplusplus
# endif
#endif

#define optional_CPP98_OR_GREATER  ( optional_CPLUSPLUS >= 199711L )
#define optional_CPP11_OR_GREATER  ( optional_CPLUSPLUS >= 201103L )
#define optional_CPP14_OR_GREATER  ( optional_CPLUSPLUS >= 201402L )
#define optional_CPP17_OR_GREATER  ( optional_CPLUSPLUS >= 201703L )
#define optional_CPP20_OR_GREATER  ( optional_CPLUSPLUS >= 202000L )

// C++ language version (represent 98 as 3):

#define optional_CPLUSPLUS_V  ( optional_CPLUSPLUS / 100 - (optional_CPLUSPLUS > 200000 ? 2000 : 1994) )

// Use C++17 std::optional if available and requested:

#if optional_CPP17_OR_GREATER && defined(__has_include )
# if __has_include( <optional> )
#  define optional_HAVE_STD_OPTIONAL  1
# else
#  define optional_HAVE_STD_OPTIONAL  0
# endif
#else
# define  optional_HAVE_STD_OPTIONAL  0
#endif

#define optional_USES_STD_OPTIONAL  ( (optional_CONFIG_SELECT_OPTIONAL == optional_OPTIONAL_STD) || ((optional_CONFIG_SELECT_OPTIONAL == optional_OPTIONAL_DEFAULT) && optional_HAVE_STD_OPTIONAL) )

//
// Using std::optional:
//

#if optional_USES_STD_OPTIONAL

#include <optional>
#include <utility>

namespace nonstd {

    using std::in_place;
    using std::in_place_type;
    using std::in_place_index;
    using std::in_place_t;
    using std::in_place_type_t;
    using std::in_place_index_t;

    using std::optional;
    using std::bad_optional_access;
    using std::hash;

    using std::nullopt;
    using std::nullopt_t;

    using std::operator==;
    using std::operator!=;
    using std::operator<;
    using std::operator<=;
    using std::operator>;
    using std::operator>=;
    using std::make_optional;
    using std::swap;
}

#else // optional_USES_STD_OPTIONAL

#include <cassert>

#if ! optional_CONFIG_NO_EXCEPTIONS
# include <stdexcept>
#endif

namespace nonstd { namespace optional_bare {

// type for nullopt

struct nullopt_t
{
    struct init{};
    nullopt_t( init ) {}
};

// extra parenthesis to prevent the most vexing parse:

const nullopt_t nullopt(( nullopt_t::init() ));

// optional access error.

#if ! optional_CONFIG_NO_EXCEPTIONS

class bad_optional_access : public std::logic_error
{
public:
  explicit bad_optional_access()
  : logic_error( "bad optional access" ) {}
};

#endif // optional_CONFIG_NO_EXCEPTIONS

// Simplistic optional: requires T to be default constructible, copyable.

template< typename T >
class optional
{
private:
    typedef void (optional::*safe_bool)() const;

public:
    typedef T value_type;

    optional()
    : has_value_( false )
    {}

    optional( nullopt_t )
    : has_value_( false )
    {}

    optional( T const & arg )
    : has_value_( true )
    , value_    ( arg  )
    {}

    template< class U >
    optional( optional<U> const & other )
    : has_value_( other.has_value() )
    , value_    ( other.value()     )
    {}

    optional & operator=( nullopt_t )
    {
        reset();
        return *this;
    }

    template< class U >
    optional & operator=( optional<U> const & other )
    {
        has_value_ = other.has_value();
        value_     = other.value();
        return *this;
    }

    void swap( optional & rhs )
    {
        using std::swap;
        if      ( has_value() == true  && rhs.has_value() == true  ) { swap( **this, *rhs ); }
        else if ( has_value() == false && rhs.has_value() == true  ) { initialize( *rhs ); rhs.reset(); }
        else if ( has_value() == true  && rhs.has_value() == false ) { rhs.initialize( **this ); reset(); }
    }

    // observers

    value_type const * operator->() const
    {
        return assert( has_value() ),
            &value_;
    }

    value_type * operator->()
    {
        return assert( has_value() ),
            &value_;
    }

    value_type const & operator*() const
    {
        return assert( has_value() ),
            value_;
    }

    value_type & operator*()
    {
        return assert( has_value() ),
            value_;
    }

#if optional_CPP11_OR_GREATER
    explicit operator bool() const
    {
        return has_value();
    }
#else
    operator safe_bool() const
    {
        return has_value() ? &optional::this_type_does_not_support_comparisons : 0;
    }
#endif

    bool has_value() const
    {
        return has_value_;
    }

    value_type const & value() const
    {
#if optional_CONFIG_NO_EXCEPTIONS
        assert( has_value() );
#else
        if ( ! has_value() )
            throw bad_optional_access();
#endif
        return value_;
    }

    value_type & value()
    {
#if optional_CONFIG_NO_EXCEPTIONS
        assert( has_value() );
#else
        if ( ! has_value() )
            throw bad_optional_access();
#endif
        return value_;
    }

    template< class U >
    value_type value_or( U const & v ) const
    {
        return has_value() ? value() : static_cast<value_type>( v );
    }

    // modifiers

    void reset()
    {
        has_value_ = false;
    }

private:
    void this_type_does_not_support_comparisons() const {}

    template< typename V >
    void initialize( V const & value )
    {
        assert( ! has_value()  );
        value_ = value;
        has_value_ = true;
    }

private:
    bool has_value_;
    value_type value_;
};

// Relational operators

template< typename T, typename U >
inline bool operator==( optional<T> const & x, optional<U> const & y )
{
    return bool(x) != bool(y) ? false : bool(x) == false ? true : *x == *y;
}

template< typename T, typename U >
inline bool operator!=( optional<T> const & x, optional<U> const & y )
{
    return !(x == y);
}

template< typename T, typename U >
inline bool operator<( optional<T> const & x, optional<U> const & y )
{
    return (!y) ? false : (!x) ? true : *x < *y;
}

template< typename T, typename U >
inline bool operator>( optional<T> const & x, optional<U> const & y )
{
    return (y < x);
}

template< typename T, typename U >
inline bool operator<=( optional<T> const & x, optional<U> const & y )
{
    return !(y < x);
}

template< typename T, typename U >
inline bool operator>=( optional<T> const & x, optional<U> const & y )
{
    return !(x < y);
}

// Comparison with nullopt

template< typename T >
inline bool operator==( optional<T> const & x, nullopt_t )
{
    return (!x);
}

template< typename T >
inline bool operator==( nullopt_t, optional<T> const & x )
{
    return (!x);
}

template< typename T >
inline bool operator!=( optional<T> const & x, nullopt_t )
{
    return bool(x);
}

template< typename T >
inline bool operator!=( nullopt_t, optional<T> const & x )
{
    return bool(x);
}

template< typename T >
inline bool operator<( optional<T> const &, nullopt_t )
{
    return false;
}

template< typename T >
inline bool operator<( nullopt_t, optional<T> const & x )
{
    return bool(x);
}

template< typename T >
inline bool operator<=( optional<T> const & x, nullopt_t )
{
    return (!x);
}

template< typename T >
inline bool operator<=( nullopt_t, optional<T> const & )
{
    return true;
}

template< typename T >
inline bool operator>( optional<T> const & x, nullopt_t )
{
    return bool(x);
}

template< typename T >
inline bool operator>( nullopt_t, optional<T> const & )
{
    return false;
}

template< typename T >
inline bool operator>=( optional<T> const &, nullopt_t )
{
    return true;
}

template< typename T >
inline bool operator>=( nullopt_t, optional<T> const & x )
{
    return (!x);
}

// Comparison with T

template< typename T, typename U >
inline bool operator==( optional<T> const & x, U const & v )
{
    return bool(x) ? *x == v : false;
}

template< typename T, typename U >
inline bool operator==( U const & v, optional<T> const & x )
{
    return bool(x) ? v == *x : false;
}

template< typename T, typename U >
inline bool operator!=( optional<T> const & x, U const & v )
{
    return bool(x) ? *x != v : true;
}

template< typename T, typename U >
inline bool operator!=( U const & v, optional<T> const & x )
{
    return bool(x) ? v != *x : true;
}

template< typename T, typename U >
inline bool operator<( optional<T> const & x, U const & v )
{
    return bool(x) ? *x < v : true;
}

template< typename T, typename U >
inline bool operator<( U const & v, optional<T> const & x )
{
    return bool(x) ? v < *x : false;
}

template< typename T, typename U >
inline bool operator<=( optional<T> const & x, U const & v )
{
    return bool(x) ? *x <= v : true;
}

template< typename T, typename U >
inline bool operator<=( U const & v, optional<T> const & x )
{
    return bool(x) ? v <= *x : false;
}

template< typename T, typename U >
inline bool operator>( optional<T> const & x, U const & v )
{
    return bool(x) ? *x > v : false;
}

template< typename T, typename U >
inline bool operator>( U const & v, optional<T> const & x )
{
    return bool(x) ? v > *x : true;
}

template< typename T, typename U >
inline bool operator>=( optional<T> const & x, U const & v )
{
    return bool(x) ? *x >= v : false;
}

template< typename T, typename U >
inline bool operator>=( U const & v, optional<T> const & x )
{
    return bool(x) ? v >= *x : true;
}

// Specialized algorithms

template< typename T >
void swap( optional<T> & x, optional<T> & y )
{
    x.swap( y );
}

// Convenience function to create an optional.

template< typename T >
inline optional<T> make_optional( T const & v )
{
    return optional<T>( v );
}

} // namespace optional-bare

using namespace optional_bare;

} // namespace nonstd

#endif // optional_USES_STD_OPTIONAL

#endif // NONSTD_OPTIONAL_BARE_HPP
