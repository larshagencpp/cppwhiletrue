#pragma once
#include "detail/buffer.hpp"
#include "detail/destroy_range.hpp"
#include "bitutils.hpp"
#include <cassert>
#include <vector>
#include <cstddef>

#ifdef NDEBUG
#define CWT_DEBUG_ONLY(...)
#else
#define CWT_DEBUG_ONLY(...) __VA_ARGS__
#endif

namespace cwt {
  template<typename T, typename A = std::allocator<T>>
  class stack {
    template<typename U>
    class iterator_templ;
  public:
    using iterator = iterator_templ<T>;
    using const_iterator = iterator_templ<const T>;
    using value_type = T;

    iterator begin() noexcept {
      auto array_index = get_array_index(0);
      if (current_begin == nullptr) {
        return{ *this, CWT_DEBUG_ONLY(0,) array_index, nullptr, nullptr, nullptr };
      }
      
      auto& first_array = *m_arrays.begin();
      return{ *this, CWT_DEBUG_ONLY(0,) array_index, first_array.begin(), first_array.begin(), first_array.end() };
    }

    iterator end() noexcept {
      return iterator{ *this, CWT_DEBUG_ONLY(size(),) get_array_index(size()) - (current_end != nullptr && current_end == current_array_end ? 1 : 0), current_begin, current_end, current_array_end };
    }

    const_iterator begin() const noexcept {
      return { *this, 0 };
    }

    iterator end() const noexcept {
      return { *this, size() };
    }

    size_t size() const noexcept {
      if(current_index < 0) {
        return 0;
      }

      auto current_uindex = static_cast<size_t>(current_index);
      auto bucket_size = ((1ull << (current_uindex + 1ull)) >> 1ull) - 1ull;
      assert(current_end >= current_begin);
      return bucket_size + static_cast<size_t>(current_end - current_begin);
    }

    T& operator[](size_t index) noexcept {
      assert(index < size());

      auto array_index = get_array_index(index);
      assert(array_index < m_arrays.size());
      size_t inner_index = index - (1ull << array_index) + 1;
      return *(m_arrays[array_index].begin() + inner_index);
    }

    void push_back(T val) {
      if (current_end == current_array_end) {
        size_t next_size = 1ull << (current_index + 1);
        m_arrays.emplace_back(next_size);
        current_begin = current_end = m_arrays.back().begin();
        current_array_end = m_arrays.back().end();
        ++current_index;
      }

      assert(current_end < current_array_end);
      new (current_end) T{ std::move(val) };
      ++current_end;
    }

    ~stack() {
      for (int index = 0; index < current_index; ++index) {
        auto uindex = static_cast<size_t>(index);
        cwt::detail::destroy_range(m_arrays[uindex].begin(), m_arrays[uindex].end());
      }

      cwt::detail::destroy_range(current_begin, current_end);
    }

    stack() = default;

    stack(stack&& o)
      :
      m_arrays(std::move(o.m_arrays)),
      current_begin(o.current_begin),
      current_end(o.current_end),
      current_array_end(o.current_array_end),
      current_index(o.current_index)
    {
      o.current_begin = nullptr;
      o.current_end = nullptr;
      o.current_array_end = nullptr;
      o.current_index = -1;
    }

  private:
    using buffer_t = cwt::detail::buffer<T,A>;
    using rebound = typename A::template rebind<buffer_t>::other;
    std::vector<buffer_t,rebound> m_arrays;
    T* current_begin = nullptr;
    T* current_end = nullptr;
    T* current_array_end = nullptr;
    int current_index = -1;

    static size_t get_array_index(size_t index) {
      return bit_scan_reverse_non_negative(static_cast<uint64_t>(index + 1ull));
    }
  };

  template<typename T, typename A>
  template<typename U>
  class stack<T, A>::iterator_templ {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using reference = U&;
    using pointer = U*;
    using difference_type = ptrdiff_t;

    iterator_templ(
      stack& s,
      CWT_DEBUG_ONLY(size_t i,)
      size_t ai,
      U* b,
      U* c,
      U* e)
      :
      stack_ref(s),
      begin(b),
      current(c),
      end(e),
      CWT_DEBUG_ONLY(index(i),)
      array_index(ai)
    {
      assert(get_linear_index() == index);
    }

    bool operator==(const iterator_templ& o) const noexcept {
      return current == o.current;
    }
    
    bool operator<(const iterator_templ& o) const noexcept {
      return array_index < o.array_index ||
        (array_index == o.array_index &&
          current < o.current);
    }

    bool operator>(const iterator_templ& o) const noexcept {
      return array_index > o.array_index ||
        (array_index == o.array_index &&
          current > o.current);
    }

    bool operator>=(const iterator_templ& o) const noexcept {
      return array_index > o.array_index ||
        (array_index == o.array_index &&
          current >= o.current);
    } 

