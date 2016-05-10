#include "devector.hpp"
#include <vector>
#include <chrono>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

template<typename tag_t>
class debug_t {
public:
  debug_t() : debug_t(0) {}
  debug_t(const debug_t& o) : debug_t(o.m_data) {}
  debug_t(debug_t&& o) noexcept : debug_t(o.m_data) {}
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

private:
  static std::set<debug_t*> m_live_objects;
  int m_data = 0;
};

template<typename tag_t>
std::set<debug_t<tag_t>*> debug_t<tag_t>::m_live_objects;

TEST_CASE("New devector is empty") {
  cwt::devector<int> dv;
  REQUIRE(dv.size() == 0);
}

TEST_CASE("New devector is empty range") {
  cwt::devector<int> dv;
  REQUIRE(dv.begin() == dv.end());
}

TEST_CASE("New devector has no capacity") {
  cwt::devector<int> dv;
  REQUIRE(dv.capacity() == 0);
}

TEST_CASE("Inserting an element increases size") {
  cwt::devector<int> dv;
  dv.push_back(1);
  REQUIRE(dv.size() == 1);
  REQUIRE(dv.begin() < dv.end());
  REQUIRE(std::distance(dv.begin(), dv.end()) == 1);
  REQUIRE(dv.capacity() >= 1);
}

TEST_CASE("Inserting an element makes the element accessible") {
  cwt::devector<int> dv;
  dv.push_back(1);
  REQUIRE(*dv.begin() == 1);
  REQUIRE(dv[0] == 1);
}

TEST_CASE("Push back constructs object and calls destructor in tear-down") {
  struct test_tag {};

  {
    cwt::devector<debug_t<test_tag>> dv;
    REQUIRE(debug_t<test_tag>::live_objects().size() == 0);

    dv.push_back(7);
    REQUIRE(*dv.begin() == 7);
    REQUIRE(dv[0] == 7);

    REQUIRE(debug_t<test_tag>::live_objects().size() == 1);
  }

  REQUIRE(debug_t<test_tag>::live_objects().size() == 0);
}

TEST_CASE("Push front constructs object and calls destructor in tear-down") {
  struct test_tag {};

  {
    cwt::devector<debug_t<test_tag>> dv;
    REQUIRE(debug_t<test_tag>::live_objects().size() == 0);

    dv.push_back(7);
    dv.push_front(9);
    REQUIRE(*dv.begin() == 9);
    REQUIRE(dv[0] == 9);

    REQUIRE(debug_t<test_tag>::live_objects().size() == 2);
  }

  REQUIRE(debug_t<test_tag>::live_objects().size() == 0);
}

TEST_CASE("Reallocation calls correct destructors") {
  struct test_tag {};

  {
    cwt::devector<debug_t<test_tag>> dv;
    REQUIRE(debug_t<test_tag>::live_objects().size() == 0);

    for (int i = 0; i < 100; ++i) {
      dv.push_back(i);
    }

    REQUIRE(dv.size() == 100);
    REQUIRE(*dv.begin() == 0);
    REQUIRE(*(dv.end() - 1) == 99);
    REQUIRE(dv[50] == 50);

    REQUIRE(debug_t<test_tag>::live_objects().size() == 100);
  }

  REQUIRE(debug_t<test_tag>::live_objects().size() == 0);
}

TEST_CASE("Reallocation calls correct destructors for push_front") {
  struct test_tag {};

  {
    cwt::devector<debug_t<test_tag>> dv;
    REQUIRE(debug_t<test_tag>::live_objects().size() == 0);

    for (int i = 0; i < 100; ++i) {
      dv.push_front(i);
    }

    REQUIRE(dv.size() == 100);
    REQUIRE(*dv.begin() == 99);
    REQUIRE(*(dv.end() - 1) == 0);
    REQUIRE(dv[50] == 49);

    REQUIRE(debug_t<test_tag>::live_objects().size() == 100);
  }

  REQUIRE(debug_t<test_tag>::live_objects().size() == 0);
}

template<typename container_t>
int count_allocations(container_t& cont) {
  auto prev_capacity = cont.capacity();
  int num_allocations = 0;

  for (int i = 0; i < 10000; ++i) {
    cont.push_back(i);
    if (cont.capacity() != prev_capacity) {
      ++num_allocations;
      prev_capacity = cont.capacity();
    }
  }

  return num_allocations;
}

