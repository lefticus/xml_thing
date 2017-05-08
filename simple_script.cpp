#include <functional>
#include <array>
#include <memory>
#include <iostream>
#include <map>

/// Helpers

template<typename T>
struct Array_View
{
  template<size_t Size>
  explicit constexpr Array_View(std::array<T, Size> &a)
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
    void *data() override {
      return &m_data;
    }
    Any_Impl_Detail(T data) : m_data(std::move(data)) {
    }

    T m_data;
  };


  template<typename T, typename = std::enable_if_t<!std::is_same_v<T, Any>>>
  Any(T &&t)
    : data(std::make_unique<Any_Impl_Detail<T>>(std::forward<T>(t)))
  {
  }

  std::unique_ptr<Any_Impl> data;
};

template<typename Result>
decltype(auto) cast(const Any &value)
{
  if constexpr(std::is_lvalue_reference_v<Result>) {
    return *static_cast<std::remove_reference_t<Result> *>(value.data->data());
  } else {
    return *static_cast<Result *>(value.data->data());
  }
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

template<typename Ret, typename Params, bool IsObject = false>
struct Function_Signature {
  using Param_Types = Params;
  using Return_Type = Ret;
  constexpr static const bool is_object = IsObject;
  template<typename T>
  Function_Signature(T &&) {}
  Function_Signature() {}
};

// Free functions

template<typename Ret, typename ... Param>
Function_Signature(Ret (*f)(Param...)) -> Function_Signature<Ret, Function_Params<Param...>>;

template<typename Ret, typename ... Param>
Function_Signature(Ret (*f)(Param...) noexcept) -> Function_Signature<Ret, Function_Params<Param...>>;

// no reference specifier

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile) -> Function_Signature<Ret, Function_Params<volatile Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile noexcept) -> Function_Signature<Ret, Function_Params<volatile Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile const) -> Function_Signature<Ret, Function_Params<volatile const Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile const noexcept) -> Function_Signature<Ret, Function_Params<volatile const Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) ) -> Function_Signature<Ret, Function_Params<Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) noexcept) -> Function_Signature<Ret, Function_Params<Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) const) -> Function_Signature<Ret, Function_Params<const Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) const noexcept) -> Function_Signature<Ret, Function_Params<const Class &, Param...>>;

// & reference specifier

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile &) -> Function_Signature<Ret, Function_Params<volatile Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile & noexcept) -> Function_Signature<Ret, Function_Params<volatile Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile const &) -> Function_Signature<Ret, Function_Params<volatile const Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile const & noexcept) -> Function_Signature<Ret, Function_Params<volatile const Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) & ) -> Function_Signature<Ret, Function_Params<Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) & noexcept) -> Function_Signature<Ret, Function_Params<Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) const &) -> Function_Signature<Ret, Function_Params<const Class &, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) const & noexcept) -> Function_Signature<Ret, Function_Params<const Class &, Param...>>;

// && reference specifier

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile &&) -> Function_Signature<Ret, Function_Params<volatile Class &&, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile && noexcept) -> Function_Signature<Ret, Function_Params<volatile Class &&, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile const &&) -> Function_Signature<Ret, Function_Params<volatile const Class &&, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) volatile const && noexcept) -> Function_Signature<Ret, Function_Params<volatile const Class &&, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) &&) -> Function_Signature<Ret, Function_Params<Class &&, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) && noexcept) -> Function_Signature<Ret, Function_Params<Class &&, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) const &&) -> Function_Signature<Ret, Function_Params<const Class &&, Param...>>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) const && noexcept) -> Function_Signature<Ret, Function_Params<const Class &&, Param...>>;


template<typename Func>
Function_Signature(Func &&) -> Function_Signature<
    typename decltype(Function_Signature{&std::decay_t<Func>::operator()})::Return_Type,
    typename decltype(Function_Signature{&std::decay_t<Func>::operator()})::Param_Types,
    true
  >;

  
struct GenericCallable
{
  template<typename Func, typename ... Param, size_t ... Indexes>
    static decltype(auto) my_invoke(Func &f, Array_View<Any> params, std::index_sequence<Indexes...>)
    {
      return (std::invoke(std::forward<Func>(f), cast<Param>(params[Indexes])...));
    };

  struct Tag
  {
  };

  template<typename Func, bool Is_Object, typename Ret, typename ... Param>
  GenericCallable(Func &&func, Function_Signature<Ret, Function_Params<Param...>, Is_Object>, Tag)
    : caller (
        [func = std::forward<Func>(func)](Array_View<Any> params) mutable {
        return std::pair<bool, Any>(
            true,
            Any(my_invoke<decltype(func), Param...>(func,  params, std::index_sequence_for<Param...>()))
            );
        }
        )
    {}

  template<typename Func, typename Ret, typename Object, typename ... Param>
  GenericCallable(Func &&func, Function_Signature<Ret, Function_Params<Object, Param...>, true>)
    : GenericCallable(std::forward<Func>(func), Function_Signature<Ret, Function_Params<Param...>, true>{}, Tag{})
  {}

  template<typename Func, typename Ret, typename ... Param>
  GenericCallable(Func &&func, Function_Signature<Ret, Function_Params<Param...>, false>)
    : GenericCallable(std::forward<Func>(func), Function_Signature<Ret, Function_Params<Param...>, false>{}, Tag{})
  {}

  template<typename Func, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Func>, GenericCallable>>>
  GenericCallable(Func &&f) 
    : GenericCallable(std::forward<Func>(f), Function_Signature{f})
  {
  }

  std::function<std::pair<bool, Any> (Array_View<Any>)> caller;
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
  Simple_Script script;
  script.dispatchers["func"].callables.emplace_back(&my_func);
  script.dispatchers["add"].callables.emplace_back(&Int::add);
  script.dispatchers["add"].callables.emplace_back(&Int::add2);
  script.dispatchers["lambda"].callables.emplace_back(&my_func);
  script.dispatchers["add"].callables.emplace_back([](double d, double d2) { return d + d2; });

  GenericCallable callable{[](double d, double d2) { return d + d2; }};

  //return std::invoke([](double d, double d2) { return d + d2;}, 5.2, 6.4);

//  std::array<Any, 2> params{1,2};
//  std::cout << cast<int>(callable.caller(Array_View{params}).second) << '\n';

//  std::array<Any, 2> params2{Int(2), 4};
//  std::cout << cast<int>(callable2.caller(Array_View{params2}).second) << '\n';

//  std::cout << cast<int>(callable4.caller(Array_View{params2}).second) << '\n';
}


