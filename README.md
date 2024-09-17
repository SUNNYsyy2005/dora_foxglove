# Dora-Foxglove Bridge by WebSocket protocol

## Intruduction
本项目采用Foxglove的WebSocket接口，将常见的ros消息类型(LaserScan、PointCloud)，通过dora传给Foxglove进行可视化展示
同时支持轨迹的可视化(轨迹定义为Pose数组)

## Instructions

### install dora


### install foxglove
```sh
sudo apt install ./foxglove-studio-*.deb
sudo apt update && sudo apt install foxglove-studio
```

### install docker
```sh
sudo apt-get remove docker docker-engine docker.io containerd runc
sudo apt install apt-transport-https ca-certificates curl software-properties-common gnupg lsb-release
curl -fsSL https://mirrors.aliyun.com/docker-ce/linux/ubuntu/gpg | sudo gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://mirrors.aliyun.com/docker-ce/linux/ubuntu $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
sudo apt update
sudo apt-get update
sudo apt install docker-ce docker-ce-cli containerd.io
```
To accelate your speed, you can nano your /etc/docker/daemon.json
```
{
  "registry-mirrors": [
    	"https://docker.m.daocloud.io"
  ]
}
```
```sh
sudo systemctl stop docker.socket
sudo systemctl stop docker
sudo systemctl start docker.socket
sudo systemctl start docker
```
### build
```sh
sudo make build
cd dora/build
cmake ..
make
```


### run
```sh
make laser_scan_server
cd dora/build
sudo ./bridge
```

