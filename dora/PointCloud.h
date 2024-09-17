#include <vector>
#include <cstring> // for std::memcpy
#include <cstdint>
#include <string>
#include <boost/asio.hpp>
#include <iostream>
#include "./Header.h"

class PointField {
public:
    enum NumericType {
        UNKNOWN = 0,
        UINT8 = 1,
        INT8 = 2,
        UINT16 = 3,
        INT16 = 4,
        UINT32 = 5,
        INT32 = 6,
        FLOAT32 = 7,
        FLOAT64 = 8
    };
    std::string name;
    uint32_t offset;
    uint8_t datatype;
    uint32_t count;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer;
        
        uint32_t name_size = name.size();
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&name_size), reinterpret_cast<const uint8_t*>(&name_size) + sizeof(name_size));
        buffer.insert(buffer.end(), name.begin(), name.end());
        
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&offset), reinterpret_cast<const uint8_t*>(&offset) + sizeof(offset));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&datatype), reinterpret_cast<const uint8_t*>(&datatype) + sizeof(datatype));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&count), reinterpret_cast<const uint8_t*>(&count) + sizeof(count));

        return buffer;
    }

    static PointField deserialize(const std::vector<uint8_t>& buffer, size_t& offset) {
        PointField field;

        uint32_t name_size;
        std::memcpy(&name_size, buffer.data() + offset, sizeof(name_size));
        offset += sizeof(name_size);

        field.name.resize(name_size);
        std::memcpy(&field.name[0], buffer.data() + offset, name_size);
        offset += name_size;

        std::memcpy(&field.offset, buffer.data() + offset, sizeof(field.offset));
        offset += sizeof(field.offset);

        std::memcpy(&field.datatype, buffer.data() + offset, sizeof(field.datatype));
        offset += sizeof(field.datatype);

        std::memcpy(&field.count, buffer.data() + offset, sizeof(field.count));
        offset += sizeof(field.count);

        return field;
    }
    PointField& operator=(const PointField& other) {
        if (this != &other) {
            name = other.name;
            offset = other.offset;
            datatype = other.datatype;
            count = other.count;
        }
        return *this;
    }
};

class PointCloud2 {
public:
    Header header;
    uint32_t height;
    uint32_t width;
    std::vector<PointField> fields;
    bool is_bigendian;
    uint32_t point_step;
    uint32_t row_step;
    std::vector<unsigned char> data;
    bool is_dense;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer = header.serialize();

        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&height), reinterpret_cast<const uint8_t*>(&height) + sizeof(height));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&width), reinterpret_cast<const uint8_t*>(&width) + sizeof(width));

        uint32_t fields_size = fields.size();
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&fields_size), reinterpret_cast<const uint8_t*>(&fields_size) + sizeof(fields_size));
        for (const auto& field : fields) {
            std::vector<uint8_t> field_buffer = field.serialize();
            buffer.insert(buffer.end(), field_buffer.begin(), field_buffer.end());
        }

        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&is_bigendian), reinterpret_cast<const uint8_t*>(&is_bigendian) + sizeof(is_bigendian));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&point_step), reinterpret_cast<const uint8_t*>(&point_step) + sizeof(point_step));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&row_step), reinterpret_cast<const uint8_t*>(&row_step) + sizeof(row_step));

        uint32_t data_size = data.size();
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&data_size), reinterpret_cast<const uint8_t*>(&data_size) + sizeof(data_size));
        buffer.insert(buffer.end(), data.begin(), data.end());

        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&is_dense), reinterpret_cast<const uint8_t*>(&is_dense) + sizeof(is_dense));

        return buffer;
    }

    static PointCloud2 deserialize(const std::vector<uint8_t>& buffer, size_t& offset) {
        PointCloud2 cloud;

        cloud.header = Header::deserialize(buffer, offset);

        std::memcpy(&cloud.height, buffer.data() + offset, sizeof(cloud.height));
        offset += sizeof(cloud.height);

        std::memcpy(&cloud.width, buffer.data() + offset, sizeof(cloud.width));
        offset += sizeof(cloud.width);

        uint32_t fields_size;
        std::memcpy(&fields_size, buffer.data() + offset, sizeof(fields_size));
        offset += sizeof(fields_size);

        cloud.fields.resize(fields_size);
        for (auto& field : cloud.fields) {
            field = PointField::deserialize(buffer, offset);
        }

        std::memcpy(&cloud.is_bigendian, buffer.data() + offset, sizeof(cloud.is_bigendian));
        offset += sizeof(cloud.is_bigendian);

        std::memcpy(&cloud.point_step, buffer.data() + offset, sizeof(cloud.point_step));
        offset += sizeof(cloud.point_step);

        std::memcpy(&cloud.row_step, buffer.data() + offset, sizeof(cloud.row_step));
        offset += sizeof(cloud.row_step);

        uint32_t data_size;
        std::memcpy(&data_size, buffer.data() + offset, sizeof(data_size));
        offset += sizeof(data_size);

        cloud.data.resize(data_size);
        std::memcpy(cloud.data.data(), buffer.data() + offset, data_size);
        offset += data_size;

        std::memcpy(&cloud.is_dense, buffer.data() + offset, sizeof(cloud.is_dense));
        offset += sizeof(cloud.is_dense);

        return cloud;
    }

    static void sendPointCloud2(const PointCloud2& scan, boost::asio::io_context& io_context, boost::asio::ip::tcp::socket &socket) {
        try {
            boost::asio::ip::tcp::resolver resolver(io_context);
            boost::asio::connect(socket, resolver.resolve("127.0.0.1", std::to_string(8763)));
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

    static PointCloud2 receivePointCloud2(unsigned short port) {
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

        size_t offset = 0;
        return PointCloud2::deserialize(buffer, offset);
    }
};