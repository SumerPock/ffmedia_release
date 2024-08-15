#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/time.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "netHandler.hpp"

/// @brief 静态变量，用于统计处理的GPS和设备包数量
uint32_t NetHandler::gpsPackets = 0;
uint32_t NetHandler::devicePackets = 0;

enum FRAME_ID_TYPE {
    FRAME_ID_UNKNOWN = 0,
    FRAME_ID_DEVICE,
    FRAME_ID_OSD,
    FRAME_ID_IP
};

#pragma pack(push, 1)
typedef struct
{
    uint16_t hh;
    uint8_t len;
    uint8_t id;
} FRAME_HEADER;

typedef struct
{
    uint32_t ipAddr;
    uint32_t ipMask;
    uint32_t gateway;
} IP_CONFIG;

typedef struct
{
    FRAME_HEADER head;
    uint32_t tv;
    uint32_t gps;
    uint32_t device;
    uint16_t crc;
} NET_RESPOND_MSG;

#pragma pack(pop)

/// @brief NetHandler构造
/// @param port
NetHandler::NetHandler(uint16_t port)
    : fd(-1),                // 文件描述符，初始化为-1，表示尚未打开
      workFlag(false),       // 工作标志，初始化为false，表示尚未开始工作
      workThread(nullptr),   // 工作线程指针，初始化为nullptr,表示线程尚未创建
      timeout(1000),         // 超时时间，初始化为1000ms
      packet(nullptr),       // 数据包指针
      packetSize(1500),      // 数据包大小，初始化为1500字节，通常是网络数据包的最大传输单元（MTU）
      dataOutputMaxLines(0), // 数据输出的最大行数，初始化为 0
      dataOutputCurLines(0), // 当前数据输出的行数，初始化为 0
      port(port)             // 端口号，初始化为传入的参数 port
{
    memset(&remotePoint, 0, sizeof remotePoint); // 将 remotePoint 结构体清零
    remotePoint.sin_family = AF_INET;            // 设置 remotePoint 的地址族为 AF_INET（IPv4）
}

/// @brief NetHandler 析构
NetHandler::~NetHandler()
{
    stop();
    if (packet)
        delete[] packet;

    if (fd >= 0)
        close(fd);
}

/// @brief 
void NetHandler::stop()
{
    workFlag = false;   // 将工作标志设置为 false，指示停止工作
    if (workThread)     // 如果工作线程已创建
    {
        workThread->join();   // 等待线程结束执行
        delete workThread;    // 释放线程对象的内存
        workThread = nullptr; // 将线程指针设为 nullptr，避免悬空指针
    }
}

void NetHandler::setNetworkDevice(const std::string& dev)
{
    netDev = dev;
}

/// @brief 设置并绑定UDP数据报套接字，准备接收网络数据包
/// @param ip
/// @param port
void NetHandler::setRemoteDeviceInfo(const std::string& ip, uint16_t port)
{
    struct in_addr ip_addr;
    if (inet_pton(AF_INET, ip.c_str(), &(ip_addr.s_addr))) {
        remotePoint.sin_addr.s_addr = ip_addr.s_addr;
    }
    remotePoint.sin_port = htons(port);
}

/// @brief 
/// @param filename 
/// @param maxLines 
void NetHandler::setDataOutputFile(const std::string& filename, int maxLines)
{
    dataOutputFile = filename;
    dataOutputMaxLines = maxLines;
    dataOutputCurLines = maxLines;
}

int NetHandler::init()
{
    if (fd >= 0)
        return 0;

    fd = setupDatagramSock();
    if (fd < 0)
        return -1;

    if (packet == nullptr)
        packet = new uint8_t[packetSize];

    return 0;
}

void NetHandler::start()
{
    workFlag = true;
    // workThread: 这是一个指向 std::thread 对象的指针，用于管理 NetHandler 的工作线程
    workThread = workThread ? workThread : new std::thread(&NetHandler::work, this);
}



void NetHandler::work()
{
    process();
}

