#pragma once
#include <cstddef>
#include <memory>
#include <map>
#include <limits>

namespace cwt {
  template<typename tag_t>
  class debug_allocator_base {
  public:
    static size_t total_bytes_allocated() {
      return m_total_bytes_allocated;
    }

    static size_t current_bytes_allocated() {
      return m_current_bytes_allocated;
    }

    static size_t total_allocations() {
      return m_total_allocations;
    }

    static const std::map<void*, size_t>& live_allocations() {
      return m_live_allocations;
    }

  protected:
    static size_t m_total_bytes_allocated;
    static size_t m_current_bytes_allocated;
    static size_t m_total_allocations;
    static std::map<void*, size_t> m_live_allocations;
  };

  template<typename T, typename tag_t>
  class debug_allocator : public debug_allocator_base<tag_t>{
  public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = T const*;
    using reference = T&;
    using const_reference = T const&;
    using difference_type = ptrdiff_t;
    using size_type = size_t;

    debug_allocator() = default;
    template<typename U>
    debug_allocator(const debug_allocator<U, tag_t>&) {}
    template<typename U>
    debug_allocator(debug_allocator<U, tag_t>&&) {}

    pointer allocate(size_t n) {
      pointer ptr = std::allocator<T>().allocate(n);

      size_t num_bytes = n * sizeof(T);
      this->m_total_bytes_allocated += num_bytes;
      this->m_current_bytes_allocated += num_bytes;
      ++this->m_total_allocations;

      if (this->m_live_allocations.find(ptr) != this->m_live_allocations.end()) {
        std::abort();
      }

      this->m_live_allocations.emplace(ptr, num_bytes);

      return ptr;
    }

    pointer allocate( std::size_t n, const void *) {
      return allocate(n);
    }

    void deallocate(pointer ptr, size_t n) {
      auto it = this->m_live_allocations.find(ptr);
      if (it == this->m_live_allocations.end()) {
        std::abort();
      }

      size_t num_bytes = n * sizeof(T);
      if (it->second != num_bytes) {
        std::abort();
      }

      this->m_live_allocations.erase(ptr);

      this->m_current_bytes_allocated -= num_bytes;

      std::allocator<T>().deallocate(ptr, n);
    }

    template<typename ...Args>
    void construct(pointer ptr, Args&&... args) {
      new (ptr) value_type{std::forward<Args>(args)...};
    }

    template<typename U>
    void destroy(U* ptr) noexcept {
      (void)ptr;
      ptr->~U();
    }

    size_type max_size() const noexcept {
      return std::numeric_limits<size_type>::max() / sizeof(value_type);
    }

  private:
  };
}

template<typename tag_t>
size_t cwt::debug_allocator_base<tag_t>::m_total_bytes_allocated = 0;

template<typename tag_t>
size_t cwt::debug_allocator_base<tag_t>::m_current_bytes_allocated = 0;

template<typename tag_t>
size_t cwt::debug_allocator_base<tag_t>::m_total_allocations = 0;

template<typename tag_t>
std::map<void*, size_t> cwt::debug_allocator_base<tag_t>::m_live_allocations;
