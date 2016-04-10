#pragma once

namespace TypeMagic
{
/** "Cleans" the given type T by stripping references (&) and cv-qualifiers (const, volatile) from it
 * const int => int
 * QString & => QString
 * const unsigned long long & => unsigned long long
 *
 * Usage:
 *   using Cleaned = Detail::CleanType<const int>;
 *   static_assert(std::is_same<Cleaned, int>, "Cleaned == int");
 */
// the order of remove_cv and remove_reference matters!
template <typename T>
using CleanType = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

/// For functors (structs with operator()), including lambdas, which in **most** cases are functors
/// "Calls" Function<Ret(*)(Arg)> or Function<Ret(C::*)(Arg)>
template <typename T> struct Function : public Function<decltype(&T::operator())> {};
/// For function pointers (&function), including static members (&Class::member)
template <typename Ret, typename Arg> struct Function<Ret(*)(Arg)> : public Function<Ret(Arg)> {};
/// Default specialization used by others.
template <typename Ret, typename Arg> struct Function<Ret(Arg)>
{
	using ReturnType = Ret;
	using Argument = Arg;
};
/// For member functions. Also used by the lambda overload if the lambda captures [this]
template <class C, typename Ret, typename Arg> struct Function<Ret(C::*)(Arg)> : public Function<Ret(Arg)> {};
template <class C, typename Ret, typename Arg> struct Function<Ret(C::*)(Arg) const> : public Function<Ret(Arg)> {};
/// Overload for references
template <typename F> struct Function<F&> : public Function<F> {};
/// Overload for rvalues
template <typename F> struct Function<F&&> : public Function<F> {};
// for more info: https://functionalcpp.wordpress.com/2013/08/05/function-traits/
}
