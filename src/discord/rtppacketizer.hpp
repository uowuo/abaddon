#pragma once
#ifdef WITH_VOICE

#include <cstdint>
#include <vector>

class RTPPacketizer {
public:
    struct RTPPacket {
        std::vector<uint8_t> Data;
        bool Marker;
    };

    // Packetize H.264 NAL units according to RFC 6184
    // Returns vector of RTP packets
    static std::vector<RTPPacket> PacketizeH264(
        const uint8_t *data,
        size_t len,
        uint32_t ssrc,
        uint16_t sequence,
        uint32_t timestamp,
        uint8_t payload_type = 105 // H.264 payload type
    );

private:
    static constexpr size_t MAX_PAYLOAD_SIZE = 1100; // Conservative MTU limit
};

#endif

