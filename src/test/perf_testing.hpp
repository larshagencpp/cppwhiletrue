#pragma once
#include "debug_allocator.hpp"
#include <array>
#include <algorithm>
#include <chrono>
#include <random>

#ifdef __unix__
#include <sys/time.h>
#include <sys/resource.h>
#elif defined(_WIN32) || defined(WIN32)
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#endif

template<typename T>
struct random_generator;

template<>
struct random_generator<int> {
  template<typename engine_t>
  int operator()(engine_t& engine) {
    return static_cast<int>(engine());
  }
};

template<typename test_generator_t>
size_t get_num_repeats(const test_generator_t& test_generator) {
  using namespace std::chrono;
  int64_t nanos = 0;

  size_t N = 1;
  for (; nanos < 100'000; N *= 2) {
    std::vector<std::decay_t<decltype(test_generator())>> tests;
    std::generate_n(std::back_inserter(tests), N, test_generator);

    using namespace std::chrono;
    auto start = high_resolution_clock::now();
    for (auto& test : tests) {
      test();
    }
    auto stop = high_resolution_clock::now();

    nanos = duration_cast<nanoseconds>(stop - start).count();
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

    using namespace std::chrono;
    auto start = high_resolution_clock::now();
    for (auto& test : tests) {
      test();
    }
    auto stop = high_resolution_clock::now();

    return duration_cast<nanoseconds>(stop - start).count();
  });

  return static_cast<double>(median) / static_cast<double>(repeats);
}

auto get_page_fault_count() {
#ifdef __unix__
  rusage usage;
  auto ret = getrusage(RUSAGE_SELF, &usage);
  if(ret != 0) {
    throw std::runtime_error("getrusage() call failed");
  }
  
  return usage.ru_minflt + usage.ru_majflt;
#elif defined(_WIN32) || defined(WIN32)
  PROCESS_MEMORY_COUNTERS pmc;
  if(!GetProcessMemoryInfo( GetCurrentProcess(), &pmc, sizeof(pmc))) {
    throw std::runtime_error("GetProcessMemoryInfo() call failed");
  }
  
  return pmc.PageFaultCount;
#endif  
}

template<typename test_generator_t>
double measure_page_faults(const test_generator_t& test_generator) {
  auto median = get_median(51, [&] {
    auto test = test_generator();

    using namespace std::chrono;
    auto start = get_page_fault_count();
    test();
    auto stop = get_page_fault_count();

    return stop - start;
  });

  return static_cast<double>(median);
}

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

template<typename container_t>
double get_push_back_time(size_t N) {
  return measure_time_ns([N] { return push_back_perf_test<container_t>(N); });
}

template<typename container_t>
double get_push_back_page_faults(size_t N) {
  return measure_page_faults([N] { return push_back_perf_test<container_t>(N); });
}

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

template<typename container_t>
double get_sort_time(size_t N) {
  return measure_time_ns([N] { return sort_perf_test<container_t>(N); });
}

template<template<typename T, typename A> class container_t, typename T>
double get_average_memory_usage(size_t N) {
  struct test_tag {};
  container_t<T, cwt::debug_allocator<T, test_tag>> cont;

  size_t sum_memory_usage = 0;
  for (unsigned i = 0; i < N; ++i) {
    cont.push_back(static_cast<T>(i));
    sum_memory_usage += cwt::debug_allocator<T, test_tag>::current_bytes_allocated();
  }

  return  static_cast<double>(sum_memory_usage) / static_cast<double>(N);
}

template<template<typename T, typename A> class container_t, typename T>
size_t count_total_allocated_bytes(size_t N) {
  struct test_tag {};

  auto start = cwt::debug_allocator<T, test_tag>::total_bytes_allocated();

  container_t<T, cwt::debug_allocator<T, test_tag>> cont;
  for (unsigned i = 0; i < N; ++i) {
    cont.push_back(static_cast<T>(i));
  }

  auto stop = cwt::debug_allocator<T, test_tag>::total_bytes_allocated();

  return stop - start;
}
