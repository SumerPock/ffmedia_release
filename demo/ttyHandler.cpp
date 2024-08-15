#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>
#include <poll.h>
#include <time.h>

// #include <asm/byteorder.h>


#include "ttyHandler.hpp"

int speed_arr[] = {
    B38400,
    B19200,
    B9600,
    B4800,
    B2400,
    B1200,
    B600,
    B230400,
    B115200,
    B57600,
    B1800,
};
int name_arr[] = {
    38400,
    19200,
    9600,
    4800,
    2400,
    1200,
    600,
    230400,
    115200,
    57600,
    1800,
};


enum TTY_DATA_ID_TYPE {
    TTY_DATA_ID_UNKNOWN = 0,
    TTY_DATA_ID_GPS,
    TTY_DATA_ID_DEVICE
};

#pragma pack(1)
struct GpsInfo {
    uint16_t latitude;
    uint32_t latitude_d;
    char latitude_h;
    uint16_t longitude;
    uint32_t longitude_d;
    char longitude_h;

    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

struct RespondMsg {
    uint32_t second;
    uint32_t gps_packets;
    uint32_t device_packets;
    uint32_t ipv4_addr;
};

#pragma pack()


TtyHandler::TtyHandler(const std::string& dev, ttyPara para)
    : NetHandler(0), device(dev), para(para)
{
    packetSize = 256;
    timeDiff = -1;
    dataOutputCurLines = 0;
    dataOutputMaxLines = 0;
}

TtyHandler::~TtyHandler()
{
}

/// @brief 设置UART波特率
/// @return 
int TtyHandler::setSpeed()
{
    int i, status;
    struct termios Opt;

    tcgetattr(fd, &Opt);
    for (i = 0; i < (int)(sizeof(speed_arr) / sizeof(int)); i++) {
        if (para.speed == name_arr[i]) {
            tcflush(fd, TCIOFLUSH);
            cfsetispeed(&Opt, speed_arr[i]);
            cfsetospeed(&Opt, speed_arr[i]);
            status = tcsetattr(fd, TCSANOW, &Opt);
            if (status != 0) {
                perror("tcsetattr fd1");
                return -1;
            }
            return 0;
        }
        tcflush(fd, TCIOFLUSH);
    }
    return -1;
}

/// @brief   配置串口参数，数据位，校验位，停止位，流控制
/// @return 
int TtyHandler::setParity()
{
    struct termios options;

    if (tcgetattr(fd, &options) != 0) {
        perror("SetupSerial 1");
        return -1;
    }
    options.c_oflag &= ~OPOST;
    options.c_cflag &= ~CSIZE;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    /**设置数据位数**/
    switch (para.databits) {
        case 7:
            options.c_cflag |= CS7;
            break;
        case 8:
            options.c_cflag |= CS8;
            break;
        default:
            fprintf(stderr, "Unsupported data size\n");
            return -1;
    }
    /**设置奇偶校验**/
    switch (para.parity) {
        case 0: /*as no parity*/
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;
            break;

        case 1:
            options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/
            options.c_iflag |= INPCK;             /* Disnable parity checking */
            break;
        case 2:
            options.c_cflag |= PARENB;  /* Enable parity */
            options.c_cflag &= ~PARODD; /* 转换为偶效验*/
            options.c_iflag |= INPCK;   /* Disnable parity checking */
            break;
        case 3:
            options.c_cflag &= ~PARENB; /* Clear parity enable */
                                        //	options.c_iflag &= ~INPCK;				/* Enable parity checking */
            break;
        default:
            fprintf(stderr, "Unsupported parity\n");
            return -1;
    }
    /**设置停止位**/
    switch (para.stopbits) {
        case 1:
            options.c_cflag &= ~CSTOPB;
            break;
        case 2:
            options.c_cflag |= CSTOPB;
            break;
        default:
            fprintf(stderr, "Unsupported stop bits\n");
            return -1;
    }
    /**使能硬件流控**/
    options.c_cflag &= ~CRTSCTS;
    /**设置read函数**/
    options.c_cc[VTIME] = 10;  // 设置等待超时时间　1s
    options.c_cc[VMIN] = 1;    // 最低字节读数据

    options.c_iflag &= ~(ICRNL | IXON);
    options.c_cflag |= CLOCAL | CREAD;

    tcflush(fd, TCIFLUSH); /**Update the options and do it NOW**/
    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        perror("SetupSerial 3");
        return -1;
    }
    return 0;
}

/// @brief  串口打开
/// @return 
int TtyHandler::open()
{
    int newFd = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (newFd == -1) {
        return -1;
    }

    if (fcntl(newFd, F_SETFL, 0) < 0)
        printf("fcntl fail\n");

    return newFd;
}

