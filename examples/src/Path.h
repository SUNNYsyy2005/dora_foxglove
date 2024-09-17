#include <vector>
#include <cstring> // for std::memcpy
#include <cstdint>
#include <string>
#include <iostream>
#include "./Header.h"
class Vector3 {
    public:
        double x;
        double y;
        double z;
};
class Quaternion {
    public:
        double x;
        double y;
        double z;
        double w;
};
class Pose{
    public:
        Vector3 position;
        Quaternion orientation;
};
class Path{
    public:
        Header header;
        std::vector<Pose> points;
        std::vector<uint8_t> serialize() const {
            std::vector<uint8_t> buffer = header.serialize();
            size_t points_size = points.size();
            buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&points_size), reinterpret_cast<const uint8_t*>(&points_size) + sizeof(points_size));
            for (size_t i = 0; i < points_size; i++) {
                buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&points[i].position.x), reinterpret_cast<const uint8_t*>(&points[i].position.x) + sizeof(points[i].position.x));
                buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&points[i].position.y), reinterpret_cast<const uint8_t*>(&points[i].position.y) + sizeof(points[i].position.y));
                buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&points[i].position.z), reinterpret_cast<const uint8_t*>(&points[i].position.z) + sizeof(points[i].position.z));
                buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&points[i].orientation.x), reinterpret_cast<const uint8_t*>(&points[i].orientation.x) + sizeof(points[i].orientation.x));
                buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&points[i].orientation.y), reinterpret_cast<const uint8_t*>(&points[i].orientation.y) + sizeof(points[i].orientation.y));
                buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&points[i].orientation.z), reinterpret_cast<const uint8_t*>(&points[i].orientation.z) + sizeof(points[i].orientation.z));
                buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&points[i].orientation.w), reinterpret_cast<const uint8_t*>(&points[i].orientation.w) + sizeof(points[i].orientation.w));
            }
            return buffer;
        }
        static Path deserialize(const std::vector<uint8_t>& buffer) {
            Path path;
            size_t offset = 0;
            path.header = Header::deserialize(buffer, offset);
            size_t points_size;
            std::memcpy(&points_size, buffer.data() + offset, sizeof(points_size));
            offset += sizeof(points_size);
            path.points.resize(points_size);
            for (size_t i = 0; i < points_size; i++) {
                std::memcpy(&path.points[i].position.x, buffer.data() + offset, sizeof(path.points[i].position.x));
                offset += sizeof(path.points[i].position.x);
                std::memcpy(&path.points[i].position.y, buffer.data() + offset, sizeof(path.points[i].position.y));
                offset += sizeof(path.points[i].position.y);
                std::memcpy(&path.points[i].position.z, buffer.data() + offset, sizeof(path.points[i].position.z));
                offset += sizeof(path.points[i].position.z);
                std::memcpy(&path.points[i].orientation.x, buffer.data() + offset, sizeof(path.points[i].orientation.x));
                offset += sizeof(path.points[i].orientation.x);
                std::memcpy(&path.points[i].orientation.y, buffer.data() + offset, sizeof(path.points[i].orientation.y));
                offset += sizeof(path.points[i].orientation.y);
                std::memcpy(&path.points[i].orientation.z, buffer.data() + offset, sizeof(path.points[i].orientation.z));
                offset += sizeof(path.points[i].orientation.z);
                std::memcpy(&path.points[i].orientation.w, buffer.data() + offset, sizeof(path.points[i].orientation.w));
                offset += sizeof(path.points[i].orientation.w);
            }
            return path;
        }
        static void sendPath(const Path& scan, asio::io_context& io_context, asio::ip::tcp::socket &socket) {
            asio::ip::tcp::resolver resolver(io_context);
            asio::connect(socket, resolver.resolve("127.0.0.1", std::to_string(8762)));
            std::vector<uint8_t> data = scan.serialize();
            uint32_t data_length = data.size();

            // 先发送数据长度
            asio::write(socket, asio::buffer(&data_length, sizeof(data_length)));

            // 发送完整的数据
            asio::write(socket, asio::buffer(data));
        }

        static Path receivePath(unsigned short port) {
            asio::io_context io_context;
            asio::ip::tcp::acceptor acceptor(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));
            asio::ip::tcp::socket socket(io_context);
            acceptor.accept(socket);

            // 先接收数据长度
            uint32_t data_length;
            asio::read(socket, asio::buffer(&data_length, sizeof(data_length)));

            // 接收完整的数据
            std::vector<uint8_t> buffer(data_length);
            asio::read(socket, asio::buffer(buffer));

            size_t offset = 0;
            return Path::deserialize(buffer);
        }
};