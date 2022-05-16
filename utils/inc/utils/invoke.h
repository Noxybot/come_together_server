#pragma once

#include <string>
#include <typeindex>


template<class T, class RowT, class... Args, size_t... Is>
void invoke_impl(T& obj, const RowT& v, std::index_sequence<Is...>, Args... args)
{
    if constexpr (std::is_same_v<RowT, std::vector<std::string>>)
        ((obj.*args)(v[Is]), ...);
    else
        ((obj.*args)(&v[Is].as<std::string>("")), ...);
}

template<class T, class RowT, class... Args>
void invoker(T& obj, const RowT& v, Args&&... args)
{
    invoke_impl(obj, v, std::index_sequence_for<Args...>{}, std::forward<Args>(args)...);
}


template <typename Ret, typename Obj>
auto cast_member(Ret (Obj::*fp)(std::string*))
{
   return fp;
}