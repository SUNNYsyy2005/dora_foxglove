#pragma once

#ifndef _HEADER_H_
#define _HEADER_H_
#include <vector>
#include <cstring> // for std::memcpy
#include <cstdint>
#include <string>
class Header {
public:
    uint32_t seq;
    double stamp;
    std::string frame_id;
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer;
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&seq), reinterpret_cast<const uint8_t*>(&seq) + sizeof(seq));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&stamp), reinterpret_cast<const uint8_t*>(&stamp) + sizeof(stamp));
        
        uint32_t frame_id_size = frame_id.size();
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&frame_id_size), reinterpret_cast<const uint8_t*>(&frame_id_size) + sizeof(frame_id_size));
        buffer.insert(buffer.end(), frame_id.begin(), frame_id.end());

        return buffer;
    }

    static Header deserialize(const std::vector<uint8_t>& buffer, size_t& offset) {
        Header header;
        std::memcpy(&header.seq, buffer.data() + offset, sizeof(header.seq));
        offset += sizeof(header.seq);

        std::memcpy(&header.stamp, buffer.data() + offset, sizeof(header.stamp));
        offset += sizeof(header.stamp);

        uint32_t frame_id_size;
        std::memcpy(&frame_id_size, buffer.data() + offset, sizeof(frame_id_size));
        offset += sizeof(frame_id_size);

        header.frame_id.resize(frame_id_size);
        std::memcpy(&header.frame_id[0], buffer.data() + offset, frame_id_size);
        offset += frame_id_size;

        return header;
    }
};
#endif // _HEADER_H_