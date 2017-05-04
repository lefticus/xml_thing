#include <functional>
#include <array>
#include <memory>
#include <iostream>

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


template<typename Ret, typename ... Param>
struct Function_Signature {
  template<typename T>
  Function_Signature(T &&) {}
};

template<typename Ret, typename ... Param>
Function_Signature(Ret (*f)(Param...)) -> Function_Signature<Ret, Param...>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...)) -> Function_Signature<Ret, Class &, Param...>;

template<typename Ret, typename Class, typename ... Param>
Function_Signature(Ret (Class::*f)(Param ...) const) -> Function_Signature<Ret, const Class &, Param...>;

template<typename Func>
Function_Signature(Func &&) -> Function_Signature<decltype(Function_Signature{&std::decay_t<Func>::operator()})>;

  
struct GenericCallable
{
  template<typename Func, typename ... Param, size_t ... Indexes>
    static auto my_invoke(Func &f, Array_View<Any> params, std::index_sequence<Indexes...>)
    {
      return std::invoke(std::forward<Func>(f), cast<Param>(params[Indexes])...);
    };



  template<typename Func, typename Ret, typename ... Param>
  GenericCallable(Func &&func, Function_Signature<Ret, Param...>)
    : caller (
        [func = std::forward<Func>(func)](Array_View<Any> params) mutable {
        return std::pair<bool, Any>(
            true,
            Any(my_invoke<decltype(func), Param...>(func,  params, std::index_sequence_for<Param...>()))
            );
        }
        )
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

  int m_val;
};

int main()
{
  GenericCallable callable(&my_func);
  GenericCallable callable2(&Int::add);
  GenericCallable callable3([](){ return 1; });

  std::array<Any, 2> params{1,2};
  std::cout << cast<int>(callable.caller(Array_View{params}).second) << '\n';

  std::array<Any, 2> params2{Int(2), 4};
  std::cout << cast<int>(callable2.caller(Array_View{params2}).second) << '\n';
}


