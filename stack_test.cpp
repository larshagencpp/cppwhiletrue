#include "stack.hpp"
#include "perf_testing.hpp"
#include "debug_t.hpp"
#include <numeric>
#include <deque>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

TEST_CASE("New stack is empty") {
  cwt::stack<int> s;
  REQUIRE(s.size() == 0);
}

TEST_CASE("New stack is empty range") {
  cwt::stack<int> s;
  REQUIRE(s.begin() == s.end());
}

TEST_CASE("Add one element to stack") {
  cwt::stack<int> s;
  s.push_back(6);
  REQUIRE(s.size() == 1);
  REQUIRE(std::distance(s.begin(), s.end()) == 1);
  REQUIRE(s[0] == 6);
  REQUIRE(*s.begin() == 6);
  REQUIRE(*(s.end() - 1) == 6);
  REQUIRE(std::accumulate(s.begin(), s.end(), 0) == 6);
}

TEST_CASE("Add three elements to stack") {
  cwt::stack<int> s;
  s.push_back(6);
  s.push_back(3);
  s.push_back(7);
  REQUIRE(s.size() == 3);
  REQUIRE(std::distance(s.begin(), s.end()) == 3);
  REQUIRE(s[0] == 6);
  REQUIRE(s[1] == 3);
  REQUIRE(s[2] == 7);
  REQUIRE(*s.begin() == 6);
  REQUIRE(*(s.end() - 1) == 7);
  REQUIRE(std::accumulate(s.begin(), s.end(), 0) == 16);
}

TEST_CASE("Add many elements to stack") {
  cwt::stack<int> s;
  std::vector<int> v;
  
  for (int i = 0; i < 1000; ++i) {
    s.push_back(i);
    v.push_back(i);
    REQUIRE(s.size() == v.size());
    REQUIRE(*(s.begin() + (s.size() - 1)) == i);
  }

  REQUIRE(std::equal(s.begin(), s.end(), v.begin()));
}

TEST_CASE("Stack calls correct constructors/destructors") {
  struct test_tag {};

  {
    cwt::stack<cwt::debug_t<test_tag>> s;
    REQUIRE(cwt::debug_t<test_tag>::live_objects().size() == 0);

    for (int i = 0; i < 100; ++i) {
      s.push_back(i);
    }

    REQUIRE(s.size() == 100);
    REQUIRE(cwt::debug_t<test_tag>::live_objects().size() == 100);
  }

  REQUIRE(cwt::debug_t<test_tag>::live_objects().size() == 0);
}

TEST_CASE("Iterators work for sorting") {
  cwt::stack<int> s;
  for (int i = 0; i < 1000; ++i) {
    s.push_back(i);
  }

  std::mt19937 engine;
  std::shuffle(s.begin(), s.end(), engine);

  std::vector<int> v(s.begin(), s.end());
  std::sort(s.begin(), s.end());
  std::sort(v.begin(), v.end());

  REQUIRE(s.size() == v.size());
  REQUIRE(std::equal(v.begin(), v.end(), s.begin(), s.end()));
}

TEST_CASE("integer push back average memory usage") {
  std::cout << std::endl << "integer push back average memory usage" << std::endl;
  std::cout << "N, vector, stack, deque" << std::endl;
  for (int N = 1; N < 100'000; N = N * 1.5 + 1) {
    auto vector_time = get_average_memory_usage<std::vector, int>(N);
    auto stack_time = get_average_memory_usage<cwt::stack, int>(N);
    auto dequeue_time = get_average_memory_usage<std::deque, int>(N);
    std::cout << N << ", " << vector_time << ", " << stack_time << ", " << dequeue_time << std::endl;
  }
}

TEST_CASE("integer push back total bytes") {
  std::cout << std::endl << "integer push back total bytes" << std::endl;
  std::cout << "N, vector, stack, deque" << std::endl;
  for (int N = 1; N < 100'000; N = N * 1.5 + 1) {
    auto vector_time = count_total_allocated_bytes<std::vector, int>(N);
    auto stack_time = count_total_allocated_bytes<cwt::stack, int>(N);
    auto dequeue_time = count_total_allocated_bytes<std::deque, int>(N);
    std::cout << N << ", " << vector_time << ", " << stack_time << ", " << dequeue_time << std::endl;
  }
}

#ifdef NDEBUG
TEST_CASE("Stack is faster than vector for integer push_back") {
  auto vec_time = get_push_back_time<std::vector<int>>(1'000'000);
  auto queue_time = get_push_back_time<std::deque<int>>(1'000'000);
  auto stack_time = get_push_back_time<cwt::stack<int>>(1'000'000);

  WARN(vec_time);
  WARN(queue_time);
  WARN(stack_time);

  CHECK(stack_time < vec_time);
  CHECK(stack_time < queue_time);
}

TEST_CASE("Measure sort time for stack") {
  std::cout << std::endl << "integer sorting" << std::endl;
  std::cout << "N, vector, stack, deque" << std::endl;
  for (int N = 10; N < 1'000'000; N *= 2) {
    auto vector_time = get_sort_time<std::vector<int>>(N);
    auto stack_time = get_sort_time<cwt::stack<int>>(N);
    auto dequeue_time = get_sort_time<std::deque<int>>(N);
    std::cout << N << ", " << vector_time << ", " << stack_time << ", " << dequeue_time << std::endl;
  }
}
#endif