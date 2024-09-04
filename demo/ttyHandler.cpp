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
//邓使用此文件进行串口配置

#include "demo_comm.hpp"

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

/// @brief 根据用户指定的参数设置终端设备的波特率
/// @return
int TtyHandler::setSpeed()
{
    int i, status;
    struct termios Opt; // 存储终端的当前配置

    // fd 是文件描述符，代表串行端口或其他终端设备
    tcgetattr(fd, &Opt); // 使用 tcgetattr 函数获取文件描述符 fd 当前的终端属性，并将其存储到 Opt 结构体中
    for (i = 0; i < (int)(sizeof(speed_arr) / sizeof(int)); i++)
    { // 遍历 speed_arr 数组中的每个元素，这个数组包含了可能的波特率设置值
        if (para.speed == name_arr[i]) 
        {
            tcflush(fd, TCIOFLUSH); // tcflush(fd, TCIOFLUSH) 清空终端设备的输入和输出缓冲区
            cfsetispeed(&Opt, speed_arr[i]); // 使用 cfsetispeed 和 cfsetospeed 函数分别设置终端设备的输入和输出速度
            cfsetospeed(&Opt, speed_arr[i]);
            status = tcsetattr(fd, TCSANOW, &Opt); // 使用 tcsetattr(fd, TCSANOW, &Opt) 将新的设置立即应用到终端设备上
            if (status != 0) 
            {
                perror("tcsetattr fd1");
                return -1;
            }
            return 0;
        }
        // 在循环中，tcflush(fd, TCIOFLUSH) 被调用多次，可能是在每次未找到匹配的波特率时，清空缓冲区，确保设备处于干净的状态
        tcflush(fd, TCIOFLUSH);
    }
    return -1;
}

/// @brief   配置串口参数，数据位，校验位，停止位，流控制
/// @return 
int TtyHandler::setParity()
{
    struct termios options; // 存储终端的当前配置

    if (tcgetattr(fd, &options) != 0) // 使用 tcgetattr 函数获取文件描述符 fd 的当前终端属性并存储到 options 中
    {
        perror("SetupSerial 1");
        return -1;
    }
    options.c_oflag &= ~OPOST; // 禁用输出处理（OPOST），防止系统对输出数据进行处理
    options.c_cflag &= ~CSIZE; // 清除字符大小位（CSIZE），为后续设置数据位大小做准备
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // 禁用规范模式（ICANON）、回显（ECHO、ECHOE）和信号处理（ISIG）
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
    options.c_cflag &= ~CRTSCTS; // 禁用硬件流控
    /**设置read函数**/
    options.c_cc[VTIME] = 10;  // 设置等待超时时间　1s，VTIME 为等待时间，单位为十分之一秒（即 1 秒）
    options.c_cc[VMIN] = 1;    // 最低字节读数据，VMIN 为最小读取字节数（设为 1，表示至少读取一个字节）

    options.c_iflag &= ~(ICRNL | IXON); // 禁用输入 CR 到 NL 映射（ICRNL）和 XON/XOFF 流控（IXON）。
    options.c_cflag |= CLOCAL | CREAD;  // 设置 CLOCAL 和 CREAD 标志，忽略调制解调器状态行并启用接收器

    // 使用 tcflush(fd, TCIFLUSH) 清除终端的输入缓冲区，确保新的设置立即生效
    tcflush(fd, TCIFLUSH); /**Update the options and do it NOW**/
    if (tcsetattr(fd, TCSANOW, &options) != 0) // 使用 tcsetattr(fd, TCSANOW, &options) 立即应用新的终端设置
    {
        perror("SetupSerial 3");
        return -1;
    }
    return 0;
}

