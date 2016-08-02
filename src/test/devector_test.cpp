#include "devector.hpp"
#include "devector_test.hpp"
#include "debug_t.hpp"
#include "debug_allocator.hpp"

#include <vector>
#include <chrono>
#include <random>
#include <deque>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

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

TEST_CASE("New devector does no allocation") {
  struct test_tag{};
  using allocator = cwt::debug_allocator<int, test_tag>;
  cwt::devector<int, allocator> dv;
  REQUIRE(allocator::total_bytes_allocated() == 0);
}

TEST_CASE("Inserting an element causes an allocation") {
  struct test_tag {};
  using allocator = cwt::debug_allocator<int, test_tag>;
  cwt::devector<int, allocator> dv;
  dv.push_back(8);
  REQUIRE(allocator::total_bytes_allocated() > 0);
}

TEST_CASE("Sanity check std::allocator") {
  auto ptr = std::allocator<int>().allocate(2);
  std::allocator<int>().deallocate(ptr, 2);
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
    cwt::devector<cwt::debug_t<test_tag>> dv;
    REQUIRE(cwt::debug_t<test_tag>::live_objects().size() == 0);

    dv.push_back(7);
    REQUIRE(*dv.begin() == 7);
    REQUIRE(dv[0] == 7);

    REQUIRE(cwt::debug_t<test_tag>::live_objects().size() == 1);
  }

  REQUIRE(cwt::debug_t<test_tag>::live_objects().size() == 0);
}

TEST_CASE("Push front constructs object and calls destructor in tear-down") {
  struct test_tag {};

  {
    cwt::devector<cwt::debug_t<test_tag>> dv;
    REQUIRE(cwt::debug_t<test_tag>::live_objects().size() == 0);

    dv.push_back(7);
    dv.push_front(9);
    REQUIRE(*dv.begin() == 9);
    REQUIRE(dv[0] == 9);

    REQUIRE(cwt::debug_t<test_tag>::live_objects().size() == 2);
  }

  REQUIRE(cwt::debug_t<test_tag>::live_objects().size() == 0);
}

TEST_CASE("Reallocation calls correct destructors") {
  struct test_tag {};

  {
    cwt::devector<cwt::debug_t<test_tag>> dv;
    REQUIRE(cwt::debug_t<test_tag>::live_objects().size() == 0);

    for (int i = 0; i < 100; ++i) {
      dv.push_back(i);
    }

    REQUIRE(dv.size() == 100);
    REQUIRE(*dv.begin() == 0);
    REQUIRE(*(dv.end() - 1) == 99);
    REQUIRE(dv[50] == 50);

    REQUIRE(cwt::debug_t<test_tag>::live_objects().size() == 100);
  }

  REQUIRE(cwt::debug_t<test_tag>::live_objects().size() == 0);
}

TEST_CASE("Reallocation calls correct destructors for push_front") {
  struct test_tag {};

  {
    cwt::devector<cwt::debug_t<test_tag>> dv;
    REQUIRE(cwt::debug_t<test_tag>::live_objects().size() == 0);

    for (int i = 0; i < 100; ++i) {
      dv.push_front(i);
    }

    REQUIRE(dv.size() == 100);
    REQUIRE(*dv.begin() == 99);
    REQUIRE(*(dv.end() - 1) == 0);
    REQUIRE(dv[50] == 49);

    REQUIRE(cwt::debug_t<test_tag>::live_objects().size() == 100);
  }

  REQUIRE(cwt::debug_t<test_tag>::live_objects().size() == 0);
}

//Allow 15% slack compared to std::vector
constexpr double perf_slack = 1.2;

TEST_CASE("Devector allocates about as often as vector") {
  int vec_allocs = count_allocations<std::vector, int>(100'000);
  int devec_allocs = count_allocations<cwt::devector, int>(100'000);
  int devec_allocs_front = count_allocations_front<cwt::devector, int>(100'000);
  
  REQUIRE(devec_allocs <= vec_allocs * perf_slack);
  REQUIRE(devec_allocs_front <= vec_allocs * perf_slack);
}

TEST_CASE("Devector move constructs about as often as vector") {
  int vec_moves = count_move_constructions<std::vector>(10'000);
  int devec_moves = count_move_constructions<cwt::devector>(10'000);

  REQUIRE(devec_moves <= vec_moves * perf_slack);
}

TEST_CASE("Devector copy constructs about as often as vector") {
  int vec_copies = count_copy_constructions<std::vector>(10'000);
  int devec_copies = count_copy_constructions<cwt::devector>(10'000);

  REQUIRE(devec_copies <= vec_copies * perf_slack);
}

TEST_CASE("Devector has about the same memory overhead as vector") {
  double vec_mem = get_average_memory_usage<std::vector, int>(100'000);
  double devec_mem = get_average_memory_usage< cwt::devector, int>(100'000);

  REQUIRE(devec_mem <= vec_mem * perf_slack);
}

TEST_CASE("Devector has about the same memory churn as vector") {
  auto vec_bytes = count_total_allocated_bytes<std::vector, int>(100'000);
  auto devec_bytes = count_total_allocated_bytes<cwt::devector, int>(100'000);

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
#endif

