#pragma once
#include <cassert>
#include <memory>

namespace cwt {
  namespace detail {
    template<typename T, typename A = std::allocator<T>>
    class buffer : private A {
    public:
      buffer() = default;
      buffer(size_t size) {
        m_begin = this->allocate(size);
        m_end = m_begin + size;
      }

      buffer(const buffer&) = delete;
      buffer(buffer&& o)
        :
        m_begin(o.m_begin),
        m_end(o.m_end)
      {
        o.m_begin = nullptr;
        o.m_end = nullptr;
      }

      buffer& operator=(const buffer&) = delete;
      buffer& operator=(buffer&& o) {
        cleanup();
        m_begin = o.m_begin;
        m_end = o.m_end;
        o.m_begin = nullptr;
        o.m_end = nullptr;
        return *this;
      }

      ~buffer() {
        cleanup();
      }

      T* begin() const noexcept {
        return m_begin;
      }

      T* end() const noexcept {
        return m_end;
      }


      size_t size() const noexcept {
        assert(m_end >= m_begin);
        return static_cast<size_t>(m_end - m_begin);
      }

    private:
      T* m_begin = nullptr;
      T* m_end = nullptr;
      void cleanup() {
        if (m_begin) {
          this->deallocate(m_begin, size());
        }
      }
    };
  }
}
