#include "bitutils.hpp"

#define CATCH_CONFIG_MAIN
#include "include_catch.hpp"

TEST_CASE("Bit scan reversal for non-negative numbers give correct results") {
  CHECK(cwt::bit_scan_reverse_non_negative(uint64_t(1)) == 0);
  CHECK(cwt::bit_scan_reverse_non_negative(uint64_t(2)) == 1);
  CHECK(cwt::bit_scan_reverse_non_negative(uint64_t(4)) == 2);
  CHECK(cwt::bit_scan_reverse_non_negative(uint64_t(8)) == 3);
  CHECK(cwt::bit_scan_reverse_non_negative(uint64_t(16)) == 4);
  CHECK(cwt::bit_scan_reverse_non_negative(uint64_t(32)) == 5);
  CHECK(cwt::bit_scan_reverse_non_negative(uint64_t(64)) == 6);
  CHECK(cwt::bit_scan_reverse_non_negative(uint64_t(128)) == 7);
  CHECK(cwt::bit_scan_reverse_non_negative(uint64_t(256)) == 8);
  CHECK(cwt::bit_scan_reverse_non_negative(uint64_t(2*256)) == 9);
  CHECK(cwt::bit_scan_reverse_non_negative(uint64_t(4*256)) == 10);
  CHECK(cwt::bit_scan_reverse_non_negative(uint64_t(8*256)) == 11);
  CHECK(cwt::bit_scan_reverse_non_negative(uint64_t(16*256)) == 12);
  CHECK(cwt::bit_scan_reverse_non_negative(uint64_t(32*256)) == 13);
}