/// @brief  open 函数是 TtyHandler 类的一个成员函数，用于打开一个指定的终端设备（TTY）。
///         它尝试以读写模式打开设备文件，并进行必要的设置
/// @return
int TtyHandler::open()
{
    // 使用全局命名空间下的 ::open 函数，尝试打开设备文件。device 是一个字符串，包含设备文件的路径（如 /dev/ttyS0）
    // O_RDWR：以读写模式打开设备，
    // O_NOCTTY：不要将这个设备分配为控制终端。即使当前进程没有控制终端，也不会试图将这个终端分配为控制终端
    // O_NDELAY：以非阻塞模式打开设备。如果设备不可用，open 函数不会阻塞进程。
    int newFd = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (newFd == -1) 
    {
        return -1;
    }
    // 使用 fcntl 函数设置文件描述符的状态标志
    // F_SETFL：设置文件状态标志
    // 0：清除所有非阻塞标志（如 O_NDELAY），将设备切换回阻塞模式
    if (fcntl(newFd, F_SETFL, 0) < 0)
        printf("fcntl fail\n");
    // 返回文件描述符
    return newFd;
}

/// @brief 用于从文件描述符 fd 中读取指定数量的字节数据到缓冲区 buffer。
///        它通过循环读取数据并处理可能发生的错误，以确保在可能的情况下尽量读取完整的数据
/// @param buffer   目标缓冲区的指针，数据将会被读取到这个缓冲区中
/// @param n        需要读取的字节数
/// @return
ssize_t TtyHandler::readn(uint8_t* buffer, size_t n)
{
    ssize_t num_read;   // 每次调用 read 函数读取的字节数
    size_t to_read;     // 记录到目前为止已经读取的总字节数
    uint8_t *buf;       // 指向 buffer 的指针，用于读取数据
    struct pollfd p_fd = 
    {
        .fd = fd,
        .events = POLLIN
    };

    buf = buffer; // buf 被初始化为指向 buffer 的开始位置，准备接收读取的数据
    for (to_read = 0; to_read < n;) // 循环持续进行，直到 to_read 等于 n（即已读取请求的字节数）
    {
        if (poll(&p_fd, 1, timeout) <= 0)//轮询数据可用性
            return to_read;
        // 使用 read 函数尝试从文件描述符 fd 读取最多 n - to_read 字节的数据到缓冲区 buf 中。
        num_read = read(fd, buf, n - to_read);

        if (num_read == 0) // 文件结束（EOF）或无更多数据
            return to_read;
        else if (num_read == -1) 
        {
            if (errno == EINTR)//如果 errno 是 EINTR，说明读取操作被信号中断，循环应该继续重试读取
                continue;
            else
                return -1;
        }
        to_read += num_read; // 增加 to_read 以包含本次读取的字节数（num_read）。
        buf += num_read;     // 更新 buf 指针，向前移动 num_read 字节，指向下一个将要写入数据的位置
    }
    return to_read;
}

/// @brief 用于将指定数量的字节（n）从缓冲区（buffer）写入到文件描述符（fd）对应的终端设备。
///        这个函数使用循环和错误处理机制，确保即使在中断情况下（例如信号打断）也能继续尝试写入，直到所有数据都被写入为止
/// @param buffer
/// @param n
/// @return
ssize_t TtyHandler::writen(const uint8_t* buffer, size_t n)
{
    ssize_t num_write;  // 每次调用 write 函数实际写入的字节数
    size_t to_write;    // 记录到目前为止已经写入的总字节数
    const uint8_t *buf; // 指向 buffer 的指针，用于写入数据

    buf = buffer; // 指向 buffer 的开始位置，准备写入数据
    for (to_write = 0; to_write < n;) // 循环持续进行，直到 to_write 等于 n（即已经写入请求的字节数）
    {
        // 使用 write 函数尝试将最多 n - to_write 字节的数据从缓冲区 buf 写入文件描述符 fd
        num_write = write(fd, buf, n - to_write);
        if (num_write <= 0) // 写入失败或未写入任何数据
        {
            if (num_write == -1 && errno == EINTR) // 表示写入操作被信号中断，循环应继续重试写入
                continue;
            else
                return -1;
        }

        to_write += num_write; // 增加 to_write 以包含本次写入的字节数
        buf += num_write;      // 更新 buf 指针，向前移动 num_write 字节，指向下一个将要写入的数据位置
    }
    return to_write;
}