void NetHandler::process()
{
    uint8_t* buf = packet;          //buf指向用于存储接收数据的缓冲区 packet
    size_t bufSize = packetSize;    //
    struct sockaddr_in fromAddress; //用于存储发送方的地址信息的 sockaddr_in 结构体
    int ret;                        //ret 存储 readSocket 函数的返回值，即读取的数据长度

    if (fd < 0 || buf == nullptr) {
        workFlag = false;
        return;
    }

    while (workFlag) {
        //循环处理接收的数据包
        ret = readSocket((char*)buf, bufSize, fromAddress, timeout);
        if (ret < (int)sizeof(FRAME_HEADER))
            continue;

        //数据包头部校验
        FRAME_HEADER* f = (FRAME_HEADER*)buf;
        if (f->hh != 0xaaee)  // ee aa ~
            continue;

        if (ret < f->len) {
            printf("Receive frame too small: %d < %d\n", ret, f->len);
            continue;
        }
        
        //CRC校验
        uint16_t srcCrc = *(uint16_t*)(buf + f->len - 2);
        uint16_t dstCrc = calculateCRC16X25(buf, f->len - 2);
        if (srcCrc != dstCrc) {
            printf("crc check err src crc:%x, cheack crc:%x\n", srcCrc, dstCrc);
            continue;
        }

        //数据包保存到文件
        if (!dataOutputFile.empty()) {
            if (dataOutputCurLines >= dataOutputMaxLines) {
                dataOutputTmpFile.assign(ptsToTimeStr(0, dataOutputFile.c_str()) + ".txt");
                dataOutputCurLines = 0;
                createDirectory(dataOutputTmpFile.c_str());
            }
            auto str = bytesToHexString(buf, f->len);
            if (writeStringToFile(dataOutputTmpFile, str) == 0)
                dataOutputCurLines++;
        }

        //数据包类型处理
        switch (f->id) {
            case FRAME_ID_DEVICE:
                processDeviceInfo(buf + sizeof(FRAME_HEADER), f->len - sizeof(FRAME_HEADER));
                break;
            case FRAME_ID_OSD:
                processOsdInfo(buf + sizeof(FRAME_HEADER), f->len - sizeof(FRAME_HEADER));
                break;
            case FRAME_ID_IP:
                processIPInfo(buf + sizeof(FRAME_HEADER), f->len - sizeof(FRAME_HEADER));
                break;
            default:
                break;
        }

        //发送心跳响应
        respondHeartbeatMsg(remotePoint);
    }
}

/// @brief 控制关机
/// @param buf 
/// @param bytes
void NetHandler::processDeviceInfo(uint8_t* buf, size_t bytes)
{
    const char* cmd;
    if (bytes < 1)
        return;

    if (buf[0] == 0)
        cmd = "poweroff";
    else if (buf[0] == 1)
        cmd = "poweroff";
    else
        return;

    printf("Start running command: %s\n", cmd);
    execCommand(cmd);
    return;
}

/// @brief 控制摄录程序开启/关闭/重启
/// @param buf 
/// @param bytes 
void NetHandler::processOsdInfo(uint8_t* buf, size_t bytes)
{
    const char* cmd;
    if (bytes < 1)
        return;

    if (buf[0] == 0)
        cmd = "systemctl stop ffmedia-capture.service";
    else if (buf[0] == 1)
        cmd = "systemctl start ffmedia-capture.service";
    else if（buf[0] == 2）
        cmd = "systemctl restart ffmedia-capture.service";
    else    
        return;

    printf("Start running command: %s\n", cmd);
    execCommand(cmd);
    return;
}

/// @brief IP地址修改
/// @param buf 
/// @param bytes 
void NetHandler::processIPInfo(uint8_t* buf, size_t bytes)
{
    if (bytes < sizeof(IP_CONFIG))
        return;

    char* cmd = NULL;
    char addrStr[32];
    char gatewayStr[32];
    IP_CONFIG* c = (IP_CONFIG*)buf;
    struct in_addr addr;

    uint32_t cidr = 0, mask = c->ipMask;
    while (mask) {
        if (mask & 1)
            cidr++;
        else
            break;
        mask >>= 1;
    }

    addr.s_addr = c->ipAddr;
    if (inet_ntop(AF_INET, &addr, addrStr, sizeof(addrStr)) == NULL) {
        perror("ip addr conversion failed");
        return;
    }

    addr.s_addr = c->gateway;
    if (inet_ntop(AF_INET, &addr, gatewayStr, sizeof(gatewayStr)) == NULL) {
        perror("gateway addr conversion failed");
        return;
    }

    const char* cmdFmt = "nmcli c m %s ipv4.addresses %s/%d ipv4.gateway %s";
    unsigned cmdSize = strlen(cmdFmt) + netDev.size() + strlen(addrStr)
                       + 4 /*cidr len*/
                       + strlen(gatewayStr);

    cmd = new char[cmdSize];
    sprintf(cmd, cmdFmt, netDev.c_str(), addrStr, cidr, gatewayStr);
    printf("Start running command: %s\n", cmd);
    if (execCommand(cmd) == 0) {
        sprintf(cmd, "nmcli c up %s", netDev.c_str());
        printf("Start running command: %s\n", cmd);
        execCommand(cmd);
    }
    return;
}