/// @brief 用于从文件描述符 fd 中读取指定数量的字节数据到缓冲区 buffer。
///        它通过循环读取数据并处理可能发生的错误，以确保在可能的情况下尽量读取完整的数据
/// @param buffer   目标缓冲区的指针，数据将会被读取到这个缓冲区中
/// @param n        需要读取的字节数
/// @return
ssize_t TtyHandler::readn(uint8_t* buffer, size_t n)
{
    ssize_t num_read;
    size_t to_read;
    uint8_t* buf;
    struct pollfd p_fd = {.fd = fd, .events = POLLIN};

    buf = buffer;
    for (to_read = 0; to_read < n;) {
        if (poll(&p_fd, 1, timeout) <= 0)
            return to_read;

        num_read = read(fd, buf, n - to_read);
        if (num_read == 0)
            return to_read;
        else if (num_read == -1) {
            if (errno == EINTR)
                continue;
            else
                return -1;
        }
        to_read += num_read;
        buf += num_read;
    }
    return to_read;
}

/// @brief 向文件描述符 fd 写入指定数量的字节数据。这个函数确保尽可能多地将数据写入，
///        使在出现中断或部分写入的情况下，仍然尝试完成整个写操作
/// @param buffer
/// @param n
/// @return
ssize_t TtyHandler::writen(const uint8_t* buffer, size_t n)
{
    ssize_t num_write;
    size_t to_write;
    const uint8_t* buf;

    buf = buffer;
    for (to_write = 0; to_write < n;) {
        num_write = write(fd, buf, n - to_write);
        if (num_write <= 0) {
            if (num_write == -1 && errno == EINTR)
                continue;
            else
                return -1;
        }

        to_write += num_write;
        buf += num_write;
    }
    return to_write;
}

/// @brief   初始化串口通信的相关配置
/// @return  
int TtyHandler::init()
{
    if (fd > 0)
        return 0;

    fd = open();
    if (fd < 0) {
        printf("Failed to open serial port!\n");
        return -1;
    }

    if (setSpeed()) {
        printf("Failed to set speed\n");
        close(fd);
        fd = -1;
        return -1;
    }

    if (setParity()) {
        printf("Failed to set parity\n");
        close(fd);
        fd = -1;
        return -1;
    }

    if (packet == nullptr)
        packet = new uint8_t[packetSize];
    return 0;
}

/// @brief 处理 GPS 数据包，将其解析为结构化数据，并根据解析出的时间信息调整系统时间
/// @param buf
/// @param bytes
/// @return
int TtyHandler::processGpsInfo(uint8_t* buf, size_t bytes)
{
    GpsInfo info;
    char info_str[256];
    if (bytes < sizeof info)
        return -1;

    memcpy((uint8_t*)&info, buf, sizeof info);
    sprintf(info_str, "%c %d %0.4f : %c %d %0.4f", info.latitude_h, info.latitude,
            info.latitude_d / 10000.0, info.longitude_h, info.longitude, info.longitude_d / 10000.0);

    if (!gpsinfoOutputFile.empty()) {
        writeStringToLine(gpsinfoOutputFile, gpsinfoOutputFileLines, info_str);
    }

    if (timeDiff >= 0) {
        time_t time;
        struct timeval tv;
        struct tm timeinfo;
        memset(&timeinfo, 0, sizeof(timeinfo));
        timeinfo.tm_year = info.year + 100;  // 2000 to 1900
        timeinfo.tm_mon = info.month - 1;
        timeinfo.tm_mday = info.day;
        timeinfo.tm_hour = info.hour;
        timeinfo.tm_min = info.minute;
        timeinfo.tm_sec = info.second;
        time = mktime(&timeinfo) + 28800;  // 8 * 60 * 60s;

        gettimeofday(&tv, NULL);
        int64_t diff = std::labs(tv.tv_sec - time);
        // printf("time test device time %ld, sys time %ld, diff %ld\n", time, tv.tv_sec, diff);

        if (diff >= timeDiff >> 1) {
            int ret;
            tv.tv_sec = time;
            if (diff > timeDiff)
                ret = settimeofday(&tv, NULL);
            else
                ret = adjtime(&tv, NULL);

            if (ret != 0)
                printf("Fail to adjust time, time diff: %ld, errno: %s\n", diff, strerror(errno));
        }
    }

    return 0;
}

/// @brief 无任何意义 
/// @param buf 
/// @param bytes 
/// @return 
int TtyHandler::processDeviceInfo(uint8_t* buf, size_t bytes)
{
    return 0;
}

/// @brief  创建和发送一个心跳响应消息
/// @return
int TtyHandler::respondHeartbeatMsg()
{
    uint8_t buf[128];
    int pos = 0, frame_size = 0;

    buf[pos++] = 0xEE;
    buf[pos++] = 0xAA;

    frame_size = 4 + sizeof(RespondMsg) + 2;
    if (frame_size > 128)
        return -1;

    buf[pos++] = frame_size;
    buf[pos++] = 6;  // id

    RespondMsg msg;
    msg.second = getUptime();
    msg.gps_packets = gpsPackets;
    msg.device_packets = devicePackets;
    msg.ipv4_addr = netDev.empty() ? 0xFFFFFFFF : getLocalIPAddressUint(netDev.c_str());
    memcpy(buf + pos, (void*)&msg, sizeof msg);
    pos += sizeof msg;

    //计算并
    uint16_t crc = calculateCRC16X25(buf, frame_size - 2);
    *(uint16_t*)(buf + pos) = crc;

    //消息发送
    if (writen(buf, frame_size) != frame_size)
        return -1;

    return 0;
}

