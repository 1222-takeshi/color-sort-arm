// Fixed-size single-producer/single-consumer byte ring buffer for the
// Pico serial bridge (firmware/bridge_pico). Header-only and
// dependency-free so the buffer logic is host-testable; the Pico main
// wires two instances between USB CDC and the servo-bus UART.
// One slot stays empty to distinguish full from empty, so the usable
// capacity is N - 1 bytes.
#pragma once

#include <cstddef>
#include <cstdint>

namespace bridge {

template <size_t N>
class RingBuffer {
 public:
  static_assert(N >= 2, "ring buffer needs at least 2 slots");

  bool push(uint8_t byte) {
    const size_t next = (head_ + 1) % N;
    if (next == tail_) return false;  // full: drop, caller counts it
    buffer_[head_] = byte;
    head_ = next;
    return true;
  }

  bool pop(uint8_t& out) {
    if (tail_ == head_) return false;  // empty
    out = buffer_[tail_];
    tail_ = (tail_ + 1) % N;
    return true;
  }

  size_t size() const { return (head_ + N - tail_) % N; }
  bool empty() const { return head_ == tail_; }
  bool full() const { return (head_ + 1) % N == tail_; }
  static constexpr size_t capacity() { return N - 1; }

 private:
  uint8_t buffer_[N];
  size_t head_ = 0;  // write position
  size_t tail_ = 0;  // read position
};

}  // namespace bridge
