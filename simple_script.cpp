#include <functional>
#include <array>
#include <memory>
#include <iostream>
#include <map>
#include <type_traits>
#include <cassert>

/// Helpers

template<typename T>
struct Array_View
{
  template<size_t Size>
  explicit constexpr Array_View(std::array<T, Size> &a) noexcept
    : m_data(a.begin()), m_end(a.end())
  {
  }

  constexpr T *begin() noexcept {
    return *m_data;
  }

  constexpr const T *begin() const noexcept {
    return *m_data;
  }

  constexpr T *end() noexcept {
    return *m_end;
  }

  constexpr const T *end() const noexcept {
    return *m_end;
  }

  constexpr const T &operator[](size_t Index) const noexcept {
    return m_data[Index];
  }

  constexpr T &operator[](size_t Index) noexcept {
    return m_data[Index];
  }

  T *m_data = nullptr;
  T *m_end = nullptr;
};



struct Any
{
  struct Any_Impl
  {
    virtual void *data() = 0;
    virtual const void *c_data() = 0;
    virtual volatile void *v_data() = 0;
    virtual const volatile void *cv_data() = 0;
    Any_Impl(Any_Impl &&) = delete;
    Any_Impl(const Any_Impl &) = delete;
  };

  template<typename T>
  struct Any_Impl_Detail : Any_Impl
  {
    constexpr const void *c_data() noexcept final {
      if constexpr(std::is_volatile_v<T>) {
        return nullptr;
      } else {
        return &m_data;
      }
    }

    constexpr void *data() noexcept final {
      if constexpr (std::is_const_v<T> || std::is_volatile_v<T>) {
        return nullptr;
      } else {
        return &m_data;
      }
    }

    constexpr volatile void *v_data() noexcept final {
      if constexpr (std::is_const_v<T>) {
        return nullptr;
      } else {
        return &m_data;
      }
    }

    constexpr const volatile void *cv_data() noexcept final {
      return &m_data;
    }

    constexpr Any_Impl_Detail(const T &t)
      noexcept(std::is_nothrow_copy_constructible_v<T>)
      : m_data(t)
    {
    }

    constexpr Any_Impl_Detail(T &&t)
      noexcept(std::is_nothrow_move_constructible_v<T>)
      : m_data(std::move(t))
    {
    }

    T m_data;
  };

  template<typename T>
  struct Any_Impl_Detail<T &> : Any_Impl
  {
    constexpr const void *c_data() noexcept final {
      if constexpr(std::is_volatile_v<T>) {
        return nullptr;
      } else {
        return &m_data;
      }
    }

    constexpr void *data() noexcept final {
      if constexpr (std::is_const_v<T> || std::is_volatile_v<T>) {
        return nullptr;
      } else {
        return &m_data;
      }
    }

    constexpr volatile void *v_data() noexcept final {
      if constexpr (std::is_const_v<T>) {
        return nullptr;
      } else {
        return &m_data;
      }
    }

    constexpr const volatile void *cv_data() noexcept final {
      return &m_data;
    }


    constexpr Any_Impl_Detail(T &t) noexcept
      : m_data(t)
    {
    }

    T &m_data;
  };


  template<typename T>
  Any(Any_Impl_Detail<T> detail)
    : data(std::make_unique<Any_Impl_Detail<T>>(std::move(detail)))
  {
  }

  std::unique_ptr<Any_Impl> data;
};


template<>
struct Any::Any_Impl_Detail<void> : Any_Impl
{
  constexpr const void *c_data() noexcept final {
    return nullptr;
  }

  constexpr void *data() noexcept final {
    return nullptr;
  }

  constexpr volatile void *v_data() noexcept final {
    return nullptr;
  }

  constexpr const volatile void *cv_data() noexcept final {
    return nullptr;
  };

  constexpr Any_Impl_Detail()
  {
  }
};


template<typename T>
constexpr auto make_any(T &&t)
{
  return Any::Any_Impl_Detail<std::decay_t<T>>{std::forward<T>(t)};
}

template<typename T>
constexpr auto forward_as_any(T &&t)
{
  return Any::Any_Impl_Detail<T &&>{std::forward<T>(t)};
}

template<typename T>
constexpr auto return_any(T t)
{
  if constexpr (!std::is_reference_v<T>) {
    return Any::Any_Impl_Detail<T>{std::move(t)};
  } else {
    return Any::Any_Impl_Detail<T>{t};
  }
}

constexpr auto return_any_void()
{
  return Any::Any_Impl_Detail<void>{};
}


