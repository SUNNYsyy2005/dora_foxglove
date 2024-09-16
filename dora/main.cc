#include "./LaserScan.h"
#include <iostream>
#include <thread>
#include <boost/asio.hpp>

int main() {
    // 创建一个 LaserScan 对象
    LaserScan scan;
    scan.ranges = {1.0, 2.0, 3.0};
    scan.intensities = {0.5, 1.5, 2.5};

    // 发送 LaserScan 对象
    std::thread sender([&]() {
        boost::asio::io_context io_context;
        boost::asio::ip::tcp::socket socket(io_context);
        
        while (true) {
            LaserScan::sendLaserScan(scan, io_context,socket);
            std::this_thread::sleep_for(std::chrono::seconds(1)); // 模拟发送间隔
        }
    });

    // 接收 LaserScan 对象
/*     std::thread receiver([&]() {
        while (true) {
            try {
                LaserScan receivedScan = LaserScan::receiveLaserScan(8764);
                std::cout << "Received ranges: ";
                for (float range : receivedScan.ranges) {
                    std::cout << range << " ";
                }
                std::cout << std::endl;

                std::cout << "Received intensities: ";
                for (float intensity : receivedScan.intensities) {
                    std::cout << intensity << " ";
                }
                std::cout << std::endl;
            } catch (const boost::system::system_error& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                // 处理接收错误，例如重新启动接收
            }
            sleep(1);
        }
    }); */

    sender.join();
//    receiver.join();

    return 0;
}