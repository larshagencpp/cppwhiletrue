#pragma once
#include "debug_allocator.hpp"
#include "debug_t.hpp"
#include "perf_testing.hpp"

#include <chrono>
#include <random>
#include <algorithm>

template<template<typename T, typename A> class container_t, typename T>
int count_allocations_front(size_t N) {
  struct test_tag {};

  auto start = cwt::debug_allocator<T, test_tag>::total_allocations();

  container_t<T, cwt::debug_allocator<T, test_tag>> cont;
  for (unsigned i = 0; i < N; ++i) {
    cont.push_front(static_cast<T>(i));
  }

  auto stop = cwt::debug_allocator<T, test_tag>::total_allocations();

  return static_cast<int>(stop - start);
}

template<template<typename T, typename A> class container_t, typename T>
int count_allocations(size_t N) {
  struct test_tag {};

  auto start = cwt::debug_allocator<T, test_tag>::total_allocations();

  container_t<T, cwt::debug_allocator<T, test_tag>> cont;
  for (unsigned i = 0; i < N; ++i) {
    cont.push_back(static_cast<T>(i));
  }

  auto stop = cwt::debug_allocator<T, test_tag>::total_allocations();

  return static_cast<int>(stop - start);
}

template<template<typename T, typename A = std::allocator<T>> class container_t>
int count_copy_constructions(size_t N) {
  struct test_tag {};

  auto start = cwt::debug_t<test_tag>::total_copy_constructions();

  container_t<cwt::debug_t<test_tag>> cont;
  for (unsigned i = 0; i < N; ++i) {
    cont.push_back(cwt::debug_t<test_tag>{static_cast<int>(i)});
  }

  auto stop = cwt::debug_t<test_tag>::total_copy_constructions();

  return static_cast<int>(stop - start);
}

template<template<typename T, typename A = std::allocator<T>> class container_t>
int count_move_constructions(size_t N) {
  struct test_tag {};

  auto start = cwt::debug_t<test_tag>::total_move_constructions();

  container_t<cwt::debug_t<test_tag>> cont;
  for (unsigned i = 0; i < N; ++i) {
    cont.push_back(cwt::debug_t<test_tag>{static_cast<int>(i)});
  }

  auto stop = cwt::debug_t<test_tag>::total_move_constructions();

  return static_cast<int>(stop - start);
}

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
    for (auto&& val : values) {
      c.push_front(std::move(val));
    }
  }

  C c;
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
    auto end = values.size() % 2 == 0 ? values.end() : values.end() - 1;
    for (auto it = values.begin(); it != end; it += 2) {
      c.push_front(std::move(*it));
      c.push_back(std::move(*(it + 1)));
    }

    if (end != values.end()) {
      c.push_front(std::move(*end));
    }
  }

  C c;
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

template<typename container_t>
double get_max_push_back_time(size_t N) {
  std::mt19937 engine;
  std::vector<typename container_t::value_type> values;

  random_generator<typename container_t::value_type> generator;
  std::generate_n(std::back_inserter(values), N, [&] { return generator(engine); });

  return get_median(101, [&] {
    int64_t max_time = 0;
    using namespace std::chrono;
    
    container_t c;
    for (const auto& val : values) {
      auto start = high_resolution_clock::now();
      c.push_back(std::move(val));
      auto stop = high_resolution_clock::now();
      max_time = std::max(max_time, static_cast<int64_t>(duration_cast<nanoseconds>(stop - start).count()));
    }

    return static_cast<double>(max_time);
  });
}
