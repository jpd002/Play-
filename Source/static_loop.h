#pragma once

//Source: https://stackoverflow.com/a/46873787

#include <type_traits>
#include <utility>

namespace detail
{
	template <class T, T... inds, class F>
	constexpr void static_loop(std::integer_sequence<T, inds...>, F&& f)
	{
		(f(std::integral_constant<T, inds>{}), ...); // C++17 fold expression
	}
} // detail

template <class T, T count, class F>
constexpr void static_loop(F&& f)
{
	detail::static_loop(std::make_integer_sequence<T, count>{}, std::forward<F>(f));
}
