#include "./LaserScan.h"
#include "./PointCloud.h"
#include "./Path.h"
#include <iostream>
#include <thread>
#include <boost/asio.hpp>

int main() {
    // 创建一个 LaserScan 对象
    LaserScan scan;
    scan.header.frame_id = "base_frame"; // 设置头信息
    scan.ranges = {1.0, 2.0, 3.0};
    scan.intensities = {0.5, 1.5, 2.5};
    PointCloud2 cloud;
    cloud.header.frame_id="base_frame"; // 设置头信息
    cloud.height = 1; // 1D 点云
    cloud.width = 4; // 100 个点
    cloud.fields = {
        {"x", 0, PointField::FLOAT32, 1},
        {"y", 4, PointField::FLOAT32, 1},
        {"z", 8, PointField::FLOAT32, 1},
    };
    cloud.is_bigendian = false;
    cloud.point_step = 12; // 每个点 12 字节
    cloud.row_step = cloud.point_step * cloud.width; // 每行的字节数
    // 示例点云数据
    float points[4][3] = {
        {1.0f, 1.0f, 1.0f},
        {2.0f, 2.0f, 2.0f},
        {3.0f, 3.0f, 3.0f},
        {4.0f, 4.0f, 4.0f}  // 第三个点 (x, y, z, intensity)
    };

    // 分配足够的空间来存储点云数据
    cloud.data.resize(cloud.row_step * cloud.height);

    // 填充点云数据
    unsigned char* data_ptr = cloud.data.data();
    for (size_t i = 0; i < cloud.width; ++i) {
        std::memcpy(data_ptr + i * cloud.point_step, points[i], cloud.point_step);
    }
    cloud.is_dense = true; // 所有点都是有效的
    std::cout<<cloud.data.size()<<std::endl;

    Path path;
    path.header.frame_id = "base_frame";
    for(int i=0;i<100;i++){
        Vector3 point;
        point.x = i/100.0;
        point.y = i/100.0;
        point.z = i/100.0;
        Quaternion orientation;
        orientation.x = 1/sqrt(3);
        orientation.y = 1/sqrt(3);
        orientation.z = 1/sqrt(3);
        orientation.w = 0;
        Pose pose;
        pose.position = point;
        pose.orientation = orientation;
        path.points.push_back(pose);
    }
    // 发送 LaserScan 对象
    std::thread sender([&]() {
        boost::asio::io_context io_context;
        boost::asio::ip::tcp::socket socket(io_context);
        
        while (true) {
            LaserScan::sendLaserScan(scan, io_context,socket);
            std::this_thread::sleep_for(std::chrono::seconds(1)); // 模拟发送间隔
        }
    });
    std::thread sender2([&]() {
        boost::asio::io_context io_context;
        boost::asio::ip::tcp::socket socket(io_context);
        while (true) {
            PointCloud2::sendPointCloud2(cloud, io_context,socket);
            std::this_thread::sleep_for(std::chrono::seconds(1)); // 模拟发送间隔
        }
    });
    std::thread sender3([&]() {
        boost::asio::io_context io_context;
        boost::asio::ip::tcp::socket socket(io_context);
        while (true) {
            Path::sendPath(path, io_context,socket);
            std::this_thread::sleep_for(std::chrono::seconds(1)); // 模拟发送间隔
        }
    });

    // 接收 LaserScan 对象
/*      std::thread receiver([&]() {
        while (true) {
            try {
                PointCloud2 receivedScan = PointCloud2::receivePointCloud2(8763);
                std::cout << "Received ranges: ";
                std::cout <<receivedScan.data.size()<< std::endl;
            } catch (const boost::system::system_error& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                // 处理接收错误，例如重新启动接收
            }
            sleep(1);
        }
    });  */

    sender.join();
    sender2.join();
    sender3.join();
   // receiver.join();

    return 0;
}