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

std::vector<uint8_t> syncWritePacket(
    uint8_t start_address, uint8_t data_len,
    const std::vector<std::pair<uint8_t, std::vector<uint8_t>>>& items) {
  std::vector<uint8_t> params = {start_address, data_len};
  for (const auto& item : items) {
    params.push_back(item.first);
    params.insert(params.end(), item.second.begin(), item.second.end());
  }
  return buildPacket(kBroadcastId, kInstSyncWrite, params);
}

std::vector<uint8_t> syncReadPacket(uint8_t start_address, uint8_t data_len,
                                    const std::vector<uint8_t>& ids) {
  std::vector<uint8_t> params = {start_address, data_len};
  params.insert(params.end(), ids.begin(), ids.end());
  return buildPacket(kBroadcastId, kInstSyncRead, params);
}

std::vector<StatusPacket> StreamParser::feed(const uint8_t* data, size_t len) {
  buffer_.insert(buffer_.end(), data, data + len);
  std::vector<StatusPacket> out;
  for (;;) {
    size_t header = buffer_.size();
    for (size_t i = 0; i + 1 < buffer_.size(); ++i) {
      if (buffer_[i] == 0xFF && buffer_[i + 1] == 0xFF) {
        header = i;
        break;
      }
    }
    if (header == buffer_.size()) {
      const bool keep = !buffer_.empty() && buffer_.back() == 0xFF;
      buffer_.clear();
      if (keep) buffer_.push_back(0xFF);
      break;
    }
    if (header > 0)
      buffer_.erase(buffer_.begin(), buffer_.begin() + header);
    if (buffer_.size() < 4) break;
    const size_t total = static_cast<size_t>(buffer_[3]) + 4;
    if (buffer_[3] < 2 || total > 68) {  // implausible LEN: resync
      buffer_.erase(buffer_.begin());
      continue;
    }
    if (buffer_.size() < total) break;
    const std::vector<uint8_t> frame(buffer_.begin(),
                                     buffer_.begin() + total);
    if (const auto st = parseStatus(frame)) {
      out.push_back(*st);
      buffer_.erase(buffer_.begin(), buffer_.begin() + total);
    } else {
      buffer_.erase(buffer_.begin());
    }
  }
  return out;
}

}  // namespace sts
