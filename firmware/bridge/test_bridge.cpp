// Host tests for the bridge ring buffer. Assert-style, no dependencies.
#include <cstdio>

#include "firmware/bridge/ring_buffer.h"

namespace {
int g_checks = 0;
int g_failures = 0;
void report(bool ok, const char* expr, int line) {
  ++g_checks;
  if (!ok) {
    ++g_failures;
    std::printf("FAIL (line %d): %s\n", line, expr);
  }
}
#define CHECK(cond) report(static_cast<bool>(cond), #cond, __LINE__)

void testBasic() {
  bridge::RingBuffer<8> rb;
  uint8_t out = 0;
  CHECK(rb.empty() && rb.size() == 0);
  CHECK(!rb.pop(out));
  CHECK(rb.push(0x11) && rb.push(0x22));
  CHECK(rb.size() == 2);
  CHECK(rb.pop(out) && out == 0x11);
  CHECK(rb.pop(out) && out == 0x22);
  CHECK(rb.empty());
}

void testFullAndWrap() {
  bridge::RingBuffer<8> rb;  // usable capacity 7
  for (int i = 0; i < 7; ++i) CHECK(rb.push(static_cast<uint8_t>(i)));
  CHECK(rb.full());
  CHECK(!rb.push(0xFF));  // full: rejected, nothing overwritten
  uint8_t out = 0;
  for (int i = 0; i < 3; ++i) {
    CHECK(rb.pop(out) && out == i);
  }
  // Wrap the write position around the end of the array.
  for (int i = 7; i < 10; ++i) CHECK(rb.push(static_cast<uint8_t>(i)));
  CHECK(rb.full());
  for (int i = 3; i < 10; ++i) {
    CHECK(rb.pop(out) && out == i);
  }
  CHECK(rb.empty());
}
}  // namespace

int main() {
  testBasic();
  testFullAndWrap();
  std::printf("%d checks, %d failure(s)\n", g_checks, g_failures);
  if (g_failures == 0) std::printf("ALL TESTS PASSED\n");
  return g_failures == 0 ? 0 : 1;
}