template<typename container_t>
int count_allocations_front(container_t& cont) {
  auto prev_capacity = cont.capacity();
  int num_allocations = 0;

  for (int i = 0; i < 10000; ++i) {
    cont.push_front(i);
    if (cont.capacity() != prev_capacity) {
      ++num_allocations;
      prev_capacity = cont.capacity();
    }
  }

  return num_allocations;
}

template<typename container_t>
size_t count_total_allocated_bytes(container_t& cont) {
  auto prev_capacity = cont.capacity();
  size_t num_allocated_bytes = 0;

  for (int i = 0; i < 10000; ++i) {
    cont.push_back(i);
    if (cont.capacity() != prev_capacity) {
      num_allocated_bytes += cont.capacity();
      prev_capacity = cont.capacity();
    }
  }

  return num_allocated_bytes;
}

template<typename container_t>
double get_average_memory_usage(container_t& cont) {
  size_t sum_memory_usage = 0;
  for (int i = 0; i < 10000; ++i) {
    cont.push_back(i);
    sum_memory_usage += cont.capacity();
  }

  return  sum_memory_usage / 10000.0;
}

template<typename F>
double measure_time_ns(const F& f) {
  std::vector<int64_t> measurements;

  for (int k = 0; k < 101; ++k) {
    using namespace std::chrono;

    auto start = high_resolution_clock::now();
    f();

    auto stop = high_resolution_clock::now();
    measurements.push_back(duration_cast<nanoseconds>(stop - start).count());
  }

  std::sort(measurements.begin(), measurements.end());
  return static_cast<double>(measurements[50]);
}

template<typename container_t>
double get_push_back_time() {
  return measure_time_ns([] {
    container_t cont;
    for (int i = 0; i < 1'000'000; ++i) {
      cont.push_back(i);
    }
  });
}

template<typename container_t>
double get_push_front_time() {
  return measure_time_ns([] {
    container_t cont;
    for (int i = 0; i < 1'000'000; ++i) {
      cont.push_front(i);
    }
  });
}

template<typename container_t>
double get_push_mixed_time() {
  return measure_time_ns([] {
    container_t cont;
    for (int i = 0; i < 500'000; ++i) {
      cont.push_front(i);
      cont.push_back(i);
    }
  });
}

//Allow 15% slack compared to std::vector
constexpr double perf_slack = 1.15;

TEST_CASE("Devector allocates about as often as vector") {
  std::vector<int> vec;
  cwt::devector<int> devec;

  int vec_allocs = count_allocations(vec);
  int devec_allocs = count_allocations(devec);
  int devec_allocs_front = count_allocations_front(devec);
  
  REQUIRE(devec_allocs <= vec_allocs * perf_slack);
  REQUIRE(devec_allocs_front <= vec_allocs * perf_slack);
}

TEST_CASE("Devector has about the same memory overhead as vector") {
  std::vector<int> vec;
  cwt::devector<int> devec;

  double vec_mem = get_average_memory_usage(vec);
  double devec_mem = get_average_memory_usage(devec);

  REQUIRE(devec_mem <= vec_mem * perf_slack);
}

TEST_CASE("Devector has about the same memory churn as vector") {
  std::vector<int> vec;
  cwt::devector<int> devec;

  auto vec_bytes = count_total_allocated_bytes(vec);
  auto devec_bytes = count_total_allocated_bytes(devec);

  REQUIRE(devec_bytes <= vec_bytes * perf_slack);
}

#ifdef NDEBUG
TEST_CASE("Devector is about as fast as vector for integer push_back/push_front") {
  auto vec_time = get_push_back_time<std::vector<int>>();
  auto devec_time_back = get_push_back_time<cwt::devector<int>>();
  auto devec_time_front = get_push_front_time<cwt::devector<int>>();
  auto devec_time_mixed = get_push_mixed_time<cwt::devector<int>>();

  CHECK(devec_time_back <= vec_time * perf_slack);
  CHECK(devec_time_front <= vec_time * perf_slack);
  CHECK(devec_time_mixed <= vec_time * perf_slack);
}
#endif

