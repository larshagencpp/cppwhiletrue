#include "devector_test.hpp"
#include "devector.hpp"
#include <iostream>
#include <boost/circular_buffer.hpp>
#include <vector>

template<typename T, typename A = std::allocator<T>>
class circular {
  using buffer_t = boost::circular_buffer_space_optimized<T, A>;
public:
  circular() {
    m_buffer.set_capacity(10'000'000);
  }

  using value_type = typename buffer_t::value_type;

  void push_back(T val) {
    m_buffer.push_back(std::move(val));
  }

  void push_front(T val) {
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

  auto begin() const {
    return m_buffer.begin();
  }

  auto begin() {
    return m_buffer.begin();
  }

  auto end() const {
    return m_buffer.end();
  }

  auto end() {
    return m_buffer.end();
  }

private:
  buffer_t m_buffer;
};

int main() {
  {
    std::cout << std::endl << "integer push back total bytes" << std::endl;
    std::cout << "N, vector, devector, deque, circular" << std::endl;
    for (int N = 10; N < 100'000; N *= 2) {
      auto vector_time = count_total_allocated_bytes<std::vector, int>(N);
      auto devector_time = count_total_allocated_bytes<cwt::devector, int>(N);
      auto dequeue_time = count_total_allocated_bytes<std::deque, int>(N);
      auto circular_time = count_total_allocated_bytes<circular, int>(N);
      std::cout << N << ", " << vector_time << ", " << devector_time << ", " << dequeue_time << ", " << circular_time << std::endl;
    }
  }

  {
    std::cout << std::endl << "integer push back total allocations" << std::endl;
    std::cout << "N, vector, devector, deque, circular" << std::endl;
    for (int N = 10; N < 100'000; N *= 2) {
      auto vector_time = count_allocations<std::vector, int>(N);
      auto devector_time = count_allocations<cwt::devector, int>(N);
      auto dequeue_time = count_allocations<std::deque, int>(N);
      auto circular_time = count_allocations<circular, int>(N);
      std::cout << N << ", " << vector_time << ", " << devector_time << ", " << dequeue_time << ", " << circular_time << std::endl;
    }
  }

  {
    std::cout << std::endl << "integer push back average memory usage" << std::endl;
    std::cout << "N, vector, devector, deque, circular" << std::endl;
    for (int N = 10; N < 100'000; N *= 2) {
      auto vector_time = get_average_memory_usage<std::vector, int>(N);
      auto devector_time = get_average_memory_usage<cwt::devector, int>(N);
      auto dequeue_time = get_average_memory_usage<std::deque, int>(N);
      auto circular_time = get_average_memory_usage<circular, int>(N);
      std::cout << N << ", " << vector_time << ", " << devector_time << ", " << dequeue_time << ", " << circular_time << std::endl;
    }
  }

  {
    std::cout << std::endl << "integer push back" << std::endl;
    std::cout << "N, vector, devector, deque, circular" << std::endl;
    for (int N = 10; N < 1'000'000; N *= 2) {
      auto vector_time = get_push_back_time<std::vector<int>>(N);
      auto devector_time = get_push_back_time<cwt::devector<int>>(N);
      auto dequeue_time = get_push_back_time<std::deque<int>>(N);
      auto circular_time = get_push_back_time<circular<int>>(N);
      std::cout << N << ", " << vector_time << ", " << devector_time << ", " << dequeue_time << ", " << circular_time << std::endl;
    }
  }

  {
    std::cout << std::endl << "integer push front" << std::endl;
    std::cout << "N, devector, deque, circular" << std::endl;
    for (int N = 10; N < 1'000'000; N *= 2) {
      auto devector_time = get_push_front_time<cwt::devector<int>>(N);
      auto dequeue_time = get_push_front_time<std::deque<int>>(N);
      auto circular_time = get_push_front_time<circular<int>>(N);
      std::cout << N << ", " << devector_time << ", " << dequeue_time << ", " << circular_time << std::endl;
    }
  }

  {
    std::cout << std::endl << "integer push mixed" << std::endl;
    std::cout << "N, devector, deque, circular" << std::endl;
    for (int N = 10; N < 1'000'000; N *= 2) {
      auto devector_time = get_push_mixed_time<cwt::devector<int>>(N);
      auto dequeue_time = get_push_mixed_time<std::deque<int>>(N);
      auto circular_time = get_push_mixed_time<circular<int>>(N);
      std::cout << N << ", " << devector_time << ", " << dequeue_time << ", " << circular_time << std::endl;
    }
  }

  {
    std::cout << std::endl << "integer push pop" << std::endl;
    std::cout << "N, devector, deque, circular" << std::endl;
    for (int N = 10; N < 1'000'000; N *= 2) {
      auto devector_time = get_push_pop_time<cwt::devector<int>>(N);
      auto dequeue_time = get_push_pop_time<std::deque<int>>(N);
      auto circular_time = get_push_pop_time<circular<int>>(N);
      std::cout << N << ", " << devector_time << ", " << dequeue_time << ", " << circular_time << std::endl;
    }
  }

  {
    std::cout << std::endl << "integer iteration" << std::endl;
    std::cout << "N, vector, devector, deque, circular" << std::endl;
    for (int N = 10; N < 1'000'000; N *= 2) {
      auto vector_time = get_iteration_time<std::vector<int>>(N);
      auto devector_time = get_iteration_time<cwt::devector<int>>(N);
      auto dequeue_time = get_iteration_time<std::deque<int>>(N);
      auto circular_time = get_iteration_time<circular<int>>(N);
      std::cout << N << ", " << vector_time << ", " << devector_time << ", " << dequeue_time << ", " << circular_time << std::endl;
    }
  }

  {
    std::cout << std::endl << "integer sorting" << std::endl;
    std::cout << "N, vector, devector, deque, circular" << std::endl;
    for (int N = 10; N < 1'000'000; N *= 2) {
      auto vector_time = get_sort_time<std::vector<int>>(N);
      auto devector_time = get_sort_time<cwt::devector<int>>(N);
      auto dequeue_time = get_sort_time<std::deque<int>>(N);
      auto circular_time = get_sort_time<circular<int>>(N);
      std::cout << N << ", " << vector_time << ", " << devector_time << ", " << dequeue_time << ", " << circular_time << std::endl;
    }
  }

  {
    std::cout << std::endl << "integer max push back time" << std::endl;
    std::cout << "N, vector, devector, deque, circular" << std::endl;
    for (int N = 10; N < 1'000'000; N *= 2) {
      auto vector_time = get_max_push_back_time<std::vector<int>>(N);
      auto devector_time = get_max_push_back_time<cwt::devector<int>>(N);
      auto dequeue_time = get_max_push_back_time<std::deque<int>>(N);
      auto circular_time = get_max_push_back_time<circular<int>>(N);
      std::cout << N << ", " << vector_time << ", " << devector_time << ", " << dequeue_time << ", " << circular_time << std::endl;
    }
  }
}