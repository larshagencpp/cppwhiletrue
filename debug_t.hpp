#pragma once
#include <set>

namespace cwt {
  template<typename tag_t>
  class debug_t {
  public:
    debug_t() : debug_t(0) {}
    debug_t(const debug_t& o) : debug_t(o.m_data) { ++m_total_copy_constructions; }
    debug_t(debug_t&& o) noexcept : debug_t(o.m_data) { ++m_total_move_constructions; }
    debug_t(int data) : m_data(data) {
      if (m_live_objects.find(this) != m_live_objects.end()) {
        std::abort();
      }

      m_live_objects.insert(this);
    }

    ~debug_t() {
      if (m_live_objects.find(this) == m_live_objects.end()) {
        std::abort();
      }

      m_live_objects.erase(this);
    }

    static const std::set<debug_t*>& live_objects() {
      return m_live_objects;
    }

    bool operator==(int i) const noexcept {
      return m_data == i;
    }

    static size_t total_move_constructions() {
      return m_total_move_constructions;
    }

    static size_t total_copy_constructions() {
      return m_total_copy_constructions;
    }

  private:
    static std::set<debug_t*> m_live_objects;
    static size_t m_total_move_constructions;
    static size_t m_total_copy_constructions;
    int m_data = 0;
  };
}

template<typename tag_t>
std::set<cwt::debug_t<tag_t>*> cwt::debug_t<tag_t>::m_live_objects;

template<typename tag_t>
size_t cwt::debug_t<tag_t>::m_total_move_constructions = 0;

template<typename tag_t>
size_t cwt::debug_t<tag_t>::m_total_copy_constructions = 0;