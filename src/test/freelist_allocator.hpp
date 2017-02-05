#pragma once
#include <memory>
#include <unordered_map>

namespace cwt {
  class freelist_allocator_base {
  public:
  protected:
    static std::unordered_map<size_t, std::vector<void*>> freelist;
  };

  template<typename T>
  class freelist_allocator : public freelist_allocator_base{
  public:
    using value_type = T;
    using pointer = T*;

    freelist_allocator() = default;
    template<typename U>
    freelist_allocator(const freelist_allocator<U>&) {}
    template<typename U>
    freelist_allocator(freelist_allocator<U>&&) {}

    pointer allocate(size_t n) {
      auto& allocs = freelist[n * sizeof(T)];
      if(allocs.empty()) {
        return std::allocator<T>().allocate(n);
      }
      
      auto alloc = allocs.back();
      allocs.pop_back();
      return static_cast<pointer>(alloc);
    }

    void deallocate(pointer ptr, size_t n) {
      freelist[n * sizeof(T)].push_back(ptr);
    }
  private:
  };
  
  std::unordered_map<size_t, std::vector<void*>> freelist_allocator_base::freelist;
}