/// @brief 响应来自远程设备的心跳消息，并将响应数据包发送回远程设备
/// @param fromAddress
void NetHandler::respondHeartbeatMsg(struct sockaddr_in& fromAddress)
{
    NET_RESPOND_MSG msg;
    msg.head.hh = 0xbbee;  // ee bb
    msg.head.len = sizeof(NET_RESPOND_MSG);
    msg.head.id = 1;
    msg.gps = gpsPackets;
    msg.device = devicePackets;
    msg.tv = getUptime();
    msg.crc = calculateCRC16X25((const uint8_t*)&msg, msg.head.len - 2);

    //发送心跳响应消息
    auto ret = writeSocket((char*)&msg, msg.head.len, fromAddress);
    if (ret != msg.head.len)
        printf("Failed to writeSocket: %d\n", ret);

    return;
}


static int reuseFlag = 1;

/// @brief 
/// @return 
int NetHandler::setupDatagramSock()
{
    int newSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (newSocket < 0) {
        perror("unable to create datagram socket");
        return newSocket;
    }

    if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR,
                   (const char*)&reuseFlag, sizeof reuseFlag)
        < 0) {
        perror("setsockopt(SO_REUSEADDR) error: ");
        close(newSocket);
        return -1;
    }

    struct sockaddr_in c_addr;
    memset(&c_addr, 0, sizeof(c_addr));
    c_addr.sin_addr.s_addr = INADDR_ANY;
    c_addr.sin_family = AF_INET;
    c_addr.sin_port = htons(port);

    if (bind(newSocket, (struct sockaddr*)&c_addr, sizeof c_addr) != 0) {
        fprintf(stderr, "[%s] bind() error (port number: %d): ", __FUNCTION__, port);
        close(newSocket);
        return -1;
    }

    /*
        int curFlags = fcntl(sock, F_GETFL, 0);
        if (fcntl(sock, F_SETFL, curFlags | O_NONBLOCK) < 0) {
            perror("Failed to make non-blocking");
            close(newSocket);
            return -1;
        }
    */
    return newSocket;
}

int NetHandler::readSocket(char* buffer, unsigned bufferSize, struct sockaddr_in& fromAddress)
{
    int bytesRead;
    socklen_t addressSize = sizeof fromAddress;

    bytesRead = recvfrom(fd, buffer, bufferSize, 0, (struct sockaddr*)&fromAddress, &addressSize);
    return bytesRead;
}

int NetHandler::readSocket(char* buffer, unsigned bufferSize, struct sockaddr_in& fromAddress, int timeout)
{
    int bytesRead = -1;

    do {
        struct pollfd p = {.fd = fd, .events = POLLIN, .revents = 0};
        int ret = poll(&p, 1, timeout);
        if (ret < 0) {
            break;
        } else if (!(p.revents & POLLIN)) {
            bytesRead = 0;
            break;
        }

        bytesRead = readSocket(buffer, bufferSize, fromAddress);

    } while (0);

    return bytesRead;
}

int NetHandler::writeSocket(char* buffer, unsigned int bufferSize, sockaddr_in& toAddress)
{
    return sendto(fd, buffer, bufferSize, 0, (struct sockaddr*)&toAddress, sizeof(struct sockaddr_in));
}

int NetHandler::writeStringToFile(const std::string& filename, const std::string& line)
{
    std::fstream outFile(filename, std::ofstream::app);
    file.open(filename, std::fstream::app);
    if (!file)
        return -1;

    outFile << line << std::endl;
    file.close();
    return 0;
}

