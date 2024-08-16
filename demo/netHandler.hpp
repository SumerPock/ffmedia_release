
#pragma once
#include <stdint.h>
#include <time.h>
#include <netinet/in.h>
#include <string>
#include <thread>

#include <thread>
#include <atomic>
#include <mutex>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "system_common.hpp"


class NetHandler
{
public:
    NetHandler(uint16_t port);
    virtual ~NetHandler();
    void setNetworkDevice(const std::string& dev);
    void setRemoteDeviceInfo(const std::string& ip, uint16_t port);
    void setDataOutputFile(const std::string& filename, int maxLines);

    virtual int init();
    void start();
    void stop();

protected:
    void work();

    void startHeartbeatThread();  // 启动心跳包发送线程
    void stopHeartbeatThread();   // 停止心跳包发送线程

    virtual void process();

    uint16_t calculateCRC16X25(const uint8_t* data, size_t length);
    int writeStringToFile(const std::string& filename, const std::string& line);
    int execCommand(const char* cmd);

private:
    void processDeviceInfo(uint8_t* buf, size_t bytes);
    void processOsdInfo(uint8_t* buf, size_t bytes);
    void processIPInfo(uint8_t* buf, size_t bytes);

    void respondHeartbeatMsg(struct sockaddr_in& fromAddress);

    int setupDatagramSock();
    int readSocket(char* buffer, unsigned bufferSize, struct sockaddr_in& fromAddress);
    int readSocket(char* buffer, unsigned bufferSize, struct sockaddr_in& fromAddress, int timeout);
    int writeSocket(char* buffer, unsigned int bufferSize, sockaddr_in& toAddress);

    void heartbeatLoop();  // 心跳包发送的循环函数

    std::thread heartbeatThread;  // 心跳包线程
    std::atomic<bool> heartbeatThreadRunning;  // 控制线程运行的标志
    std::mutex heartbeatMutex;    // 保护线程安全的互斥锁

protected:
    int fd;
    std::fstream file;
    bool workFlag;
    std::thread* workThread;
    int timeout;

    uint8_t* packet;
    size_t packetSize;


    static uint32_t gpsPackets;
    static uint32_t devicePackets;

    std::string netDev;
    std::string dataOutputFile;
    std::string dataOutputTmpFile;
    int dataOutputMaxLines;
    int dataOutputCurLines;

private:
    uint16_t port;
    struct sockaddr_in remotePoint;
};

uint32_t getLocalIPAddressUint(const char* interface);
int setInterfaceIPAddress(const char* interface, uint32_t ipAddr);
std::string bytesToHexString(const uint8_t* data, size_t size);