template<typename Result>
constexpr decltype(auto) cast(Any::Any_Impl &value)
{
  if constexpr(std::is_const_v<Result>) {
    return (*static_cast<std::remove_reference_t<Result> *>(value.c_data()));
  } else {
    return (*static_cast<std::remove_reference_t<Result> *>(value.data()));
  }
}


template<typename Result>
constexpr decltype(auto) cast(const Any &value)
{
  return cast<Result>(*value.data.get());
}


/// begin impl

struct [[nodiscard]] Callable_Results : std::pair<bool, Any>
{
  bool succeeded() const noexcept { return first; }
};

template<typename ... Param>
struct Function_Params
{
};

template<typename Ret, typename Params, bool IsNoExcept = false, bool IsMemberObject = false, bool IsObject = false>
struct Function_Signature {
  using Param_Types = Params;
  using Return_Type = Ret;
  constexpr static const bool is_object = IsObject;
  constexpr static const bool is_member_object = IsMemberObject;
  constexpr static const bool is_noexcept = IsNoExcept;
  template<typename T>
  constexpr Function_Signature(T &&) noexcept {}
  constexpr Function_Signature() noexcept = default;
};

// Free functions

template<typename Ret, typename ... Param>
Function_Signature(Ret (*f)(Param...)) -> Function_Signature<Ret, Function_Params<Param...>>;

template<typename Ret, typename ... Param>
Function_Signature(Ret (*f)(Param...) noexcept) -> Function_Signature<Ret, Function_Params<Param...>, true>;

// no reference specifier

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile) -> Function_Signature<Ret, Function_Params<volatile Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile noexcept) -> Function_Signature<Ret, Function_Params<volatile Class &, Param...>, true>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile const) -> Function_Signature<Ret, Function_Params<volatile const Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile const noexcept) -> Function_Signature<Ret, Function_Params<volatile const Class &, Param...>, true>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) ) -> Function_Signature<Ret, Function_Params<Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) noexcept) -> Function_Signature<Ret, Function_Params<Class &, Param...>, true>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) const) -> Function_Signature<Ret, Function_Params<const Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) const noexcept) -> Function_Signature<Ret, Function_Params<const Class &, Param...>, true>;

// & reference specifier

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile &) -> Function_Signature<Ret, Function_Params<volatile Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile & noexcept) -> Function_Signature<Ret, Function_Params<volatile Class &, Param...>, true>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile const &) -> Function_Signature<Ret, Function_Params<volatile const Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile const & noexcept) -> Function_Signature<Ret, Function_Params<volatile const Class &, Param...>, true>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) & ) -> Function_Signature<Ret, Function_Params<Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) & noexcept) -> Function_Signature<Ret, Function_Params<Class &, Param...>, true>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) const &) -> Function_Signature<Ret, Function_Params<const Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) const & noexcept) -> Function_Signature<Ret, Function_Params<const Class &, Param...>, true>;

// && reference specifier

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile &&) -> Function_Signature<Ret, Function_Params<volatile Class &&, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile && noexcept) -> Function_Signature<Ret, Function_Params<volatile Class &&, Param...>, true>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile const &&) -> Function_Signature<Ret, Function_Params<volatile const Class &&, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile const && noexcept) -> Function_Signature<Ret, Function_Params<volatile const Class &&, Param...>, true>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) &&) -> Function_Signature<Ret, Function_Params<Class &&, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) && noexcept) -> Function_Signature<Ret, Function_Params<Class &&, Param...>, true>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) const &&) -> Function_Signature<Ret, Function_Params<const Class &&, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) const && noexcept) -> Function_Signature<Ret, Function_Params<const Class &&, Param...>, true>;

template<typename Ret, typename Class>
Function_Signature(Ret (Class::*f)) -> Function_Signature<Ret, Function_Params<Class &>, true, true>;

template<typename Func>
Function_Signature(Func &&) -> Function_Signature<
    typename decltype(Function_Signature{&std::decay_t<Func>::operator()})::Return_Type,
    typename decltype(Function_Signature{&std::decay_t<Func>::operator()})::Param_Types,
    decltype(Function_Signature{&std::decay_t<Func>::operator()})::is_noexcept,
    false,
    true
  >;

  
struct GenericCallable
{
  GenericCallable(std::function<Any (Array_View<Any::Any_Impl *>)> func)
    : caller(std::move(func))
      // todo do I need to specify noexcept here? how is it determined?
  {
  }

  std::function<Any (Array_View<Any::Any_Impl *>)> caller;
};

