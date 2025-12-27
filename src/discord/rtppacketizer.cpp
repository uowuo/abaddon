#ifdef WITH_VOICE

#include "rtppacketizer.hpp"
#include <cstring>

std::vector<RTPPacketizer::RTPPacket> RTPPacketizer::PacketizeH264(
    const uint8_t *data,
    size_t len,
    uint32_t ssrc,
    uint16_t sequence,
    uint32_t timestamp,
    uint8_t payload_type) {

    std::vector<RTPPacket> packets;

    if (len == 0) return packets;

    // Extract NAL unit type (first 5 bits of first byte)
    uint8_t nal_type = data[0] & 0x1F;
    uint8_t f_nri = data[0] & 0xE0; // F and NRI bits

    if (len <= MAX_PAYLOAD_SIZE) {
        // Single NAL Unit Packet (RFC 6184 section 5.6)
        RTPPacket packet;
        packet.Marker = true;
        packet.Data.resize(12 + len);

        // RTP Header
        packet.Data[0] = 0x80; // Version 2, no padding, no extension, no CSRC
        packet.Data[1] = payload_type & 0x7F; // Payload type
        packet.Data[2] = (sequence >> 8) & 0xFF;
        packet.Data[3] = sequence & 0xFF;
        packet.Data[4] = (timestamp >> 24) & 0xFF;
        packet.Data[5] = (timestamp >> 16) & 0xFF;
        packet.Data[6] = (timestamp >> 8) & 0xFF;
        packet.Data[7] = timestamp & 0xFF;
        packet.Data[8] = (ssrc >> 24) & 0xFF;
        packet.Data[9] = (ssrc >> 16) & 0xFF;
        packet.Data[10] = (ssrc >> 8) & 0xFF;
        packet.Data[11] = ssrc & 0xFF;

        // Copy NAL unit
        std::memcpy(packet.Data.data() + 12, data, len);

        packets.push_back(packet);
    } else {
        // FU-A Fragmentation (RFC 6184 section 5.8)
        size_t offset = 1; // Skip NAL header
        size_t remaining = len - 1;
        bool first = true;

        while (remaining > 0) {
            RTPPacket packet;
            size_t fragment_size = remaining > (MAX_PAYLOAD_SIZE - 2) ? (MAX_PAYLOAD_SIZE - 2) : remaining;
            bool last = (remaining <= MAX_PAYLOAD_SIZE - 2);

            packet.Marker = last;
            packet.Data.resize(12 + 2 + fragment_size);

            // RTP Header
            packet.Data[0] = 0x80;
            packet.Data[1] = (payload_type & 0x7F) | (last ? 0x80 : 0x00); // Marker bit
            packet.Data[2] = (sequence >> 8) & 0xFF;
            packet.Data[3] = sequence & 0xFF;
            packet.Data[4] = (timestamp >> 24) & 0xFF;
            packet.Data[5] = (timestamp >> 16) & 0xFF;
            packet.Data[6] = (timestamp >> 8) & 0xFF;
            packet.Data[7] = timestamp & 0xFF;
            packet.Data[8] = (ssrc >> 24) & 0xFF;
            packet.Data[9] = (ssrc >> 16) & 0xFF;
            packet.Data[10] = (ssrc >> 8) & 0xFF;
            packet.Data[11] = ssrc & 0xFF;

            // FU Indicator (F + NRI from original NAL)
            packet.Data[12] = f_nri | 28; // FU-A type

            // FU Header
            packet.Data[13] = nal_type;
            if (first) {
                packet.Data[13] |= 0x80; // S bit
                first = false;
            }
            if (last) {
                packet.Data[13] |= 0x40; // E bit
            }

            // Copy fragment
            std::memcpy(packet.Data.data() + 14, data + offset, fragment_size);

            packets.push_back(packet);

            offset += fragment_size;
            remaining -= fragment_size;
            sequence++;
        }
    }

    return packets;
}

#endif

