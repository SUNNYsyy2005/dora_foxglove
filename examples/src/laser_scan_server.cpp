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
#include "./PointCloud.h"
#include "./Path.h"
#include "flatbuffers/flatbuffers.h"
#include "LaserScan_generated.h"
#include "PointCloud_generated.h"
#include "PosesInFrame_generated.h"
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
PointCloud2 receivedCloud;
Path receivedPath;
foxglove::NumericType CreateType(int type) {
    switch (type) {
        case 0:
            return foxglove::NumericType_UNKNOWN;
        case 1:
            return foxglove::NumericType_UINT8;
        case 2:
            return foxglove::NumericType_INT8;
        case 3:
            return foxglove::NumericType_UINT16;
        case 4:
            return foxglove::NumericType_INT16;
        case 5:
            return foxglove::NumericType_UINT32;
        case 6:
            return foxglove::NumericType_INT32;
        case 7:
            return foxglove::NumericType_FLOAT32;
        case 8:
            return foxglove::NumericType_FLOAT64;
        default:
            return foxglove::NumericType_UNKNOWN;
    }
}
int main(int argc, char** argv) {
    std::vector<std::string> args(argv, argv + argc);
    if (args.size() < 4) {
        std::cerr << "Usage: example_server_flatbuffers /path/to/SceneUpdate.bfbs" << std::endl;
        return 1;
    }
    const auto& sceneLaserBfbsPath = args[1];
    const auto& sceneCloudBfbsPath = args[2];
    const auto& scenePathBfbsPath = args[3];
    //接受LaserScan
    std::thread receiver([&]() {
        receivedScan = LaserScan::receiveLaserScan(8764);
        
    });
    receiver.join();
    std::thread receiver2([&]() {
        receivedCloud = PointCloud2::receivePointCloud2(8763);
        
    });
    receiver2.join();
    std::thread receiver3([&]() {
        receivedPath = Path::receivePath(8762);
    });
    receiver3.join();
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
    std::signal(SIGINT, [](int sig) {
        std::cerr << "received signal " << sig << ", shutting down" << std::endl;
        running = false;
    });
    const auto channelIds = server->addChannels({{
    .topic = "laserScan_msg",
    .encoding = "flatbuffer",
    .schemaName = "foxglove.LaserScan",
    .schema = foxglove::base64Encode(getFileContents(sceneLaserBfbsPath)),
  }});
    const auto chanId = channelIds.front();
    const auto channelIds2 = server->addChannels({{
    .topic = "pointCloud_msg",
    .encoding = "flatbuffer",
    .schemaName = "foxglove.PointCloud",
    .schema = foxglove::base64Encode(getFileContents(sceneCloudBfbsPath)),
  }});
    const auto chanId2 = channelIds2.front();
    const auto channelIds3 = server->addChannels({{
    .topic = "path_msg",
    .encoding = "flatbuffer",
    .schemaName = "foxglove.Path",
    .schema = foxglove::base64Encode(getFileContents(scenePathBfbsPath)),
    }});
    // 创建 FlatBuffer 构建器
    flatbuffers::FlatBufferBuilder builder;
    builder.ForceDefaults(true);
    flatbuffers::FlatBufferBuilder builder2;
    builder2.ForceDefaults(false);
    flatbuffers::FlatBufferBuilder builder3;
    builder2.ForceDefaults(false);
    while (running) {
        builder.Clear();
        auto now = nanosecondsSinceEpoch();
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


        builder2.Clear();
        now = nanosecondsSinceEpoch();
        sec = receivedCloud.header.stamp;
        timestamp = foxglove::Time(receivedCloud.header.stamp, receivedCloud.header.stamp-sec);
        frame_id = builder2.CreateString(receivedCloud.header.frame_id);
        pose = foxglove::CreatePose(
            builder2, foxglove::CreateVector3(builder2, 0, 0, 0),
            foxglove::CreateQuaternion(builder2, 0, 0, 0, 1));
        // 设置 point_stride
        auto point_stride = receivedCloud.point_step;
        // 设置 fields
        std::vector<flatbuffers::Offset<foxglove::PackedElementField>> fields;
        for (const auto& field : receivedCloud.fields) {
            auto name = builder2.CreateString(field.name);
            auto offset = field.offset;
            auto type = CreateType(field.datatype);
            fields.push_back(foxglove::CreatePackedElementField(builder2, name, offset, type));
        }
        auto fields_vector = builder2.CreateVector(fields);

        // 设置 data
        auto data_vector = builder2.CreateVector(receivedCloud.data);
        auto point_cloud = foxglove::CreatePointCloud(
            builder2, &timestamp, frame_id, pose, point_stride, fields_vector, data_vector);
        // 完成构建
        builder2.Finish(point_cloud);
        // 发送消息
        server->broadcastMessage(chanId2, now, builder2.GetBufferPointer(), builder2.GetSize());


        builder3.Clear();
        now = nanosecondsSinceEpoch();
        sec = receivedPath.header.stamp;
        timestamp = foxglove::Time(receivedPath.header.stamp, receivedPath.header.stamp-sec);
        frame_id = builder3.CreateString(receivedPath.header.frame_id);
        pose = foxglove::CreatePose(
            builder3, foxglove::CreateVector3(builder3, 0, 0, 0),
            foxglove::CreateQuaternion(builder3, 0, 0, 0, 1));
        // 设置 poses
        std::vector<flatbuffers::Offset<foxglove::Pose>> poses;
        for (const auto& pose : receivedPath.poses) {
            auto position = foxglove::CreateVector3(builder3, pose.position.x, pose.position.y, pose.position.z);
            auto orientation = foxglove::CreateQuaternion(builder3, pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w);
            poses.push_back(foxglove::CreatePose(builder3, position, orientation));
        }
        auto poses_vector = builder3.CreateVector(poses);
        auto path = foxglove::CreatePath(builder3, &timestamp, frame_id, poses_vector);
        // 完成构建
        builder3.Finish(path);
        // 发送消息
        server->broadcastMessage(channelIds3.front(), now, builder3.GetBufferPointer(), builder3.GetSize());
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    // 停止服务器
    server->stop();

    return 0;
}
