#pragma once
// Feetech STS3215 packet protocol (SCS/STS format), dependency-free.
//
// The packet and register definitions here transcribe the shared
// definition implemented on the org side (SO-101 evaluation track,
// so101_hardware/protocol.py); see docs/pico_bridge_design.md. If a
// definition changes, mirror it in both places and say so in the PR.
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace sts {

inline constexpr uint8_t kBroadcastId = 0xFE;

inline constexpr uint8_t kInstPing = 0x01;
inline constexpr uint8_t kInstRead = 0x02;
inline constexpr uint8_t kInstWrite = 0x03;
inline constexpr uint8_t kInstSyncRead = 0x82;
inline constexpr uint8_t kInstSyncWrite = 0x83;

// STS3215 registers used by this project.
inline constexpr uint8_t kRegId = 5;
inline constexpr uint8_t kRegTorqueEnable = 40;
inline constexpr uint8_t kRegAcc = 41;
inline constexpr uint8_t kRegGoalPosition = 42;
inline constexpr uint8_t kRegGoalTime = 44;
inline constexpr uint8_t kRegGoalSpeed = 46;
inline constexpr uint8_t kRegLock = 55;
inline constexpr uint8_t kRegPresentPosition = 56;
inline constexpr uint8_t kRegPresentSpeed = 58;
inline constexpr uint8_t kRegPresentLoad = 60;
inline constexpr uint8_t kRegPresentVoltage = 62;
inline constexpr uint8_t kRegPresentTemperature = 63;
inline constexpr uint8_t kRegMoving = 66;

inline constexpr int kTicksPerRev = 4096;

// Checksum over ID, LEN, INST/ERR and PARAM bytes.
uint8_t checksum(const uint8_t* data, size_t len);

// Build an instruction packet: FF FF ID LEN INST PARAM... CHK.
std::vector<uint8_t> buildPacket(uint8_t id, uint8_t instruction,
                                 const std::vector<uint8_t>& params = {});

std::vector<uint8_t> pingPacket(uint8_t id);
// WRITE packet: register address followed by the data bytes.
std::vector<uint8_t> writePacket(uint8_t id, uint8_t address,
                                 const std::vector<uint8_t>& data);

struct StatusPacket {
  uint8_t id;
  uint8_t error;
  std::vector<uint8_t> params;
};

// SYNC_WRITE (broadcast): per servo, ID + data_len bytes starting at
// start_address. Each item's data must have exactly data_len bytes.
std::vector<uint8_t> syncWritePacket(
    uint8_t start_address, uint8_t data_len,
    const std::vector<std::pair<uint8_t, std::vector<uint8_t>>>& items);

// SYNC_READ request (broadcast): start_address, bytes per servo, IDs.
std::vector<uint8_t> syncReadPacket(uint8_t start_address, uint8_t data_len,
                                    const std::vector<uint8_t>& ids);

// Parse one complete status packet. Returns nullopt on a bad header,
// a length mismatch, or a checksum error.
std::optional<StatusPacket> parseStatus(const std::vector<uint8_t>& packet);

uint16_t decodeU16(const uint8_t* le);  // little-endian
void encodeU16(uint16_t value, uint8_t* le_out);

// Sign-magnitude decoding: bit `sign_bit` is the sign, the rest is the
// magnitude (position/speed use bit 15, load uses bit 10).
int decodeSignedMagnitude(uint16_t value, int sign_bit = 15);

// Incremental parser for a status-packet byte stream (the host side
// of the Pico bridge's frame recovery; see docs/pico_bridge_design.md).
// Feed bytes in any chunking; feed() returns the packets completed by
// that chunk. Garbage before a header is dropped; on a bad frame one
// byte is skipped and the scan resumes, so a corrupted packet never
// blocks the packets behind it.
class StreamParser {
 public:
  std::vector<StatusPacket> feed(const uint8_t* data, size_t len);
  std::vector<StatusPacket> feed(const std::vector<uint8_t>& chunk) {
    return feed(chunk.data(), chunk.size());
  }

 private:
  std::vector<uint8_t> buffer_;
};

}  // namespace sts
