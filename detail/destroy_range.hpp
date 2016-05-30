#pragma once

namespace cwt {
  namespace detail {
    template<typename T>
    void destroy_range(T*, T*, std::true_type) { }

    template<typename T>
    void destroy_range(T* first, T* last, std::false_type) {
      for (; first != last; ++first) {
        first->~T();
      }
    }

    template<typename T>
    void destroy_range(T* first, T* last) {
      destroy_range(first, last, std::is_trivially_destructible<T>());
    }
  }
}