    difference_type operator-(const iterator_templ& o) const noexcept {
      if (array_index == o.array_index) {
        return current - o.current;
      }

      assert(get_linear_index() == index);
      assert(o.get_linear_index() == o.index);
      return static_cast<difference_type>(get_linear_index() - o.get_linear_index());
    }

    iterator_templ(const iterator_templ& o) = default;

    iterator_templ operator-(difference_type diff) const noexcept {
      assert(diff <= static_cast<difference_type>(index));
      iterator_templ copy = *this;
      copy -= diff;
      return copy;
    }

    iterator_templ operator+(difference_type diff) const noexcept {
      iterator_templ copy = *this;
      copy += diff;
      return copy;
    }

    iterator_templ& operator-=(difference_type diff) noexcept {
      assert(index == stack_ref.get().size() || &stack_ref.get()[index] == current);
      
      assert(static_cast<difference_type>(index) - diff >= 0);
      assert(static_cast<difference_type>(index) - diff <= static_cast<difference_type>(stack_ref.get().size()));

      current -= diff;
      CWT_DEBUG_ONLY(index = static_cast<size_t>(static_cast<difference_type>(index) - diff));

      if (current < begin || current >= end) {
        current += diff;
        CWT_DEBUG_ONLY(auto new_index = static_cast<difference_type>(index) + diff);
        CWT_DEBUG_ONLY(index = static_cast<size_t>(new_index));
        set_linear_index(get_linear_index() - static_cast<size_t>(diff));
      }

      assert(index == stack_ref.get().size() || &stack_ref.get()[index] == current);
      return *this;
    }

    iterator_templ& operator+=(difference_type diff) noexcept {
      assert(index == stack_ref.get().size() || &stack_ref.get()[index] == current);

      assert(static_cast<difference_type>(index) + diff >= 0);
      assert(static_cast<difference_type>(index) + diff <= static_cast<difference_type>(stack_ref.get().size()));
      
      auto new_current = current + diff;
      if (new_current < begin || new_current >= end) {
        set_linear_index(get_linear_index() + static_cast<size_t>(diff));
      }
      else {
        current = new_current;
        CWT_DEBUG_ONLY(auto new_index = static_cast<difference_type>(index) + diff);
        CWT_DEBUG_ONLY(index = static_cast<size_t>(new_index));
      }

      assert(index == stack_ref.get().size() || &stack_ref.get()[index] == current);
      return *this;
    }

    iterator_templ operator++(int) noexcept {
      auto copy = *this;
      ++(*this);
      return copy;
    }

    iterator_templ& operator++() noexcept {
      assert(index == stack_ref.get().size() || &stack_ref.get()[index] == current);

      assert(index < stack_ref.get().size());
      CWT_DEBUG_ONLY(++index);

      assert(current < end);
      ++current;
      if (current == end) {
        ++array_index;
        if (array_index != stack_ref.get().m_arrays.size()) {
          auto& array_ref = stack_ref.get().m_arrays[array_index];
          begin = current = array_ref.begin();
          end = array_ref.end();
        }
        else {
          --array_index;
        }
      }

      assert(index == stack_ref.get().size() || &stack_ref.get()[index] == current);
      return *this;
    }

    iterator_templ& operator--() noexcept {
      assert(index == stack_ref.get().size() || &stack_ref.get()[index] == current);

      assert(index > 0);
      CWT_DEBUG_ONLY(--index);

      if (current == begin) {
        --array_index;
        auto& array_ref = stack_ref.get().m_arrays[array_index];
        begin = array_ref.begin();
        end = array_ref.end();
        current = end - 1;
      }
      else {
        --current;
      }

      assert(index == stack_ref.get().size() || &stack_ref.get()[index] == current);
      return *this;
    }

    bool operator!=(const iterator_templ& o) const noexcept {
      return !(*this == o);
    }

    reference operator*() {
      assert(&stack_ref.get()[index] == current);
      return *current;
    }

    U* inner() {
      return current;
    }

    size_t outer() {
      return array_index;
    }

  private:
    std::reference_wrapper<stack<T, A>> stack_ref;
    U* begin;
    U* current;
    U* end;
    CWT_DEBUG_ONLY(size_t index;)
    size_t array_index;

    size_t get_linear_index() const noexcept {
      assert(current >= begin);
      auto ret = (1ull << array_index) - 1ull + static_cast<size_t>(current - begin);
      assert(ret == index);
      return ret;
    }

    void set_linear_index(size_t linear_index) {
      auto new_array_index = stack<T,A>::get_array_index(linear_index);
      assert(new_array_index <= stack_ref.get().m_arrays.size());
      array_index = std::min(new_array_index, stack_ref.get().m_arrays.size() - 1);
      CWT_DEBUG_ONLY(index = linear_index;)
      auto& array_ref = stack_ref.get().m_arrays[array_index];
      begin = array_ref.begin();
      end = array_ref.end();
      current = begin + (linear_index - (1ull << array_index) + 1ull);

      assert(get_linear_index() == index);
      assert(index == stack_ref.get().size() || &stack_ref.get()[index] == current);
    }
  };
}
