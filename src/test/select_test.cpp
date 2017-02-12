#include <random>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <iostream>
#include <array>
#include <cassert>
#include <functional>

auto generate_numbers(int n, std::mt19937& engine) {
  std::vector<int> numbers;
  std::generate_n(std::back_inserter(numbers), n, [&] { return engine(); });
  return numbers;
}

template<typename... F>
std::vector<uint64_t> measure_times(
  int,
  const std::vector<int>&,
  std::vector<int>&)
{
  return {};
}

template<typename F>
int64_t median_time(
  int repeats,
  int n,
  const std::vector<int>& orig,
  std::vector<int>& scratch,
  const F& algo)
{
  std::vector<int64_t> times;
  for (int i = 0; i < repeats; ++i) {
    scratch = orig;
    auto start = std::chrono::high_resolution_clock::now();
    algo(scratch.begin(), scratch.begin() + n, scratch.end(), std::less<>());
    auto stop = std::chrono::high_resolution_clock::now();
    times.push_back(std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count());
  }

  std::nth_element(times.begin(), times.begin() + repeats/2, times.end());
  return times[static_cast<size_t>(repeats/2)];
}

template<typename F, typename... Fs>
std::vector<int64_t> measure_times(
  int n,
  const std::vector<int>& orig,
  std::vector<int>& scratch,
  const F& algo,
  const Fs& ...algos)
{
  std::vector<int64_t> ret = { median_time(51, n, orig, scratch, algo) };
  auto rest = measure_times(n, orig, scratch, algos...);
  ret.insert(ret.end(), rest.begin(), rest.end());
  return ret;
}

template<typename... Fs>
void test_perf(const Fs& ...algos) {
  std::mt19937 engine{ std::random_device{}() };
  const int N = 1'000'000;
  auto numbers = generate_numbers(N, engine);
  const auto copy = numbers;

  for (int n = 1; n < N; n = (n*4)/3 + 10) {
    auto times = measure_times(n, copy, numbers, algos...);
    std::cout << n;
    for (auto time : times) {
      std::cout << ",\t" << time;
    }

    std::cout << "\n";
  }
}

template<typename... F>
void test_perf_desc(const F& ...algos) {
  std::mt19937 engine{ std::random_device{}() };
  const int N = 1'000'000;
  auto numbers = generate_numbers(N, engine);
  std::sort(numbers.begin(), numbers.end(), std::greater<>());
  const auto copy = numbers;

  for (int n = 1; n < N; n = (n * 4) / 3 + 10) {
    auto times = measure_times(n, copy, numbers, algos...);
    std::cout << n;
    for (auto time : times) {
      std::cout << ",\t" << time;
    }

    std::cout << "\n";
  }
}

