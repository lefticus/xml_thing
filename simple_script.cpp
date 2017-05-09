#include <functional>
#include <array>
#include <memory>
#include <iostream>
#include <map>
#include <type_traits>

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
  };

  template<typename T>
  struct Any_Impl_Detail : Any_Impl
  {
    void *data() noexcept override {
      return &m_data;
    }

    // is this correct? What if the thing is not moveable and this move reverts to a copy?
    constexpr Any_Impl_Detail(T data) noexcept((std::is_move_constructible_v<T>
                                                && std::is_nothrow_move_constructible_v<T>)
                                               || (!std::is_move_constructible_v<T>
                                                   && std::is_nothrow_copy_constructible_v<T>))
      : m_data(std::move(data)) {
    }

    T m_data;
  };


  template<typename T>
  Any(Any_Impl_Detail<T> detail)
    : data(std::make_unique<Any_Impl_Detail<T>>(std::move(detail)))
  {
  }

  std::unique_ptr<Any_Impl> data;
};

template<typename T>
constexpr auto make_any(T &&t)
{
  return Any::Any_Impl_Detail<T>(std::forward<T>(t));
}

template<typename Result>
constexpr decltype(auto) cast(Any::Any_Impl &value)
{
  if constexpr(std::is_lvalue_reference_v<Result>) {
    return (*static_cast<std::remove_reference_t<Result> *>(value.data()));
  } else {
    return (*static_cast<Result *>(value.data()));
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
  template<bool Is_Noexcept, typename Func, typename ... Param, size_t ... Indexes>
    constexpr static decltype(auto) my_invoke(Func &f, [[maybe_unused]] Array_View<Any::Any_Impl *> params, std::index_sequence<Indexes...>) noexcept(Is_Noexcept)
    {
      return (std::invoke(std::forward<Func>(f), cast<Param>(*params[Indexes])...));
    };

  struct Tag
  {
  };

  template<typename Func, bool Is_Noexcept, bool Is_MemberObject, bool Is_Object, typename Ret, typename ... Param>
  GenericCallable(Func &&func, Function_Signature<Ret, Function_Params<Param...>, Is_Noexcept, Is_MemberObject, Is_Object>, Tag)
    : caller (
        [func = std::forward<Func>(func)](Array_View<Any::Any_Impl *> params) mutable noexcept(Is_Noexcept) {
        return std::pair<bool, Any>(
            true,
            make_any(my_invoke<Is_Noexcept, decltype(func), Param...>(func,  params, std::index_sequence_for<Param...>()))
            );
        }
        )
    {}

  template<typename Func, typename Ret, typename Object, typename ... Param, bool Is_Noexcept>
  GenericCallable(Func &&func, Function_Signature<Ret, Function_Params<Object, Param...>, Is_Noexcept, false, true>)
    : GenericCallable(std::forward<Func>(func), Function_Signature<Ret, Function_Params<Param...>, Is_Noexcept, false, true>{}, Tag{})
  {}

  template<typename Func, typename Ret, typename ... Param, bool Is_Noexcept>
  GenericCallable(Func &&func, Function_Signature<Ret, Function_Params<Param...>, Is_Noexcept, false, false> fs)
    : GenericCallable(std::forward<Func>(func), fs, Tag{})
  {}

  template<typename Func, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Func>, GenericCallable>>>
  GenericCallable(Func &&f) 
    : GenericCallable(std::forward<Func>(f), Function_Signature{f})
  {
  }

  std::function<std::pair<bool, Any> (Array_View<Any::Any_Impl *>)> caller;
};





int my_func(int i, int j) noexcept
{
  return i + j;
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

  GenericCallable callable2{[]{return std::make_unique<int>(42); }};
  GenericCallable callable{[](double d, double d2) noexcept { return d + d2; }};

  static_assert(Function_Signature{[](double d, double d2) noexcept { return d + d2; }}.is_noexcept);
  static_assert(!Function_Signature{[](double d, double d2) { return d + d2; }}.is_noexcept);

  //return std::invoke([](double d, double d2) { return d + d2;}, 5.2, 6.4);

  std::tuple t{make_any(1.6), make_any(2.8)};
  std::array<Any::Any_Impl *, 2> params{&std::get<0>(t), &std::get<1>(t)};
  return cast<int>(callable.caller(Array_View{params}).second);
//  std::cout << cast<int>(callable.caller(Array_View{params}).second) << '\n';

//  std::array<Any, 2> params2{Int(2), 4};
//  std::cout << cast<int>(callable2.caller(Array_View{params2}).second) << '\n';

//  std::cout << cast<int>(callable4.caller(Array_View{params2}).second) << '\n';
}


