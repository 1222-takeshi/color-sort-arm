// Pico serial bridge for the STS3215 bus (issue #5).
//
// Transparent byte forwarding between the host (USB CDC) and the
// servo bus adapter (UART), per docs/pico_bridge_design.md: the
// bridge does no framing; packet recovery lives on the host
// (sts::StreamParser). Servo-bus traffic is request/response on a
// half-duplex bus, so a polling pump with ring buffers is sufficient;
// dropped bytes (ring full) are counted and blinked out on the LED
// rather than silently lost.
//
// Wiring: UART0 GP0 (TX) / GP1 (RX) to the bus adapter's RX / TX,
// 1,000,000 baud 8N1. Servo power never comes from the Pico.
#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include "firmware/bridge/ring_buffer.h"

namespace {
constexpr uint kBaud = 1000000;
bridge::RingBuffer<4096> g_to_bus;   // host -> bus
bridge::RingBuffer<4096> g_to_host;  // bus -> host
uint32_t g_dropped = 0;

void pump() {
  // USB -> ring (non-blocking read).
  int c;
  while ((c = getchar_timeout_us(0)) >= 0) {
    if (!g_to_bus.push(static_cast<uint8_t>(c))) ++g_dropped;
  }
  // UART -> ring.
  while (uart_is_readable(uart0)) {
    if (!g_to_host.push(uart_getc(uart0))) ++g_dropped;
  }
  // Ring -> UART.
  uint8_t byte;
  while (!g_to_bus.empty() && uart_is_writable(uart0)) {
    g_to_bus.pop(byte);
    uart_putc_raw(uart0, byte);
  }
  // Ring -> USB.
  while (g_to_host.pop(byte)) putchar_raw(byte);
}
}  // namespace

int main() {
  stdio_init_all();
  uart_init(uart0, kBaud);
  gpio_set_function(0, GPIO_FUNC_UART);
  gpio_set_function(1, GPIO_FUNC_UART);
  uart_set_format(uart0, 8, 1, UART_PARITY_NONE);
  uart_set_fifo_enabled(uart0, true);

  const uint led = PICO_DEFAULT_LED_PIN;
  gpio_init(led);
  gpio_set_dir(led, GPIO_OUT);
  absolute_time_t next_blink = get_absolute_time();
  for (;;) {
    pump();
    // Heartbeat: slow blink; fast blink while drops are unreported.
    if (time_reached(next_blink)) {
      gpio_put(led, !gpio_get(led));
      const int period_ms = g_dropped > 0 ? 100 : 1000;
      if (g_dropped > 0 && period_ms == 100) g_dropped = 0;
      next_blink = delayed_by_ms(next_blink, period_ms);
    }
  }
}