uint32_t crc16_x25_table1[256] = {0x0, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
                                  0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7, 0x1081, 0x108,
                                  0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e, 0x9cc9, 0x8d40, 0xbfdb, 0xae52,
                                  0xdaed, 0xcb64, 0xf9ff, 0xe876, 0x2102, 0x308b, 0x210, 0x1399, 0x6726, 0x76af,
                                  0x4434, 0x55bd, 0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
                                  0x3183, 0x200a, 0x1291, 0x318, 0x77a7, 0x662e, 0x54b5, 0x453c, 0xbdcb, 0xac42,
                                  0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974, 0x4204, 0x538d, 0x6116, 0x709f,
                                  0x420, 0x15a9, 0x2732, 0x36bb, 0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1,
                                  0xab7a, 0xbaf3, 0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x528, 0x37b3, 0x263a,
                                  0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72, 0x6306, 0x728f,
                                  0x4014, 0x519d, 0x2522, 0x34ab, 0x630, 0x17b9, 0xef4e, 0xfec7, 0xcc5c, 0xddd5,
                                  0xa96a, 0xb8e3, 0x8a78, 0x9bf1, 0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a,
                                  0x16b1, 0x738, 0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
                                  0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 0x840, 0x19c9,
                                  0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff, 0x9489, 0x8500, 0xb79b, 0xa612,
                                  0xd2ad, 0xc324, 0xf1bf, 0xe036, 0x18c1, 0x948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c,
                                  0x7df7, 0x6c7e, 0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
                                  0x2942, 0x38cb, 0xa50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd, 0xb58b, 0xa402,
                                  0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134, 0x39c3, 0x284a, 0x1ad1, 0xb58,
                                  0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c, 0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1,
                                  0xa33a, 0xb2b3, 0x4a44, 0x5bcd, 0x6956, 0x78df, 0xc60, 0x1de9, 0x2f72, 0x3efb,
                                  0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232, 0x5ac5, 0x4b4c,
                                  0x79d7, 0x685e, 0x1ce1, 0xd68, 0x3ff3, 0x2e7a, 0xe70e, 0xf687, 0xc41c, 0xd595,
                                  0xa12a, 0xb0a3, 0x8238, 0x93b1, 0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb,
                                  0xe70, 0x1ff9, 0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
                                  0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0xf78};

uint16_t NetHandler::calculateCRC16X25(const uint8_t* data, size_t length)
{
    uint16_t crc = 0xFFFF;  // init
    for (size_t i = 0; i < length; i++)
        crc = (crc >> 8) ^ crc16_x25_table1[(crc ^ data[i]) & 0x0FF];

    // crc = __be16_to_cpu(crc);
    return crc ^ 0xFFFF;  // xor
}

/// @brief
/// @param cmd 一个 C 字符串（const char*），表示要执行的命令
/// @return
int NetHandler::execCommand(const char* cmd)
{
    // system() 函数执行由 cmd 参数指定的命令，并将返回值存储在 status 变量中。
    auto status = system(cmd);
    if (status != 0) {
        //    if (status == -1 || (WIFEXITED(status) && WEXITSTATUS(status) == 127)) {
        printf("Failed to run command(%s), status: %d\n", cmd, status);
        return -1;
    }
    return 0;
}

std::string bytesToHexString(const uint8_t* data, size_t size)
{
    std::stringstream ss;
    ss << std::hex << std::uppercase;
    for (size_t i = 0; i < size; ++i) {
        ss << "0x" << static_cast<int>(data[i]) << " ";
    }
    return ss.str();
}

uint32_t getLocalIPAddressUint(const char* interface)
{
    int fd = -1;
    uint32_t ipAddr = 0;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        printf("unable to create datagram socket: %d\n", errno);
        return ipAddr;
    }

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

    if (ioctl(fd, SIOCGIFADDR, &ifr) != -1)
        ipAddr = ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr;

    close(fd);
    return ipAddr;
}

int setInterfaceIPAddress(const char* interface, uint32_t ipAddr)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        return -1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

    struct sockaddr_in* addr = (struct sockaddr_in*)&ifr.ifr_addr;
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = ipAddr;

    if (ioctl(sockfd, SIOCSIFADDR, &ifr) < 0) {
        perror("Error setting IP address");
        close(sockfd);
        return -1;
    }

    close(sockfd);
    return 0;
}