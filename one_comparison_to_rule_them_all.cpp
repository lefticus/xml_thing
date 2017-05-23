#include <algorithm>
#include <functional>


template<typename Comparison, typename First, typename ... T>
constexpr decltype(auto) variadic_compare(Comparison &&comparison, First && first, T && ... t)
{
  // noexcept is almost impossible to get correct
  // we have to know if each comparison is noexcept
  // and if there is a copy involved, and if that is noexcept
  
  constexpr bool is_const = ( std::is_const_v<std::remove_reference_t<First>> || ... || std::is_const_v<std::remove_reference_t<T>> );
  using CommonType = std::common_type_t<std::decay_t<First>, std::decay_t<T>...>;

  constexpr bool is_ptr_convertible = ( std::is_convertible_v<std::decay_t<First> *, CommonType *> && ... && std::is_convertible_v<std::decay_t<T> *, CommonType *> );

  if constexpr( is_ptr_convertible ) {
    using Type = std::conditional_t<is_const,
                                  const std::decay_t<CommonType> *,
                                  std::decay_t<CommonType> *>;

    Type retval = &first;

    ( ( retval = comparison(*retval, t) ? retval : &t), ... );

    constexpr bool is_rvalue = ( std::is_rvalue_reference_v<decltype(first)> && ... && std::is_rvalue_reference_v<decltype(t)> );

    if constexpr ( is_rvalue ) {
      return std::move(*retval);
    } else {
      return (*retval);
    }
  } else {
    CommonType v = first;
    ((v = (comparison(v, t) ? v : t)), ...);
    return v;
  }
}

template<typename ... T>
constexpr decltype(auto) variadic_min(T && ... t) noexcept(noexcept(variadic_compare(std::less<>{}, std::forward<T>(t)...)))
{
  return variadic_compare(std::less<>{}, std::forward<T>(t)...);
}


struct Base
{
  int val = 0;
  constexpr bool operator<(const Base &other) const noexcept
  {
    return val < other.val;
  }
};

struct Derived : Base
{

};

int main()
{
  static_assert(std::is_same_v<decltype(variadic_min(1,2,3)), int &&>);

  // this shouldn't compile
  //variadic_min(1,2,3) = 4;

  int i = 1;
  int j = 2;
  int k = 3;

  variadic_min(i,j,k) = 15;
  static_assert(std::is_same_v<decltype(variadic_min(i,j,11.2f)), float>);
  float v = variadic_min(i, j, 11.2f);

  const int l = 1;
  const int m = 2;

  static_assert(variadic_min(3.2, 1.3f, 9) == 1.3f);

  variadic_min(l, m);

  Base b{10};
  Derived d{9};

  Base &result = variadic_min(b, d);
  result.val += 11; // this is 9 + 11

  // and i + j + k == 20, so result is 40
  return i + j + k + d.val;
}
