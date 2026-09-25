#ifndef __PACKET_H__
#define __PACKET_H__

#include <cstdint>

constexpr const std::size_t MAX_PACKET_SIZE = 65535;

#pragma pack(push, 1)
struct PacketHeader
{
    std::uint32_t packet_id;
    std::uint32_t packet_size;
};
#pragma pack(pop)

#endif // __PACKET_H__