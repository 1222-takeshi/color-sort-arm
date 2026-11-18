#include "app/serial_transport.h"

#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>

namespace arm {
namespace {
std::string sysError(const std::string& what) {
  return what + ": " + std::strerror(errno);
}
}  // namespace

SerialTransport::SerialTransport(const std::string& device, int baud) {
  fd_ = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd_ < 0) throw std::runtime_error(sysError("open " + device));
  termios tio{};
  if (tcgetattr(fd_, &tio) != 0)
    throw std::runtime_error(sysError("tcgetattr " + device));
  cfmakeraw(&tio);
  // 1 Mbps is a standard speed on Linux (B1000000) and macOS
  // (cfsetspeed maps it through the driver ioctl).
  if (cfsetspeed(&tio, baud) != 0)
    throw std::runtime_error(sysError("cfsetspeed"));
  tio.c_cflag |= (CLOCAL | CREAD);
  tio.c_cflag &= ~CRTSCTS;
  if (tcsetattr(fd_, TCSANOW, &tio) != 0)
    throw std::runtime_error(sysError("tcsetattr " + device));
  flushInput();
}

SerialTransport::~SerialTransport() {
  if (fd_ >= 0) ::close(fd_);
}

void SerialTransport::write(const std::vector<uint8_t>& bytes) {
  size_t sent = 0;
  while (sent < bytes.size()) {
    const ssize_t n = ::write(fd_, bytes.data() + sent, bytes.size() - sent);
    if (n < 0) {
      if (errno == EAGAIN) continue;
      throw std::runtime_error(sysError("serial write"));
    }
    sent += static_cast<size_t>(n);
  }
}

std::vector<uint8_t> SerialTransport::readSome(int timeout_ms) {
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(fd_, &rfds);
  timeval tv{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
  const int ready = ::select(fd_ + 1, &rfds, nullptr, nullptr, &tv);
  if (ready <= 0) return {};
  uint8_t buf[512];
  const ssize_t n = ::read(fd_, buf, sizeof(buf));
  if (n <= 0) return {};
  return std::vector<uint8_t>(buf, buf + n);
}

void SerialTransport::flushInput() {
  if (fd_ >= 0) tcflush(fd_, TCIFLUSH);
}

}  // namespace arm