/// @brief   初始化串口通信的相关配置
/// @return  
int TtyHandler::init()
{
    if (fd > 0)
        return 0;

    // 调用 open() 函数尝试打开串行端口，返回一个文件描述符 fd
    fd = open();
    if (fd < 0) {
        printf("Failed to open serial port!\n");
        return -1;
    }

    // 调用 setSpeed() 函数设置串行端口的通信速度
    if (setSpeed()) {
        printf("Failed to set speed\n");
        close(fd);
        fd = -1;
        return -1;
    }
    // 调用 setParity() 函数设置串行端口的校验方式（如奇偶校验）
    if (setParity()) {
        printf("Failed to set parity\n");
        close(fd);
        fd = -1;
        return -1;
    }

    if (packet == nullptr) // 检查 packet 是否为空指针（即是否未分配内存）
        packet = new uint8_t[packetSize]; // 如果 packet 为 nullptr，使用 new 分配一个新的字节数组，大小为 packetSize，用于存储数据包
    return 0;
}

/// @brief 处理 GPS 数据包，将其解析为结构化数据，并根据解析出的时间信息调整系统时间
/// @param buf
/// @param bytes
/// @return
int TtyHandler::processGpsInfo(uint8_t* buf, size_t bytes)
{
    GpsInfo info;               // 用于存储 GPS 信息
    char info_str[256];         // 用于存储格式化后的 GPS 信息字符串
    if (bytes < sizeof info)    // 检查传入的字节数 bytes 是否小于 GpsInfo 结构体的大小
        return -1;
    // 使用 memcpy 将接收到的 GPS 数据从 buf 缓冲区复制到 info 结构体中
    memcpy((uint8_t*)&info, buf, sizeof info);
    // 使用 sprintf 函数将 info 中的 GPS 信息格式化为易读的字符串形式并存储到 info_str 中
    sprintf(info_str,
            "%c %d %0.4f : %c %d %0.4f",
            info.latitude_h, 
            info.latitude,
            info.latitude_d / 10000.0, 
            info.longitude_h, 
            info.longitude, 
            info.longitude_d / 10000.0);

    if (!gpsinfoOutputFile.empty()) // 检查 gpsinfoOutputFile 是否为空。如果不为空，说明需要将 GPS 信息写入文件
    {                               // 调用 writeStringToLine 函数，将 info_str 写入指定的文件中
        writeStringToLine(gpsinfoOutputFile, gpsinfoOutputFileLines, info_str);
    }

    if (timeDiff >= 0) 
    {
        time_t time;
        struct timeval tv;
        struct tm timeinfo;
        memset(&timeinfo, 0, sizeof(timeinfo));

        // struct tm 结构体的 tm_year 成员表示的是自 1900 年以来的年数
        // 如果 info.year 表示的年份是从 2000 年开始的年份数（例如，如果 info.year 是 23，表示 2023 年），
        // 则需要将 info.year 转换为自 1900 年以来的年数
        // 因此，通过 info.year + 100 可以将年份调整为 struct tm 期望的格式。
        // 例如，info.year = 23（表示 2023 年）时，tm_year 应该设置为 123（2023 - 1900 = 123）。
        timeinfo.tm_year = info.year + 100;  // 2000 to 1900

        // struct tm 结构体的 tm_mon 成员表示月份，但它的范围是从 0 到 11（即，0 表示 1 月，1 表示 2 月，以此类推)
        timeinfo.tm_mon = info.month - 1;

        timeinfo.tm_mday = info.day;
        timeinfo.tm_hour = info.hour;
        timeinfo.tm_min = info.minute;
        timeinfo.tm_sec = info.second;

        //将所有的时间转换为秒，进行计算
        time = mktime(&timeinfo) + 28800; // 8 * 60 * 60s; 28800 秒，等于 8 小时，可能用于调整时区

        gettimeofday(&tv, NULL); // 使用 gettimeofday 获取当前系统时间
        int64_t diff = std::labs(tv.tv_sec - time); // 并计算设备时间与系统时间的差异 diff
        // printf("time test device time %ld, sys time %ld, diff %ld\n", time, tv.tv_sec, diff);

        if (diff >= timeDiff >> 1) // 如果时间差 diff 超过 timeDiff 的一半
        {
            int ret;
            tv.tv_sec = time;
            // 其中settimeofday用于设置系统的当前时间。它可以直接更改系统时钟的时间，立即将系统时间设置为用户指定的时间值
            // 当你需要立即将系统时间更改为特定的时间时使用。适用于需要快速同步时间的场合，例如调整时间以纠正明显的时间错误

            // 而用于逐渐调整系统时间，使系统时间以更平滑的方式逐步接近指定的时间。它不是立即更改时间，而是调整时钟速度，使时间缓慢加快或减慢
            // 当你需要平滑调整系统时间而不引起时间跳跃时使用。适用于网络时间协议（NTP）守护进程，它使用 adjtime 来校准系统时钟，
            // 使其缓慢同步到正确的时间，而不会干扰系统中的其他时间依赖操作
            if (diff > timeDiff) // 如果时间差大于 timeDiff，使用 settimeofday 直接设置系统时间
                ret = settimeofday(&tv, NULL);
            else
                ret = adjtime(&tv, NULL); // 使用 adjtime 调整时间，慢慢校准到正确时间

            if (ret != 0)
                printf("Fail to adjust time, time diff: %ld, errno: %s\n", diff, strerror(errno));
        }
    }
    return 0;
}

