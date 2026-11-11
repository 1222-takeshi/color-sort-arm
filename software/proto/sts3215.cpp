#include "proto/sts3215.h"

namespace sts {

uint8_t checksum(const uint8_t* data, size_t len) {
  unsigned sum = 0;
  for (size_t i = 0; i < len; ++i) sum += data[i];
  return static_cast<uint8_t>(~sum & 0xFF);
}

std::vector<uint8_t> buildPacket(uint8_t id, uint8_t instruction,
                                 const std::vector<uint8_t>& params) {
  std::vector<uint8_t> packet = {0xFF, 0xFF, id,
                                 static_cast<uint8_t>(params.size() + 2),
                                 instruction};
  packet.insert(packet.end(), params.begin(), params.end());
  packet.push_back(checksum(packet.data() + 2, packet.size() - 2));
  return packet;
}

std::vector<uint8_t> pingPacket(uint8_t id) {
  return buildPacket(id, kInstPing);
}

std::vector<uint8_t> writePacket(uint8_t id, uint8_t address,
                                 const std::vector<uint8_t>& data) {
  std::vector<uint8_t> params = {address};
  params.insert(params.end(), data.begin(), data.end());
  return buildPacket(id, kInstWrite, params);
}

std::optional<StatusPacket> parseStatus(const std::vector<uint8_t>& packet) {
  if (packet.size() < 6) return std::nullopt;
  if (packet[0] != 0xFF || packet[1] != 0xFF) return std::nullopt;
  const uint8_t length = packet[3];
  if (length < 2 || packet.size() != static_cast<size_t>(length) + 4)
    return std::nullopt;
  if (checksum(packet.data() + 2, packet.size() - 3) != packet.back())
    return std::nullopt;
  StatusPacket out;
  out.id = packet[2];
  out.error = packet[4];
  out.params.assign(packet.begin() + 5, packet.end() - 1);
  return out;
}

uint16_t decodeU16(const uint8_t* le) {
  return static_cast<uint16_t>(le[0] | (le[1] << 8));
}

void encodeU16(uint16_t value, uint8_t* le_out) {
  le_out[0] = static_cast<uint8_t>(value & 0xFF);
  le_out[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

int decodeSignedMagnitude(uint16_t value, int sign_bit) {
  const uint16_t sign_mask = static_cast<uint16_t>(1u << sign_bit);
  const int magnitude = value & (sign_mask - 1);
  return (value & sign_mask) ? -magnitude : magnitude;
}

}  // namespace sts
