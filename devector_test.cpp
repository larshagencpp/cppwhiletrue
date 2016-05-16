#include "devector.hpp"
#include <vector>
#include <chrono>
#include <random>
#include <deque>
#include <boost/circular_buffer.hpp>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

template<typename T>
class circular {
  using buffer_t = boost::circular_buffer<T>;
public:
  using value_type = typename buffer_t::value_type;

  void push_back(T val) {
    if (m_buffer.full()) {
      m_buffer.set_capacity(m_buffer.capacity() * 1.7 + 1);
    }

    m_buffer.push_back(std::move(val));
  }

  void push_front(T val) {
    if (m_buffer.full()) {
      m_buffer.set_capacity(m_buffer.capacity() * 1.7 + 1);
    }

    m_buffer.push_front(std::move(val));
  }

  T& front() {
    return m_buffer.front();
  }

  size_t size() const noexcept {
    return m_buffer.size();
  }

  void pop_front() {
    m_buffer.pop_front();
  }

  auto begin() {
    return m_buffer.begin();
  }

  auto end() {
    return m_buffer.end();
  }

private:
  boost::circular_buffer<T> m_buffer;
};

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

template<typename test_generator_t>
size_t get_num_repeats(const test_generator_t& test_generator) {
  using namespace std::chrono;
  int64_t nanos = 0;

  size_t N = 1;
  for (; nanos < 1'000'000; N *= 2) {
    std::vector<std::decay_t<decltype(test_generator())>> tests;
    std::generate_n(std::back_inserter(tests), N, test_generator);

    auto start = high_resolution_clock::now();
    for (auto& test : tests) {
      test();
    }
    auto stop = high_resolution_clock::now();

    nanos = duration_cast<nanoseconds>(stop - start).count();
  }

  return N / 2;
}

template<typename test_generator_t>
double measure_time_ns(const test_generator_t& test_generator) {
  std::vector<int64_t> measurements;
  auto repeats = get_num_repeats(test_generator);
  auto samples = 21;

  for (int k = 0; k < samples; ++k) {
    using namespace std::chrono;

    std::vector<std::decay_t<decltype(test_generator())>> tests;
    std::generate_n(std::back_inserter(tests), repeats, test_generator);

    auto start = high_resolution_clock::now();
    for (auto& test : tests) {
      test();
    }
    auto stop = high_resolution_clock::now();
    measurements.push_back(duration_cast<nanoseconds>(stop - start).count());
  }

  std::sort(measurements.begin(), measurements.end());
  return static_cast<double>(measurements[samples / 2]) / repeats;
}

template<typename T>
struct random_generator;

template<>
struct random_generator<int> {
  template<typename engine_t>
  int operator()(engine_t& engine) {
    return engine();
  }
};

template<typename C>
struct push_back_perf_test{
  push_back_perf_test(size_t N) {
    random_generator<typename C::value_type> generator;
    std::generate_n(std::back_inserter(values), N, [&] { return generator(engine); });
  }

  void operator()() {
    C c;
    for (auto&& val : values) {
      c.push_back(std::move(val));
    }
  }

  std::mt19937 engine;
  std::vector<typename C::value_type> values;
};

template<typename C>
struct iteration_perf_test {
  iteration_perf_test(size_t N) {
    random_generator<typename C::value_type> generator;
    std::generate_n(std::back_inserter(values), N, [&] { return generator(engine); });
  }

  void operator()() {
    int sum = 0;
    for (const auto& val : values) {
      sum += val;
    }
    sum_dest = sum;
  }

  std::mt19937 engine;
  C values;
  int sum_dest;
};

template<typename C>
struct push_front_perf_test {
  push_front_perf_test(size_t N) {
    random_generator<typename C::value_type> generator;
    std::generate_n(std::back_inserter(values), N, [&] { return generator(engine); });
  }

  void operator()() {
    C c;
    for (auto&& val : values) {
      c.push_front(std::move(val));
    }
  }

  std::mt19937 engine;
  std::vector<typename C::value_type> values;
};

template<typename C>
struct push_mixed_perf_test {
  push_mixed_perf_test(size_t N) {
    random_generator<typename C::value_type> generator;
    std::generate_n(std::back_inserter(values), N, [&] { return generator(engine); });
  }

  void operator()() {
    C c;
    auto end = values.size() % 2 == 0 ? values.end() : values.end() - 1;
    for (auto it = values.begin(); it != end; it += 2) {
      c.push_front(std::move(*it));
      c.push_back(std::move(*(it + 1)));
    }

    if (end != values.end()) {
      c.push_front(std::move(*end));
    }
  }

  std::mt19937 engine;
  std::vector<typename C::value_type> values;
};

template<typename C>
struct push_pop_perf_test {
  push_pop_perf_test(size_t N) {
    std::mt19937 engine;
    random_generator<typename C::value_type> generator;
    std::generate_n(std::back_inserter(values), N, [&] { return generator(engine); });
  }

