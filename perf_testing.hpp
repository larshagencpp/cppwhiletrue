#pragma once
#include "debug_allocator.hpp"
#include <algorithm>
#include <chrono>
#include <random>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

template<typename T>
struct random_generator;

template<>
struct random_generator<int> {
  template<typename engine_t>
  int operator()(engine_t& engine) {
    return engine();
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

template<template<typename T, typename A> typename container_t, typename T>
double get_average_memory_usage(size_t N) {
  struct test_tag {};
  container_t<T, cwt::debug_allocator<T, test_tag>> cont;

  size_t sum_memory_usage = 0;
  for (unsigned i = 0; i < N; ++i) {
    cont.push_back(i);
    sum_memory_usage += cwt::debug_allocator<T, test_tag>::current_bytes_allocated();
  }

  return  sum_memory_usage / N;
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