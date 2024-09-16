#include <foxglove/websocket/base64.hpp>
#include <foxglove/websocket/server_factory.hpp>
#include <foxglove/websocket/websocket_notls.hpp>
#include <foxglove/websocket/websocket_server.hpp>
#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <fstream>
#include <iostream>
#include <memory>
#include <queue>
#include <thread>
#include <unordered_set>
#include <cmath>
#include "./LaserScan.h"
#include "flatbuffers/flatbuffers.h"
#include "LaserScan_generated.h"

std::atomic<bool> running = true;


static uint64_t nanosecondsSinceEpoch() {
  return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count());
}

static std::string getFileContents(std::string_view path) {
  std::ifstream infile;
  infile.open(path.data(), std::ios::binary | std::ios::in);
  if (!infile) {
    throw std::runtime_error("Could not open file " + std::string(path));
  }
  infile.seekg(0, std::ios::end);
  int length = infile.tellg();
  infile.seekg(0, std::ios::beg);
  std::string result(length, '\0');
  infile.read(result.data(), length);
  infile.close();
  return result;
}
LaserScan receivedScan;
int main(int argc, char** argv) {
    std::vector<std::string> args(argv, argv + argc);
    if (args.size() < 2) {
        std::cerr << "Usage: example_server_flatbuffers /path/to/SceneUpdate.bfbs" << std::endl;
        return 1;
    }
    //接受LaserScan
    std::thread receiver([&]() {
        receivedScan = LaserScan::receiveLaserScan(8764);
        
    });
    receiver.join();

    const auto& sceneUpdateBfbsPath = args[1];
    // 创建 WebSocket 服务器
    const auto logHandler = [](foxglove::WebSocketLogLevel, char const* msg) {
        std::cout << msg << std::endl;
    };
    foxglove::ServerOptions serverOptions;
    auto server = foxglove::ServerFactory::createServer<websocketpp::connection_hdl>(
        "C++ FlatBuffers example server", logHandler, serverOptions);
    foxglove::ServerHandlers<foxglove::ConnHandle> hdlrs;
    hdlrs.subscribeHandler = [&](foxglove::ChannelId chanId, foxglove::ConnHandle clientHandle) {
        const auto clientStr = server->remoteEndpointString(clientHandle);
        std::cout << "Client " << clientStr << " subscribed to " << chanId << std::endl;
    };
    hdlrs.unsubscribeHandler = [&](foxglove::ChannelId chanId, foxglove::ConnHandle clientHandle) {
        const auto clientStr = server->remoteEndpointString(clientHandle);
        std::cout << "Client " << clientStr << " unsubscribed from " << chanId << std::endl;
    };
    server->setHandlers(std::move(hdlrs));
    // 启动服务器
    server->start("0.0.0.0", 8765);
    const auto channelIds = server->addChannels({{
    .topic = "laserScan_msg",
    .encoding = "flatbuffer",
    .schemaName = "foxglove.LaserScan",
    .schema = foxglove::base64Encode(getFileContents(sceneUpdateBfbsPath)),
  }});
    const auto chanId = channelIds.front();
    std::signal(SIGINT, [](int sig) {
        std::cerr << "received signal " << sig << ", shutting down" << std::endl;
        running = false;
    });
    // 创建 FlatBuffer 构建器
    flatbuffers::FlatBufferBuilder builder;
    builder.ForceDefaults(true);
    while (running) {
        builder.Clear();
        const auto now = nanosecondsSinceEpoch();
        int sec = receivedScan.header.stamp;
        auto timestamp = foxglove::Time(receivedScan.header.stamp, receivedScan.header.stamp-sec);
        auto frame_id = builder.CreateString(receivedScan.header.frame_id);
        auto pose = foxglove::CreatePose(
            builder, foxglove::CreateVector3(builder, 0, 0, 0),
            foxglove::CreateQuaternion(builder, 0, 0, 0, 1));
        auto start_angle = receivedScan.angle_min;
        auto end_angle = receivedScan.angle_max;
        std::vector<float> ranges = receivedScan.ranges; // 示例范围数据
        std::vector<double> dranges(ranges.begin(), ranges.end());
        std::vector<float> intensities = receivedScan.intensities; // 示例强度数据
        std::vector<double> dintensities(intensities.begin(), intensities.end());

        auto ranges_vector = builder.CreateVector(dranges);
        auto intensities_vector = builder.CreateVector(dintensities);

        auto laser_scan = foxglove::CreateLaserScan(
            builder, &timestamp, frame_id, pose, start_angle, end_angle, ranges_vector, intensities_vector);

        builder.Finish(laser_scan);

        // 发送 LaserScan 消息
        server->broadcastMessage(chanId,now, builder.GetBufferPointer(), builder.GetSize());

        // 等待一段时间
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    // 停止服务器
    server->stop();

    return 0;
}