template<bool Is_Noexcept, typename Func, typename ... Param, size_t ... Indexes>
constexpr static decltype(auto) my_invoke(Func &f, [[maybe_unused]] Array_View<Any::Any_Impl *> params, std::index_sequence<Indexes...>) 
  noexcept(Is_Noexcept)
{
  return (std::invoke(std::forward<Func>(f), cast<Param>(*params[Indexes])...));
};


template<typename Func, bool Is_Noexcept, bool Is_MemberObject, bool Is_Object, typename Ret, typename ... Param>
auto make_callable_impl(Func &&func, Function_Signature<Ret, Function_Params<Param...>, Is_Noexcept, Is_MemberObject, Is_Object>)
{
  return [func = std::forward<Func>(func)](Array_View<Any::Any_Impl *> params) mutable noexcept(Is_Noexcept) {
    if constexpr(std::is_same_v<Ret, void>) {
      my_invoke<Is_Noexcept, decltype(func), Param...>(func,  params, std::index_sequence_for<Param...>());
      return return_any_void();
    } else {
      return return_any<Ret>(my_invoke<Is_Noexcept, decltype(func), Param...>(func,  params, std::index_sequence_for<Param...>()));
    }
  };
}

// this version peels off the function object itself from the function signature, when used
// on a callable object
template<typename Func, typename Ret, typename Object, typename ... Param, bool Is_Noexcept>
auto make_callable(Func &&func, Function_Signature<Ret, Function_Params<Object, Param...>, Is_Noexcept, false, true>)
{
  return make_callable_impl(std::forward<Func>(func), Function_Signature<Ret, Function_Params<Param...>, Is_Noexcept, false, true>{});
} 

template<typename Func, typename Ret, typename ... Param, bool Is_Noexcept>
auto make_callable(Func &&func, Function_Signature<Ret, Function_Params<Param...>, Is_Noexcept, false, false> fs)
{
  return make_callable_impl(std::forward<Func>(func), fs);
}

template<typename Func>
auto make_callable(Func &&func)
{
  return make_callable(std::forward<Func>(func), Function_Signature{func});
}


int my_func(int i, int j) noexcept
{
  return i + j;
}

void my_func_2()
{
}

struct Int
{
  Int(const int i)
    : m_val(i)
  {
  }

  int add(int j) const {
    return m_val + j;
  }

  int add2(double k) const volatile noexcept {
    return m_val * 2 + k;
  }

  int &val() { return m_val; }

  const int &val2() { return m_val; }

  int m_val;
};

struct Dispatcher
{
  std::vector<GenericCallable> callables;
};

struct Simple_Script
{
  std::map<std::string, Dispatcher> dispatchers;
};



int main()
{
//  Simple_Script script;
//  script.dispatchers["func"].callables.emplace_back(&my_func);
//  script.dispatchers["add"].callables.emplace_back(&Int::add);
//  script.dispatchers["add"].callables.emplace_back(&Int::add2);
//  script.dispatchers["lambda"].callables.emplace_back(&my_func);
//  script.dispatchers["add"].callables.emplace_back([](double d, double d2) { return d + d2; });

  GenericCallable callable4{make_callable(&Int::val2)};
  GenericCallable callable3{make_callable(&Int::val)};
  GenericCallable callable2{make_callable([]{return std::make_unique<int>(42); })};
  GenericCallable callable{make_callable([](double d, double d2) noexcept { return d + d2; })};

  auto func2 = make_callable(&my_func_2);

  int i=2;
  auto val = make_any(i);

  static_assert(!std::is_reference_v<decltype(val.m_data)>);

  auto val2 = forward_as_any(i);

  static_assert(std::is_reference_v<decltype(val2.m_data)>);


  static_assert(Function_Signature{[](double d, double d2) noexcept { return d + d2; }}.is_noexcept);
  static_assert(!Function_Signature{[](double d, double d2) { return d + d2; }}.is_noexcept);

  //return std::invoke([](double d, double d2) { return d + d2;}, 5.2, 6.4);

  std::tuple t{make_any(1.6), make_any(2.8)};
  std::array<Any::Any_Impl *, 2> params{&std::get<0>(t), &std::get<1>(t)};
  return cast<int>(callable.caller(Array_View{params}));
//  std::cout << cast<int>(callable.caller(Array_View{params}).second) << '\n';

//  std::array<Any, 2> params2{Int(2), 4};
//  std::cout << cast<int>(callable2.caller(Array_View{params2}).second) << '\n';

//  std::cout << cast<int>(callable4.caller(Array_View{params2}).second) << '\n';
}


