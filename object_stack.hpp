#include <cstddef>
#include <functional>
#include <iostream>
#include <stdexcept>

struct S {
  S() { puts("S()"); }
  S(S &&) { puts("S(S&&)"); }
  ~S() { puts("~S()"); }
};

template <std::size_t Size = 8192, std::size_t MaxEntries = 24>
struct Object_Stack {
  alignas(std::max_align_t) char storage[Size]{};

  Object_Stack() = default;
  Object_Stack(const Object_Stack &) = delete;
  Object_Stack &operator=(const Object_Stack &) = delete;
  Object_Stack &operator=(Object_Stack &&) = delete;
  Object_Stack(Object_Stack &&) = delete;

  ~Object_Stack() { clear(); }

  struct Entry {
    using Deleter = void (*)(void *);

    std::size_t offset{0};
    std::size_t size{0};
    const std::type_info *type{nullptr};
    Deleter deleter{};
  };

  std::array<Entry, MaxEntries> entries;
  std::size_t num_entries{0};

  template <typename T> std::size_t get_next_offset() const noexcept {
    if (num_entries == 0) {
      return 0;
    } else {
      const auto next_byte =
          entries[num_entries - 1].offset + entries[num_entries - 1].size;
      const auto remainder = next_byte % alignof(T);
      if (remainder == 0) {
        return next_byte;
      } else {
        return (next_byte / alignof(T) * alignof(T)) + alignof(T);
      }
    }
  }

  template <typename T> void push_back(T &&obj) {
    if (num_entries == MaxEntries) {
      throw std::runtime_error("Max entries reached");
    }

    const auto offset = get_next_offset<T>();

    if (offset + sizeof(T) > Size) {
      throw std::runtime_error("Size exceeded");
    }

    typename Entry::Deleter deleter = []() -> void (*)(void *) {
      if constexpr (std::is_trivially_destructible_v<T>) {
        return nullptr;
      } else {
        return [](void *ptr) { static_cast<T *>(ptr)->~T(); };
      }
    }();

    entries[num_entries] = Entry{offset, sizeof(T), &typeid(T), deleter};

    new (&storage[offset]) T{std::forward<T>(obj)};

    ++num_entries;
  }

  [[nodiscard]] constexpr auto size() noexcept { return num_entries; }
  [[nodiscard]] constexpr bool empty() noexcept { return num_entries == 0; }

  constexpr void pop_back() {
    if (entries[num_entries - 1].deleter != nullptr) {
      (entries[num_entries - 1].deleter)(
          &storage[entries[num_entries - 1].offset]);
    }
    --num_entries;
  }

  constexpr void clear() {
    while (!empty()) {
      pop_back();
    }
  }

  template <typename T> const T &get(const std::size_t entry) const {
    if (typeid(T) == *entries[entry].type) {
      return reinterpret_cast<T &>(storage[entries[entry].offset]);
    } else {
      throw std::range_error("Type Mismatch");
    }
  }

  template <typename T> T &get(const std::size_t entry) {
    if (typeid(T) == *entries[entry].type) {
      return reinterpret_cast<T &>(storage[entries[entry].offset]);
    } else {
      throw std::range_error("Type Mismatch");
    }
  }

  template <typename T> T &&move(const std::size_t entry) {
    if (typeid(T) == *entries[entry].type) {
      return std::move(reinterpret_cast<T &>(storage[entries[entry].offset]));
    } else {
      throw std::range_error("Type Mismatch");
    }
  }

  [[nodiscard]] constexpr auto begin() const noexcept {
    return std::cbegin(entries);
  }
  [[nodiscard]] constexpr auto end() const noexcept {
    return std::next(std::cbegin(entries), num_entries);
  }
};

int main() {
  Object_Stack c;
  c.push_back(std::string{"Hello World"});
  c.push_back(std::string{"Hello World"});
  c.push_back(std::string{"Hello World"});
  c.push_back(std::string{"Hello World"});
  c.push_back(std::string{"Hello World"});
  c.push_back(std::string{"Hello World"});
  c.push_back(std::string{"Hello World"});
  c.push_back('a');
  c.push_back('b');
  c.push_back('c');
  c.push_back(1);
  c.push_back(S{});
  c.push_back(std::uint16_t{2});
  c.push_back(std::uint16_t{2});
  c.push_back(std::string{"Hello World"});

  for (const auto &entry : c) {
    std::cout << '(' << entry.type->name() << ')' << entry.offset << " -> "
              << (entry.offset + entry.size - 1) << '\n';
  }

  // c.push_back(2.3);
  // c.get<int>(0) = 35;
  //    auto s = c.move<S>(1);
  puts("here");
  // c.get<double>(2) = 19.5;
}
