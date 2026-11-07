// Pan-tilt demo firmware for the ESP32-CAM mount (Raspberry Pi Pico).
//
// Wiring: I2C0 on GP4 (SDA) / GP5 (SCL) to the PCA9685 (address 0x40).
// Channel 0 = pan servo, channel 1 = tilt servo. Servo power (5-6 V) is
// fed to the PCA9685 V+ from a separate supply; only GND is shared with
// the Pico. Behavior: pan sweeps slowly between its limits, tilt holds
// the home angle that looks down at the workspace. Angles, limits and
// the pulse mapping live in firmware/pan_tilt (host-tested).
#include "hardware/i2c.h"
#include "pca9685.h"
#include "pico/stdlib.h"

#include "firmware/pan_tilt/pan_tilt.h"

namespace {
constexpr double kTiltHomeDeg = -45.0;   // look down at the workspace
constexpr double kPanSweepSpeed = 30.0;  // deg/s
constexpr int kStepMs = 20;              // 50 Hz update, matches PWM rate

void setAxis(Pca9685& pwm, int channel, double angle_deg,
             const pantilt::AxisConfig& axis) {
  const int pulse_us = pantilt::angleToPulseUs(angle_deg, axis);
  pwm.setOffCount(static_cast<uint8_t>(channel),
                  pantilt::pulseUsToCounts(pulse_us));
}
}  // namespace

int main() {
  stdio_init_all();
  i2c_init(i2c0, 400 * 1000);
  gpio_set_function(4, GPIO_FUNC_I2C);
  gpio_set_function(5, GPIO_FUNC_I2C);
  gpio_pull_up(4);
  gpio_pull_up(5);

  Pca9685 pwm(i2c0);
  if (!pwm.init()) {
    // No PCA9685 answering: blink the LED so the failure is visible
    // without a serial console, and keep retrying once per second.
    const uint led = PICO_DEFAULT_LED_PIN;
    gpio_init(led);
    gpio_set_dir(led, GPIO_OUT);
    while (true) {
      gpio_put(led, 1);
      sleep_ms(200);
      gpio_put(led, 0);
      sleep_ms(800);
      if (pwm.init()) break;
    }
  }

  pantilt::Sweeper pan(pantilt::kPanConfig, kPanSweepSpeed, 0.0);
  while (true) {
    const double angle = pan.update(kStepMs / 1000.0);
    setAxis(pwm, pantilt::kPanChannel, angle, pantilt::kPanConfig);
    // Re-assert tilt every cycle too, so a transient I2C error on one
    // write cannot leave the tilt servo uncommanded until reboot.
    setAxis(pwm, pantilt::kTiltChannel, kTiltHomeDeg, pantilt::kTiltConfig);
    sleep_ms(kStepMs);
  }
}
