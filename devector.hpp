#pragma once
#include <cassert>
#include <memory>
#include <cstring> 

namespace cwt {
  namespace detail {
    template<typename T>
    void relocate(T* first1, T* last1, T* first2, T*, std::true_type) {
      std::memmove(first2, first1, (last1 - first1)*sizeof(T));
    }

    template<typename T>
    void relocate(T* first1, T* last1, T* first2, T* last2, std::false_type) {
      if(first2 < first1) {
        assert(first2 < first1);

        static_assert(std::is_nothrow_move_constructible<T>(), "T must be nothrow_move_constructible");
        for (auto ptr = first2; ptr != last2; ++ptr, ++first1) {
          new (ptr) T{ std::move(*first1) };
          first1->~T();
        }
      }
      else {
        assert(first2 > first1);

        static_assert(std::is_nothrow_move_constructible<T>(), "T must be nothrow_move_constructible");
        --last1;
        for (auto ptr = last2 - 1; ptr >= first2; --ptr, --last1) {
          new (ptr) T{ std::move(*last1) };
          last1->~T();
        }
      }
    }

    template<typename T>
    void relocate(T* first1, T* last1, T* first2, T* last2) {
      relocate(first1, last1, first2, last2, std::is_trivial<T>());
    }

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

	template<typename T>
	class devector{

    struct buffer_deleter {
      void operator()(T* ptr) {
        operator delete(ptr);
      }
    };

    using buffer_ptr = std::unique_ptr<T, buffer_deleter>;

  public:
    devector() {}

    devector(devector&& o) :
      m_buffer(std::move(o.m_buffer)),
      m_begin(o.m_begin),
      m_end(o.m_end),
      m_buffer_end(o.m_end)
    {
      o.m_begin = nullptr;
      o.m_end = nullptr;
      o.m_buffer_end = nullptr;
    }

    devector& operator=(devector&& o) {
      m_buffer = std::move(o.m_buffer);
      m_begin = o.m_begin;
      m_end = o.m_end;
      m_buffer_end = o.m_end;
      o.m_begin = nullptr;
      o.m_end = nullptr;
      o.m_buffer_end = nullptr;

      return *this;
    }

    ~devector() {
      for (; m_begin != m_end; ++m_begin) {
        m_begin->~T();
      }
    }

    size_t size() const noexcept {
      return m_end - m_begin;
    }

    size_t capacity() const noexcept {
      return m_buffer_end - m_buffer.get();
    }

    void push_back(T val) {
      assert(m_end <= m_buffer_end);
      if (m_end == m_buffer_end) {
        shift_or_grow();
      }

      assert(m_end < m_buffer_end);
      new (m_end) T{ std::move(val) };
      ++m_end;
    }

    void push_front(T val) {
      assert(m_begin >= m_buffer.get());
      if (m_begin == m_buffer.get()) {
        shift_or_grow();
      }

      assert(m_begin > m_buffer.get());
      new (m_begin - 1) T{ std::move(val) };
      --m_begin;
    }

    T* begin() noexcept {
      return m_begin;
    }

    T* end() noexcept {
      return m_end;
    }

    T& operator[](size_t pos) noexcept {
      assert(pos < size());
      return *(m_begin + pos);
    }

  private:
    
    void shift_or_grow() {
      if (size() / static_cast<double>(capacity()) < reallocation_limit
        && capacity() - size() >= 2)
      {
        shift();
      }
      else {
        grow();
      }
    }

    void shift() {
      auto left_gap = calculate_left_gap_size(capacity());

      auto new_begin = m_buffer.get() + left_gap;
      auto new_end = new_begin + size();

      assert(new_begin != m_begin);

      detail::relocate(m_begin, m_end, new_begin, new_end);

      m_begin = new_begin;
      m_end = new_end;

      m_prev_begin = m_begin;
      m_prev_end = m_end;
    }

    void grow() {
      auto new_cap = static_cast<size_t>(capacity() * growth_factor + 2);
      auto new_buffer = buffer_ptr(static_cast<T*>(operator new (new_cap * sizeof(T))));

      auto left_gap = calculate_left_gap_size(new_cap);

      auto new_begin = new_buffer.get() + left_gap;
      auto new_end = new_begin + size();
      auto new_buffer_end = new_buffer.get() + new_cap;

      std::uninitialized_copy(begin(), end(), new_begin);
      detail::destroy_range(m_begin, m_end);

      m_buffer = std::move(new_buffer);
      m_buffer_end = new_buffer_end;
      m_begin = new_begin;
      m_end = new_end;

      m_prev_begin = m_begin;
      m_prev_end = m_end;
    }

    size_t calculate_left_gap_size(size_t new_cap) {
      assert(m_begin == m_buffer.get() || m_end == m_buffer_end);
      
      auto inserted_front = std::max(0ll, m_prev_begin - m_begin);
      auto inserted_back = std::max(0ll, m_end - m_prev_end);
      auto inserted_total = inserted_front + inserted_back;

      assert(inserted_total > 0);

      auto left_gap_percent = .1 + .8 * (inserted_front / (double)inserted_total);

      assert(left_gap_percent < .91);
      assert(left_gap_percent > .09);

      size_t left_gap = left_gap_percent * (new_cap - size());

      left_gap = std::max(1ull, left_gap);
      left_gap = std::min(new_cap - size() - 1ull, left_gap);

      assert(left_gap > 0);
      assert(new_cap - size() - left_gap > 0);

      return left_gap;
    }

    static constexpr double reallocation_limit = 0.88;
    static constexpr double growth_factor = 1.7;

    buffer_ptr m_buffer;
    T* m_buffer_end = nullptr;
    T* m_begin = nullptr;
    T* m_end = nullptr;

    T* m_prev_begin = nullptr;
    T* m_prev_end = nullptr;
  };
}