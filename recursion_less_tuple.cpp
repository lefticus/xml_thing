#include <cstdint>
#include <utility>
#include <memory>

///
/// Tuple Implementation
///

/// Type, Index of base class, empty object optimization
template<typename T, std::size_t I, bool = std::is_empty<T>() && !std::is_final<T>() >
struct TupleData : T
{
  constexpr explicit TupleData(T t) : T(std::move(t))
  {
  }
  
  TupleData() = default;
};

/// all other objects
template<typename T, std::size_t I>
struct TupleData<T, I, false>
{
  T t;

  constexpr explicit TupleData(T t_t) : t(std::move(t_t)) {}
  
  TupleData() = default;
};

/// Base class that inherits from all TupleData objects needed
template <typename...> struct TupleBase;

template <size_t... indices, typename... Args>
struct TupleBase<std::index_sequence<indices...>, Args...>
 : TupleData<Args,indices>...
{
  template<typename ... Args2>
  constexpr explicit TupleBase(Args2 && ... args)
   : TupleData<Args,indices>(std::forward<Args2>(args))...
  {
     static_assert(sizeof...(Args2) == sizeof...(Args));
  }
   
  TupleBase() = default;
};

template <typename... Args>
using Index_Sequence = decltype(std::index_sequence_for<Args...>());

template<typename ... T>
struct Tuple : TupleBase<Index_Sequence<T...>, T...>
{
  using TupleBase<Index_Sequence<T...>, T...>::TupleBase;
  
};



///
/// End tuple implemenation
///

///
/// Free `get` functions
/// We cut the number of versions in 1/2 by using constexpr if
///

template<std::size_t Idx, typename T, bool Empty>
constexpr T& get(TupleData<T, Idx, Empty> &t) { if constexpr (Empty) { return t; } else { return t.t; } }

template<typename T, std::size_t Idx, bool Empty>
constexpr T& get(TupleData<T, Idx, Empty> &t) { if constexpr (Empty) { return t; } else { return t.t; } }

template<std::size_t Idx, typename T, bool Empty>
constexpr const T& get(const TupleData<T, Idx, Empty> &t) { if constexpr (Empty) { return t; } else { return t.t; } }

template<typename T, std::size_t Idx, bool Empty>
constexpr const T& get(const TupleData<T, Idx, Empty> &t) { if constexpr (Empty) { return t; } else { return t.t; } }

template<std::size_t Idx, typename T, bool Empty>
constexpr T && get(TupleData<T, Idx, Empty> &&t) { if constexpr (Empty) { return std::move(t); } else { return std::move(t.t); } }

template<typename T, std::size_t Idx, bool Empty>
constexpr T && get(TupleData<T, Idx, Empty> &&t) { if constexpr (Empty) { return std::move(t); } else { return std::move(t.t); } }

template<std::size_t Idx, typename T, bool Empty>
constexpr const T && get(const TupleData<T, Idx, Empty> &&t) { if constexpr (Empty) { return std::move(t); } else { return std::move(t.t); } }

template<typename T, std::size_t Idx, bool Empty>
constexpr const T && get(const TupleData<T, Idx, Empty> &&t) { if constexpr (Empty) { return std::move(t); } else { return std::move(t.t); } }



///
/// tuple_cat impl
///

/// concat two Tuples together, implementation
/// moves each element out into the new tuple
///
/// NOTE: The [[maybe_unused]] is necessary because of a seeming bug in gcc7
template<typename ... T, typename ... T2, size_t ... Indexes, size_t ... Indexes2>
  auto concat(Tuple<T...> t, [[maybe_unused]] Tuple<T2...> t2, 
              std::index_sequence<Indexes...>, 
              std::index_sequence<Indexes2...>)
{
  return Tuple<T..., T2...>(get<Indexes>(std::move(t))..., get<Indexes2>(std::move(t2))...);
}

/// concat two Tuples together, marshals to previous implementation
template<typename ... T, typename ... T2>
  constexpr auto concat(Tuple<T...> t, Tuple<T2...> t2)
{
  return concat(std::move(t), std::move(t2), std::index_sequence_for<T...>(), std::index_sequence_for<T2...>());
}