  void operator()() {
    auto N = values.size();
    for (size_t i = 0; i < N; ++i) {
      auto val = std::move(values.front());
      values.pop_front();
      values.push_back(std::move(val));
    }
  }
  
  C values;
};

template<typename container_t>
double get_push_back_time(size_t N) {
  return measure_time_ns([N] { return push_back_perf_test<container_t>(N); });
}

template<typename container_t>
double get_push_front_time(size_t N) {
  return measure_time_ns([N] { return push_front_perf_test<container_t>(N); });
}

template<typename container_t>
double get_push_mixed_time(size_t N) {
  return measure_time_ns([N] { return push_mixed_perf_test<container_t>(N); });
}

template<typename container_t>
double get_push_pop_time(size_t N) {
  return measure_time_ns([N] { return push_pop_perf_test<container_t>(N); });
}

template<typename container_t>
double get_iteration_time(size_t N) {
  return measure_time_ns([N] { return iteration_perf_test<container_t>(N); });
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
  auto vec_time = get_push_back_time<std::vector<int>>(1'000'000);
  auto devec_time_back = get_push_back_time<cwt::devector<int>>(1'000'000);
  auto devec_time_front = get_push_front_time<cwt::devector<int>>(1'000'000);
  auto devec_time_mixed = get_push_mixed_time<cwt::devector<int>>(1'000'000);

  CHECK(devec_time_back <= vec_time * perf_slack);
  CHECK(devec_time_front <= vec_time * perf_slack);
  CHECK(devec_time_mixed <= vec_time * perf_slack);
}

/*
TEST_CASE("Test int push_back perf") {
  std::cout << std::endl << "integer push back" << std::endl;
  std::cout << "N, vector, devector, deque, circular" << std::endl;
  for (int N = 10; N < 10'000'000; N *= 2) {
    auto vector_time = get_push_back_time<std::vector<int>>(N);
    auto devector_time = get_push_back_time<cwt::devector<int>>(N);
    auto dequeue_time = get_push_back_time<std::deque<int>>(N);
    auto circular_time = get_push_back_time<circular<int>>(N);
    std::cout << N << ", " << vector_time << ", " << devector_time << ", " << dequeue_time << ", " << circular_time << std::endl;
  }
}

TEST_CASE("Test int push_front perf") {
  std::cout << std::endl << "integer push front" << std::endl;
  std::cout << "N, devector, deque, circular" << std::endl;
  for (int N = 10; N < 10'000'000; N *= 2) {
    auto devector_time = get_push_front_time<cwt::devector<int>>(N);
    auto dequeue_time = get_push_front_time<std::deque<int>>(N);
    auto circular_time = get_push_front_time<circular<int>>(N);
    std::cout << N << ", " << devector_time << ", " << dequeue_time << ", " << circular_time << std::endl;
  }
}

TEST_CASE("Test int push_mixed perf") {
  std::cout << std::endl << "integer push mixed" << std::endl;
  std::cout << "N, devector, deque, circular" << std::endl;
  for (int N = 10; N < 10'000'000; N *= 2) {
    auto devector_time = get_push_mixed_time<cwt::devector<int>>(N);
    auto dequeue_time = get_push_mixed_time<std::deque<int>>(N);
    auto circular_time = get_push_mixed_time<circular<int>>(N);
    std::cout << N << ", " << devector_time << ", " << dequeue_time << ", " << circular_time << std::endl;
  }
}

TEST_CASE("Test int push_pop perf") {
  std::cout << std::endl << "integer push pop" << std::endl;
  std::cout << "N, devector, deque, circular" << std::endl;
  for (int N = 10; N < 10'000'000; N *= 2) {
    auto devector_time = get_push_pop_time<cwt::devector<int>>(N);
    auto dequeue_time = get_push_pop_time<std::deque<int>>(N);
    auto circular_time = get_push_pop_time<circular<int>>(N);
    std::cout << N << ", " << devector_time << ", " << dequeue_time << ", " << circular_time << std::endl;
  }
}
*/

TEST_CASE("Test int iteration perf") {
  std::cout << std::endl << "integer iteration" << std::endl;
  std::cout << "N, vector, devector, deque, circular" << std::endl;
  for (int N = 10; N < 10'000'000; N *= 2) {
    auto vector_time = get_iteration_time<std::vector<int>>(N);
    auto devector_time = get_iteration_time<cwt::devector<int>>(N);
    auto dequeue_time = get_iteration_time<std::deque<int>>(N);
    auto circular_time = get_iteration_time<circular<int>>(N);
    std::cout << N << ", " << vector_time << ", " << devector_time << ", " << dequeue_time << ", " << circular_time << std::endl;
  }
}

#endif

