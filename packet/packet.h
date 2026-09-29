#ifndef __PACKET_H__
#define __PACKET_H__

#include <cstdint>
#include <cstddef>
#include <concepts>
#include <vector>

template <typename T>
concept PacketType = requires(const T& packet)
{
    { packet.serialize() } -> std::same_as<std::vector<std::byte>>;
};

constexpr const std::size_t MAX_PACKET_SIZE = 65535;

#pragma pack(push, 1)
struct PacketHeader
{
    std::uint32_t packet_id;
    std::uint32_t packet_size;
};
#pragma pack(pop)

#endif // __PACKET_H__