/// provide a handy operator<< that calls concat.
///
/// WHY? Because C++17
template<typename ... T, typename ... T2>
constexpr auto operator<<(Tuple<T...> t, Tuple<T2...> t2)
{
  return concat(std::move(t), std::move(t2));
}


/// The actual tuple_cat implementation
/// 
/// Abuses C++17's fold expressions to generate the final type desired,
/// we move in each tuple and RTO takes care of the rest
template<typename ... T>
constexpr auto tuple_cat(T && ... t)
{
  return ( std::forward<T>(t) << ... );
}

template<typename ... T1, typename ... T2, size_t ... Idx, typename Op>
constexpr bool lexigraphical_compare(const Tuple<T1...> &t1, const Tuple<T2...> &t2, std::index_sequence<Idx...>, Op &&op)
{
  enum Result {
    Maybe = 0, // make Maybe 0 for more efficient comparisons during folding
    True,
    False
  };    

  
  // This is possible because of C++17's constexpr lambda support
  auto comp_func = [&op](const auto &lhs, const auto &rhs) {
    if (op(lhs, rhs)) return True;
    if (op(rhs, lhs)) return False;
    return Maybe;
  };
  
  Result r = Maybe;
  
  // C++17 fold, short circuiting on the first "not maybe" result
  ( ( (r = comp_func(get<Idx>(t1), get<Idx>(t2))) == Maybe) && ... );
  
  return r == True;
}

template<typename ... T1, typename ... T2>
constexpr bool operator<(const Tuple<T1...> &t1, const Tuple<T2...> &t2)
{
  // C++17's static_assert that does not require a description
  static_assert(sizeof...(T1) == sizeof...(T2));
  // C++17's 
  return lexigraphical_compare(t1, t2, std::index_sequence_for<T1...>(),
                               [](const auto &lhs, const auto &rhs){
                                 return lhs < rhs;
                               });
}

template<typename ... T1, typename ... T2>
constexpr bool operator>(const Tuple<T1...> &t1, const Tuple<T2...> &t2)
{
  static_assert(sizeof...(T1) == sizeof...(T2));
  return lexigraphical_compare(t1, t2, std::index_sequence_for<T1...>(),
                               [](const auto &lhs, const auto &rhs){
                                 return lhs > rhs;
                               });
}



namespace std
{
  //template<typename ... T>
  //struct tuple_size;
  
  // 8.5.3 - we have to actually make our tuple_size available in the `std` namespace
  template< class... Types >
  struct tuple_size< Tuple<Types...> >
    : public std::integral_constant<std::size_t, sizeof...(Types)> { };
  
  // The const/volatile versions are provided by the standard implementation
  template< std::size_t I, class... Types >
  struct tuple_element< I, Tuple<Types...> > {
    Tuple<Types...> *p;
    typedef std::decay_t<decltype(get<I>(*p))> type;
  };
};

// Type that is not default constructible for testing
struct S
{
  S(int) {}
  static_assert(!std::is_default_constructible<S>());
};


int main()
{
  Tuple<int, double, float, std::unique_ptr<int>, S, long> t(1,2,3, nullptr, 2, 1000);
  get<0>(t) = 5;
  get<double>(t) = 3.3;
  
  const Tuple<int, int> t2{};
  [[maybe_unused]] auto copy = t2;

  get<2>(std::move(t));
  
  //auto ptr = std::make_unique<int>(10);
  //Tuple<std::unique_ptr<int>> t3(std::move(ptr));
  
  //const Tuple<int,int> t4 = t2;
  Tuple<const Tuple<int,int> &> t4(t2);
  
  [[maybe_unused]] auto t5 = tuple_cat(t2, t2, t2, std::move(t));
  
  constexpr Tuple<int, double, int> t10(1,2.3,4);
  constexpr Tuple<int, double, int> t11(1,2.3,5);
  static_assert(t10 < t11);
  static_assert(!(t11 < t10));

  static_assert(t11 > t10);
  static_assert(!(t10 > t11));
  
  static_assert(std::tuple_size<decltype(t10)>::value == 3);
  
  auto [a,b,c] = t10;
  return get<0>(t) + get<1>(t) + get<float>(t) + get<0>(t2) + get<11>(t5);
}


