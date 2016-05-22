#pragma once
#include "debug_allocator.hpp"
#include "debug_t.hpp"

#include <chrono>
#include <random>
#include <algorithm>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

template<template<typename T, typename A> typename container_t, typename T>
int count_allocations_front(size_t N) {
  struct test_tag {};

  auto start = cwt::debug_allocator<T, test_tag>::total_allocations();

  container_t<T, cwt::debug_allocator<T, test_tag>> cont;
  for (unsigned i = 0; i < N; ++i) {
    cont.push_front(i);
  }

  auto stop = cwt::debug_allocator<T, test_tag>::total_allocations();

  return static_cast<int>(stop - start);
}

template<template<typename T, typename A> typename container_t, typename T>
size_t count_total_allocated_bytes(size_t N) {
  struct test_tag {};

  auto start = cwt::debug_allocator<T, test_tag>::total_bytes_allocated();

  container_t<T, cwt::debug_allocator<T, test_tag>> cont;
  for (unsigned i = 0; i < N; ++i) {
    cont.push_back(i);
  }

  auto stop = cwt::debug_allocator<T, test_tag>::total_bytes_allocated();

  return stop - start;
}

template<template<typename T, typename A> typename container_t, typename T>
int count_allocations(size_t N) {
  struct test_tag {};

  auto start = cwt::debug_allocator<T, test_tag>::total_allocations();

  container_t<T, cwt::debug_allocator<T, test_tag>> cont;
  for (unsigned i = 0; i < N; ++i) {
    cont.push_back(i);
  }

  auto stop = cwt::debug_allocator<T, test_tag>::total_allocations();

  return static_cast<int>(stop - start);
}

template<template<typename T, typename A = std::allocator<T>> typename container_t>
int count_copy_constructions(size_t N) {
  struct test_tag {};

  auto start = cwt::debug_t<test_tag>::total_copy_constructions();

  container_t<cwt::debug_t<test_tag>> cont;
  for (unsigned i = 0; i < N; ++i) {
    cont.push_back(i);
  }

  auto stop = cwt::debug_t<test_tag>::total_copy_constructions();

  return static_cast<int>(stop - start);
}

template<template<typename T, typename A = std::allocator<T>> typename container_t>
int count_move_constructions(size_t N) {
  struct test_tag {};

  auto start = cwt::debug_t<test_tag>::total_move_constructions();

  container_t<cwt::debug_t<test_tag>> cont;
  for (unsigned i = 0; i < N; ++i) {
    cont.push_back(i);
  }

  auto stop = cwt::debug_t<test_tag>::total_move_constructions();

  return static_cast<int>(stop - start);
}

template<template<typename T, typename A> typename container_t, typename T>
double get_average_memory_usage(size_t N) {
  struct test_tag {};
  container_t<T, cwt::debug_allocator<T, test_tag>> cont;

  size_t sum_memory_usage = 0;
  for (unsigned i = 0; i < N; ++i) {
    cont.push_back(i);
    sum_memory_usage += cwt::debug_allocator<T, test_tag>::current_bytes_allocated();
  }

  return  sum_memory_usage / 10000.0;
}

template<typename test_generator_t>
size_t get_num_repeats(const test_generator_t& test_generator) {
  using namespace std::chrono;
  int64_t nanos = 0;

  size_t N = 1;
  for (; nanos < 100'000; N *= 2) {
    std::vector<std::decay_t<decltype(test_generator())>> tests;
    std::generate_n(std::back_inserter(tests), N, test_generator);

    LARGE_INTEGER start, stop, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    for (auto& test : tests) {
      test();
    }
    QueryPerformanceCounter(&stop);

    nanos = ((stop.QuadPart - start.QuadPart) * 1'000'000'000) / freq.QuadPart;
  }

  return N / 2;
}

template<typename func_t>
auto get_median(size_t num_reps, const func_t& func) {
  using ret_t = std::decay_t<decltype(func())>;
  std::vector<ret_t> measurements;
  for (unsigned i = 0; i < num_reps; ++i) {
    measurements.push_back(func());
  }

  std::sort(measurements.begin(), measurements.end());
  return measurements[num_reps / 2];
}

template<typename test_generator_t>
double measure_time_ns(const test_generator_t& test_generator) {
  auto repeats = get_num_repeats(test_generator);

  auto median = get_median(51, [&] {
    std::vector<std::decay_t<decltype(test_generator())>> tests;
    std::generate_n(std::back_inserter(tests), repeats, test_generator);

    LARGE_INTEGER start, stop, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    for (auto& test : tests) {
      test();
    }
    QueryPerformanceCounter(&stop);

    return ((stop.QuadPart - start.QuadPart) * 1'000'000'000) / freq.QuadPart;
  });

  return static_cast<double>(median) / repeats;
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
struct push_back_perf_test {
  push_back_perf_test(size_t N) {
    random_generator<typename C::value_type> generator;
    std::generate_n(std::back_inserter(values), N, [&] { return generator(engine); });
  }

  void operator()() {
    for (auto&& val : values) {
      c.push_back(std::move(val));
    }
  }

  std::mt19937 engine;
  C c;
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
struct sort_perf_test {
  sort_perf_test(size_t N) {
    random_generator<typename C::value_type> generator;
    std::generate_n(std::back_inserter(values), N, [&] { return generator(engine); });
  }

  void operator()() {
    std::sort(values.begin(), values.end());
  }

  std::mt19937 engine;
  C values;
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

template<typename container_t>
double get_sort_time(size_t N) {
  return measure_time_ns([N] { return sort_perf_test<container_t>(N); });
}

template<typename container_t>
double get_max_push_back_time(size_t N) {
  std::mt19937 engine;
  std::vector<typename container_t::value_type> values;

  random_generator<typename container_t::value_type> generator;
  std::generate_n(std::back_inserter(values), N, [&] { return generator(engine); });

  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);

  return get_median(101, [&] {
    int64_t max_time = 0;
    LARGE_INTEGER start, stop;
    
    container_t c;
    for (const auto& val : values) {
      QueryPerformanceCounter(&start);
      c.push_back(std::move(val));
      QueryPerformanceCounter(&stop);
      max_time = std::max(max_time, stop.QuadPart - start.QuadPart);
    }

    return (max_time * 1'000'000'000) / freq.QuadPart;
  });
}