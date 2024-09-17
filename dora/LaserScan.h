#include <vector>
#include <cstring> // for std::memcpy
#include <cstdint>
#include <string>
#include <boost/asio.hpp>
#include <iostream>
#include "./Header.h"

class LaserScan {
public:
    Header header;             // Header结构体实例

    float angle_min;           // 开始扫描的角度
    float angle_max;           // 结束扫描的角度
    float angle_increment;     // 每一次扫描增加的角度

    float time_increment;      // 测量的时间间隔
    float scan_time;           // 扫描的时间间隔

    float range_min;           // 距离最小值
    float range_max;           // 距离最大值

    std::vector<float> ranges;         // 距离数组
    std::vector<float> intensities;    // 强度数组

    // 序列化函数
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer = header.serialize();

        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&angle_min), reinterpret_cast<const uint8_t*>(&angle_min) + sizeof(angle_min));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&angle_max), reinterpret_cast<const uint8_t*>(&angle_max) + sizeof(angle_max));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&angle_increment), reinterpret_cast<const uint8_t*>(&angle_increment) + sizeof(angle_increment));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&time_increment), reinterpret_cast<const uint8_t*>(&time_increment) + sizeof(time_increment));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&scan_time), reinterpret_cast<const uint8_t*>(&scan_time) + sizeof(scan_time));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&range_min), reinterpret_cast<const uint8_t*>(&range_min) + sizeof(range_min));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&range_max), reinterpret_cast<const uint8_t*>(&range_max) + sizeof(range_max));

        size_t ranges_size = ranges.size();
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&ranges_size), reinterpret_cast<const uint8_t*>(&ranges_size) + sizeof(ranges_size));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(ranges.data()), reinterpret_cast<const uint8_t*>(ranges.data()) + ranges_size * sizeof(float));

        size_t intensities_size = intensities.size();
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&intensities_size), reinterpret_cast<const uint8_t*>(&intensities_size) + sizeof(intensities_size));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(intensities.data()), reinterpret_cast<const uint8_t*>(intensities.data()) + intensities_size * sizeof(float));

        return buffer;
    }

    // 反序列化函数
    static LaserScan deserialize(const std::vector<uint8_t>& buffer) {
        LaserScan scan;
        size_t offset = 0;

        scan.header = Header::deserialize(buffer, offset);

        std::memcpy(&scan.angle_min, buffer.data() + offset, sizeof(scan.angle_min));
        offset += sizeof(scan.angle_min);

        std::memcpy(&scan.angle_max, buffer.data() + offset, sizeof(scan.angle_max));
        offset += sizeof(scan.angle_max);

        std::memcpy(&scan.angle_increment, buffer.data() + offset, sizeof(scan.angle_increment));
        offset += sizeof(scan.angle_increment);

        std::memcpy(&scan.time_increment, buffer.data() + offset, sizeof(scan.time_increment));
        offset += sizeof(scan.time_increment);

        std::memcpy(&scan.scan_time, buffer.data() + offset, sizeof(scan.scan_time));
        offset += sizeof(scan.scan_time);

        std::memcpy(&scan.range_min, buffer.data() + offset, sizeof(scan.range_min));
        offset += sizeof(scan.range_min);

        std::memcpy(&scan.range_max, buffer.data() + offset, sizeof(scan.range_max));
        offset += sizeof(scan.range_max);

        size_t ranges_size;
        std::memcpy(&ranges_size, buffer.data() + offset, sizeof(ranges_size));
        offset += sizeof(ranges_size);
        scan.ranges.resize(ranges_size);
        std::memcpy(scan.ranges.data(), buffer.data() + offset, ranges_size * sizeof(float));
        offset += ranges_size * sizeof(float);

        size_t intensities_size;
        std::memcpy(&intensities_size, buffer.data() + offset, sizeof(intensities_size));
        offset += sizeof(intensities_size);
        scan.intensities.resize(intensities_size);
        std::memcpy(scan.intensities.data(), buffer.data() + offset, intensities_size * sizeof(float));

        return scan;
    }
    static void sendLaserScan(const LaserScan& scan, boost::asio::io_context& io_context, boost::asio::ip::tcp::socket &socket) {
        try {
            boost::asio::ip::tcp::resolver resolver(io_context);
            boost::asio::connect(socket, resolver.resolve("127.0.0.1", std::to_string(8764)));
            std::vector<uint8_t> data = scan.serialize();
            uint32_t data_length = data.size();

            // 先发送数据长度
            boost::asio::write(socket, boost::asio::buffer(&data_length, sizeof(data_length)));

            // 发送完整的数据
            boost::asio::write(socket, boost::asio::buffer(data));

        } catch (const boost::system::system_error& e) {
            std::cout<< "Error: " << e.what() << std::endl;
        }
    }

    static LaserScan receiveLaserScan(unsigned short port) {
        boost::asio::io_context io_context;
        boost::asio::ip::tcp::acceptor acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
        boost::asio::ip::tcp::socket socket(io_context);
        acceptor.accept(socket);

        // 先接收数据长度
        uint32_t data_length;
        boost::asio::read(socket, boost::asio::buffer(&data_length, sizeof(data_length)));

        // 接收完整的数据
        std::vector<uint8_t> buffer(data_length);
        boost::asio::read(socket, boost::asio::buffer(buffer));

        return LaserScan::deserialize(buffer);
    }
};