template<typename F>
void test_correctness(const F& algo) {
  std::mt19937 engine{ std::random_device{}() };

  for (int i = 0; i < 10; ++i) {
    auto numbers = generate_numbers(1'000, engine);
    auto copy = numbers;

    auto pos = static_cast<std::ptrdiff_t>(engine() % numbers.size());
    algo(numbers.begin(), numbers.begin() + pos, numbers.end(), std::less<>());

    std::sort(numbers.begin(), numbers.begin() + pos);
    std::sort(numbers.begin() + pos + 1, numbers.end());
    std::sort(copy.begin(), copy.end());

    if (!std::equal(numbers.begin(), numbers.end(), copy.begin(), copy.end())) {
      throw std::logic_error("Error in select algorithm");
    }
  }
}

template<typename I, typename C>
void heap_select(I begin, I middle, I end, const C& comp) {
  auto one_past_middle = middle + 1;
  std::make_heap(begin, one_past_middle, comp);
  for (auto it = one_past_middle; it < end; ++it) {
    if (comp(*it, *begin)) {
      std::pop_heap(begin, one_past_middle, comp);
      std::swap(*it, *middle);
      std::push_heap(begin, one_past_middle, comp);
    }
  }

  std::swap(*begin, *middle);
}

template<typename I, typename C>
void select_select(I begin, I middle, I end, const C& comp) {
  auto one_past_middle = middle + 1;
  std::swap(*middle, *std::max_element(begin, one_past_middle, comp));
  auto end_of_current_set = one_past_middle;
  for (auto it = one_past_middle; it < end; ++it) {
    if (comp(*it, *middle)) {
      std::swap(*it, *end_of_current_set++);
      if (end_of_current_set - begin > 2 * (one_past_middle - begin)) {
        std::nth_element(begin, middle, end_of_current_set, comp);
        end_of_current_set = one_past_middle;
      }
    }
  }

  std::nth_element(begin, middle, end_of_current_set, comp);
}

template<typename I, typename C>
I partition_range(I begin, I end, const C& comp) {
  std::array<I, 3> pivots{ begin, begin + (end - begin) / 2, end - 1 };
  std::sort(pivots.begin(), pivots.end(), [&comp](auto p1, auto p2) { return comp(*p1, *p2); });
  std::swap(*begin, *pivots[1]);
  auto it = std::partition(begin + 1, end, [begin,&comp](const auto& val) { return comp(val, *begin); });
  std::swap(*begin, *--it);
  return it;
}

template<typename I, typename C>
I upper_half_nth_element(I begin, I middle, I end, const C& comp) {
  auto it = partition_range(begin, end, comp);
  return it >= middle ? it : upper_half_nth_element(it + 1, middle, end, comp);
}

template<typename I, typename C>
void relaxed_select_select(I begin, I middle, I end, const C& comp) {
  auto one_past_middle = middle + 1;
  std::swap(*middle, *std::max_element(begin, one_past_middle, comp));
  auto end_of_current_set = one_past_middle;
  auto current_max = middle;

  for (auto it = one_past_middle; it < end; ++it) {
    if (comp(*it, *current_max)) {
      std::swap(*it, *end_of_current_set++);
      if (end_of_current_set - begin > 2 * (one_past_middle - begin)) {
        current_max = upper_half_nth_element(begin, middle, end_of_current_set, comp);
        end_of_current_set = current_max + 1;
      }
    }
  }

  std::nth_element(begin, middle, end_of_current_set, comp);
}

template<typename I, typename C>
void twoway_relaxed_select_select(I begin, I middle, I end, const C& comp) {
  if (middle < begin + (end - begin) / 2) {
    relaxed_select_select(begin, middle, end, comp);
    return;
  }

  auto diff = end - middle;
  relaxed_select_select(begin, begin + diff - 1, end, [&comp](auto v1, auto v2) { return !comp(v1, v2); });
  for (; end > middle;) {
    std::swap(*begin++, *--end);
  }
}

template<typename I, typename C>
void threeway_relaxed_select_select(I begin, I middle, I end, const C& comp) {
  if (middle < begin + (end - begin) / 20) {
    relaxed_select_select(begin, middle, end, comp);
  }
  else if ((end - middle) < (end - begin) / 20) {
    auto diff = end - middle;
    relaxed_select_select(begin, begin + diff - 1, end, [&comp](auto v1, auto v2) { return !comp(v1, v2); });
    for (; end > middle;) {
      std::swap(*begin++, *--end);
    }
  }
  else {
    std::array<I, 51> pivots;
    std::mt19937 engine;
    auto size = static_cast<size_t>(end - begin);
    std::generate_n(
      pivots.begin(),
      51,
      [&engine,begin,size]
        { return begin + static_cast<std::ptrdiff_t>(engine() % size); });
    auto index = ((middle - begin) * 51) / (end - begin);
    std::nth_element(
      pivots.begin(),
      pivots.begin() + index,
      pivots.end(),
      [&comp](auto p1, auto p2) { return comp(*p1, *p2); });
    auto pivot = pivots[static_cast<size_t>(index)];
    std::swap(*begin, *pivot);
    auto it = std::partition(begin + 1, end, [begin, &comp](auto val) { return comp(val, *begin); });
    std::swap(*begin, *--it);
    if (it < middle) {
      threeway_relaxed_select_select(it + 1, middle, end, comp);
    }
    else if (it > middle) {
      threeway_relaxed_select_select(begin, middle, it, comp);
    }
  }
}

#define MAKE_FUNCTION_OBJECT(algo) [](auto b, auto m, auto e, auto c) { algo(b, m, e, c); }

int main() {
  test_correctness(MAKE_FUNCTION_OBJECT(heap_select));
  test_correctness(MAKE_FUNCTION_OBJECT(select_select));
  test_correctness(MAKE_FUNCTION_OBJECT(relaxed_select_select));
  test_correctness(MAKE_FUNCTION_OBJECT(twoway_relaxed_select_select));
  test_correctness(MAKE_FUNCTION_OBJECT(threeway_relaxed_select_select));

  std::cout << "n" << ",\t" << "std::nth_element" << ",\t" << "heap_select"  << "\n";
  test_perf(
    MAKE_FUNCTION_OBJECT(std::nth_element),
    MAKE_FUNCTION_OBJECT(heap_select),
    MAKE_FUNCTION_OBJECT(select_select),
    MAKE_FUNCTION_OBJECT(relaxed_select_select),
    MAKE_FUNCTION_OBJECT(twoway_relaxed_select_select),
    MAKE_FUNCTION_OBJECT(threeway_relaxed_select_select));

  test_perf_desc(
    MAKE_FUNCTION_OBJECT(std::nth_element),
    MAKE_FUNCTION_OBJECT(heap_select),
    MAKE_FUNCTION_OBJECT(select_select),
    MAKE_FUNCTION_OBJECT(relaxed_select_select),
    MAKE_FUNCTION_OBJECT(twoway_relaxed_select_select),
    MAKE_FUNCTION_OBJECT(threeway_relaxed_select_select));
}
