#pragma once
// Minimal PCA9685 driver for the Pico (I2C, 16-channel PWM controller).
// Only what the pan-tilt mount needs: init at a fixed PWM frequency and
// set one channel's pulse by its off-count. Register values follow the
// PCA9685 datasheet.
#include <cstdint>

#include "hardware/i2c.h"
#include "pico/stdlib.h"

class Pca9685 {
 public:
  static constexpr uint8_t kDefaultAddress = 0x40;

  Pca9685(i2c_inst_t* i2c, uint8_t address = kDefaultAddress)
      : i2c_(i2c), address_(address) {}

  // Set the PWM frequency (50 Hz for hobby servos) and enable
  // auto-increment. Returns false on any I2C error.
  bool init(double freq_hz = 50.0) {
    if (!writeReg(kMode1, 0x00)) return false;  // normal mode (not a reset)
    // Prescale can only be set while MODE1.SLEEP is on.
    const uint8_t prescale =
        static_cast<uint8_t>(25.0e6 / (4096.0 * freq_hz) - 1.0 + 0.5);
    if (!writeReg(kMode1, 0x10)) return false;  // sleep
    if (!writeReg(kPrescale, prescale)) return false;
    if (!writeReg(kMode1, 0x20)) return false;  // wake, auto-increment
    busy_wait_us(1000);  // oscillator stabilisation after wake
    if (!writeReg(kMode1, 0xA0)) return false;  // restart + auto-increment
    return true;
  }

  // Set a channel fully on at count 0 and off at off_count (0..4095).
  bool setOffCount(uint8_t channel, int off_count) {
    if (channel > 15) return false;
    if (off_count < 0) off_count = 0;
    if (off_count > 4095) off_count = 4095;
    const uint8_t base = static_cast<uint8_t>(kLed0OnL + 4 * channel);
    const uint8_t data[4] = {0x00, 0x00, static_cast<uint8_t>(off_count & 0xFF),
                             static_cast<uint8_t>((off_count >> 8) & 0x0F)};
    return writeBytes(base, data, 4);
  }

 private:
  static constexpr uint8_t kMode1 = 0x00;
  static constexpr uint8_t kPrescale = 0xFE;
  static constexpr uint8_t kLed0OnL = 0x06;

  bool writeReg(uint8_t reg, uint8_t value) {
    return writeBytes(reg, &value, 1);
  }
  bool writeBytes(uint8_t reg, const uint8_t* data, int len) {
    uint8_t buf[5];
    buf[0] = reg;
    for (int i = 0; i < len; ++i) buf[1 + i] = data[i];
    return i2c_write_blocking(i2c_, address_, buf, len + 1, false) == len + 1;
  }

  i2c_inst_t* i2c_;
  uint8_t address_;
};