// read
// name     type 	data
// header 	ushort 	FFBB
// length 	uchar
// id 		uchar 	01|02
// data
// crc 		crc-16 	X25

/// @brief 从串口读取数据帧，进行校验和解析，然后根据数据帧的类型（如 GPS 信息或设备信息）
///        调用相应的处理函数，并返回心跳消息。它还会将处理过的数据保存到文件中
/// @param buf
/// @param bytes
/// @return
void TtyHandler::process()
{
    uint8_t* buf = packet;
    size_t bufSize = packetSize;
    int pos, readLen, frameSize;
    int ret;

    if (fd < 0 || buf == nullptr) {
        workFlag = false;
        return;
    }

    while (workFlag) {
        pos = 0;
        readLen = 1;
        frameSize = 0;

        //读取并验证帧头部分
        ret = readn(buf + pos, readLen);
        if (ret != readLen || buf[pos] != 0xFF)
            continue;

        pos += ret;
        ret = readn(buf + pos, readLen);
        if (ret != readLen || buf[pos] != 0xBB)
            continue;

        pos += ret;
        ret = readn(buf + pos, readLen);
        if (ret != readLen)
            continue;

        frameSize = buf[pos];
        pos += ret;
        readLen = frameSize - pos;

        //检查帧大小是否合法
        if ((size_t)frameSize > bufSize || readLen <= 0) {
            printf("Erron: frame_size: %d, pos: %d\n", frameSize, pos);
            continue;
        }

        //读取完整的帧内容
        ret = readn(buf + pos, readLen);
        if (ret != readLen) {
            printf("Failed to read %d buf : %d\n", readLen, ret);
            continue;
        }

        //校验CRC
        int crcPos = pos + readLen - 2;
        uint16_t srcCrc = *(uint16_t*)(buf + crcPos);
        uint16_t dstCrc = calculateCRC16X25(buf, frameSize - 2);
        if (srcCrc != dstCrc) {
            printf("crc check err src crc:%x, cheack crc:%x\n", srcCrc, dstCrc);
            continue;
        }

        // data ID 根据数据ID处理数据
        switch (buf[pos++]) {
            case TTY_DATA_ID_GPS:  // gps info
                processGpsInfo(buf + pos, readLen - 1);
                gpsPackets++;
                break;
            case TTY_DATA_ID_DEVICE:  // device info
                processDeviceInfo(buf + pos, readLen - 1);
                devicePackets++;
                break;
            default:
                break;
        }
        //发送心跳信息
        respondHeartbeatMsg();
        //将数据保存到文件
        if (!dataOutputFile.empty()) {
            if (dataOutputCurLines >= dataOutputMaxLines) {
                dataOutputTmpFile.assign(ptsToTimeStr(0, dataOutputFile.c_str()) + ".txt");
                dataOutputCurLines = 0;
                createDirectory(dataOutputTmpFile.c_str());
            }
            auto str = bytesToHexString(buf, frameSize);
            if (writeStringToFile(dataOutputTmpFile, str) == 0)
                dataOutputCurLines++;
        }
    }

    return;
}


void TtyHandler::setDataOutputFile(const std::string& filename, int maxLines)
{
    dataOutputFile = filename;
    dataOutputMaxLines = maxLines;
    dataOutputCurLines = maxLines;
}

void TtyHandler::setGpsDataOutputFile(const std::string& filename, int lineNumber)
{
    gpsinfoOutputFile = filename;
    gpsinfoOutputFileLines = lineNumber;
}

void TtyHandler::setTimeDiff(int diff)
{
    timeDiff = diff;
}


int TtyHandler::writeStringToLine(const std::string& filename, int lineNumber, const std::string& newString)
{
    std::vector<std::string> lines;

    if (lineNumber < 0) {
        lines.push_back(newString);
    } else {
        file.open(filename, std::fstream::in);
        if (!file) {
            printf("Error: Unable to open input file: %s\n", filename.c_str());
            return -1;
        }

        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }

        file.close();

        if (lineNumber >= (int)lines.size()) {
            lines.insert(lines.end(), lineNumber - lines.size(), "");
            lines.push_back(newString);
        } else {
            if (lines[lineNumber] == newString)
                return 0;
            lines[lineNumber] = newString;
        }
    }

    file.open(filename, std::fstream::out);
    if (!file) {
        printf("Error: Unable to open output file: %s\n", filename.c_str());
        return -1;
    }

    for (const std::string& l : lines) {
        file << l << std::endl;
    }

    file.close();
    return 0;
}