/// @brief 无任何意义 ，我的想法是将这包数据解析，存入队列中，让网口程序发送他
/// @param buf 
/// @param bytes 
/// @return 
int TtyHandler::processDeviceInfo(uint8_t* buf, size_t bytes)
{
    // 将接收到的缓冲区数据转换为字符串或其他适当的数据格式
    std::string data(reinterpret_cast<char *>(buf), bytes);

    // 将数据放入消息队列
    messageQueue.push(data);

    return 0;
}

/// @brief  创建和发送一个心跳响应消息,这个消息时RK向STM32发送的
/// @return
int TtyHandler::respondHeartbeatMsg()
{
    uint8_t buf[128];
                                 // pos 是当前写入缓冲区的偏移量
    int pos = 0, frame_size = 0; // frame_size 当前写入缓冲区的偏移量

    //设置消息头部
    buf[pos++] = 0xEE;
    buf[pos++] = 0xAA;

    //2字头的头 + 帧大小 ID + 整体消息 + 2字节的CRC校验
    frame_size = 4 + sizeof(RespondMsg) + 2;
    if (frame_size > 128)
        return -1;
    //长度及ID信息
    buf[pos++] = frame_size;
    buf[pos++] = 6;  // id

    RespondMsg msg;
    msg.second = getUptime(); // 获取设备的运行时间（系统启动后的秒数）
    msg.gps_packets = gpsPackets;//GPS包
    msg.device_packets = devicePackets;//设备数据包
    //获取本地网络接口的IPV4地址信息
    msg.ipv4_addr = netDev.empty() ? 0xFFFFFFFF : getLocalIPAddressUint(netDev.c_str());

    memcpy(buf + pos, (void*)&msg, sizeof msg);
    pos += sizeof msg;

    //计算并添加CRC效验码
    uint16_t crc = calculateCRC16X25(buf, frame_size - 2);
    *(uint16_t*)(buf + pos) = crc;

    // 调用 writen 函数将构建的消息写入到文件描述符 fd（通常是串行端口或网络连接）中
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
    uint8_t *buf = packet; // 指向存储接收数据的缓冲区 packet
    size_t bufSize = packetSize;

    // pos 当前缓冲区的位置指针，用于读取数据。readLen 是当前要读取的数据长度 。frameSize 是当前数据帧的总长度
    int pos, readLen, frameSize;
    int ret;

    if (fd < 0 || buf == nullptr)
    { // 如果 fd（文件描述符）无效或 buf（缓冲区指针）为空，关闭工作标志 workFlag 并返回，不继续处理
        workFlag = false;
        return;
    }

    while (workFlag) 
    {
        pos = 0;
        readLen = 1;
        frameSize = 0;

        //读取并验证帧头部分
        ret = readn(buf + pos, readLen);
        // 先读取一个字节，检查是否为 0xFF，如果不是，则继续下一次循环
        if (ret != readLen || buf[pos] != 0xFF)
            continue;

        pos += ret;
        ret = readn(buf + pos, readLen);
        // 再读取一个字节，检查是否为 0xBB，如果不是，也继续下一次循环
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
        switch (buf[pos++]) 
        {
            case TTY_DATA_ID_GPS:  // gps info，获取GPS信息，并解析，用于时间差值判断
                processGpsInfo(buf + pos, readLen - 1);
                gpsPackets++;
                break;

            case TTY_DATA_ID_DEVICE:  // device info，获取STM32设备信息，并解析，传递给网口UDP发送给远端
                processDeviceInfo(buf + pos, readLen - 1);
                devicePackets++;
                break;

            default:
                break;
        }
        //发送心跳信息，发送串口数据返回
        respondHeartbeatMsg();
        //将数据保存到文件
        if (!dataOutputFile.empty()) // 检查是否需要将数据保存到文件
        {
            if (dataOutputCurLines >= dataOutputMaxLines) // 根据当前行数是否超过最大行数判断是否需要创建新文件
            {
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

/// @brief TtyHandler 类的一个成员函数，用于在指定文件的指定行写入或替换字符串。
///        该函数可以将一个新字符串添加到文件的末尾，插入到指定位置，或替换现有内容
///        writeStringToLine 函数用于在文件的指定行写入或替换字符串。它可以添加新行、插入或替换特定行内容
/// @param filename
/// @param lineNumber
/// @param newString
/// @return
int TtyHandler::writeStringToLine(const std::string& filename, int lineNumber, const std::string& newString)
{
    std::vector<std::string> lines; // 定义一个字符串向量 lines，用于存储文件中的每一行数据

    if (lineNumber < 0)
    { // 表示需要在文件的最后添加新字符串 newString。将 newString 添加到 lines 向量中
        lines.push_back(newString);
    }
    else
    {   // 打开指定的文件 filename 进行读操作
        file.open(filename, std::fstream::in);
        if (!file) 
        {
            printf("Error: Unable to open input file: %s\n", filename.c_str());
            return -1;
        }
        // 使用 std::getline 逐行读取文件内容，并将每一行添加到 lines 向量中
        std::string line;
        while (std::getline(file, line)) 
        {
            lines.push_back(line);
        }

        file.close();
        // 如果指定的 lineNumber 大于等于文件中的行数，表示需要在文件末尾添加新内容
        if (lineNumber >= (int)lines.size())
        {   // 使用 insert 方法在 lines 的末尾插入一些空行，以确保行数达到 lineNumber
            lines.insert(lines.end(), lineNumber - lines.size(), "");
            //然后，将 newString 添加到 lines 的末尾
            lines.push_back(newString);
        }
        else 
        {
            if (lines[lineNumber] == newString) // 检查当前行内容是否与 newString 相同
                return 0;
            lines[lineNumber] = newString; // 如果不同，则将指定行的内容替换为 newString
        }
    }

    file.open(filename, std::fstream::out); // 以输出模式重新打开文件
    if (!file) 
    {
        printf("Error: Unable to open output file: %s\n", filename.c_str());
        return -1;
    }
    //使用 for 循环将 lines 中的每一行写回到文件中
    for (const std::string& l : lines)
    {
        file << l << std::endl;
    }

    file.close();
    return 0;
}
