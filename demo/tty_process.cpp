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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
// #include <asm/byteorder.h>


#include "tty_process.hpp"

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


struct Ipv4Info {
    uint8_t ip0;
    uint8_t ip1;
    uint8_t ip2;
    uint8_t ip3;
};


struct RespondMsg {
    uint32_t second;
    uint32_t gps_packets;
    uint32_t device_packets;
    uint32_t ipv4_addr;
};

#pragma pack()


uint16_t calculateCRC16_X25(const uint8_t* data, size_t length);
uint32_t getLocalIPAddressUint(const char* interface);
std::string bytesToHexString(const uint8_t* data, size_t size);
int writeStringToLine(const std::string& filename, int lineNumber, const std::string& newString);
std::string timeToString(struct timeval time);


ModuleTtyProcess::ModuleTtyProcess(const std::string& dev, ttyPara para)
    : device(dev), para(para), fd(-1), time_out(1000), packet(nullptr), packet_size(256),
      gps_packets(0), device_packets(0)
{
    gettimeofday(&start_tv, NULL);
    cur_tv = start_tv;
    time_diff = -1;
    work_flag = false;
    work_thread = nullptr;
    data_output_cur_lines = 0;
    data_output_max_lines = 0;
}

ModuleTtyProcess::~ModuleTtyProcess()
{
    stop();
    close();

    if (packet)
        delete[] packet;
}

int ModuleTtyProcess::setSpeed()
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

int ModuleTtyProcess::setParity()
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

int ModuleTtyProcess::open()
{
    fd = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        return -1;
    }

    if (fcntl(fd, F_SETFL, 0) < 0)
        printf("fcntl fail\n");

    return fd;
}

void ModuleTtyProcess::close()
{
    if (fd < 0)
        return;

    ::close(fd);
    fd = -1;
}

ssize_t ModuleTtyProcess::readn(uint8_t* buffer, size_t n)
{
    ssize_t num_read;
    size_t to_read;
    uint8_t* buf;
    struct pollfd p_fd = {.fd = fd, .events = POLLIN};

    buf = buffer;
    for (to_read = 0; to_read < n;) {
        if (poll(&p_fd, 1, time_out) <= 0)
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

ssize_t ModuleTtyProcess::writen(const uint8_t* buffer, size_t n)
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


int ModuleTtyProcess::init()
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
        close();
        return -1;
    }

    if (setParity()) {
        printf("Failed to set parity\n");
        close();
        return -1;
    }

    if (packet)
        delete[] packet;

    packet = new uint8_t[packet_size];
    return 0;
}

void ModuleTtyProcess::start()
{
    work_flag = true;
    work_thread = work_thread ? work_thread : new std::thread(&ModuleTtyProcess::ttyProcess, this);
}

void ModuleTtyProcess::stop()
{
    work_flag = false;
    if (work_thread) {
        work_thread->join();
        delete work_thread;
        work_thread = nullptr;
    }
}

int ModuleTtyProcess::processGpsInfo(uint8_t* buf, size_t bytes)
{
    GpsInfo info;
    char info_str[256];
    if (bytes < sizeof info)
        return -1;

    memcpy((uint8_t*)&info, buf, sizeof info);
    sprintf(info_str, "%c %d %0.4f : %c %d %0.4f", info.latitude_h, info.latitude,
            info.latitude_d / 10000.0, info.longitude_h, info.longitude, info.longitude_d / 10000.0);

    if (!gpsinfo_output_file.empty()) {
        writeStringToLine(gpsinfo_output_file, gpsinfo_output_file_lines, info_str);
    }

    if (time_diff >= 0) {
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

        if (diff >= time_diff >> 1) {
            int ret;
            tv.tv_sec = time;
            if (diff > time_diff)
                ret = settimeofday(&tv, NULL);
            else
                ret = adjtime(&tv, NULL);

            if (ret != 0)
                printf("Fail to adjust time, time diff: %ld, errno: %s\n", diff, strerror(errno));
        }
    }

    return 0;
}

int ModuleTtyProcess::processDeviceInfo(uint8_t* buf, size_t bytes)
{
    return 0;
}

int ModuleTtyProcess::respondHeartbeatMsg()
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
    gettimeofday(&cur_tv, NULL);
    msg.second = cur_tv.tv_sec - start_tv.tv_sec;
    msg.gps_packets = gps_packets;
    msg.device_packets = device_packets;
    msg.ipv4_addr = net_dev.empty() ? 0xFFFFFFFF : getLocalIPAddressUint(net_dev.c_str());
    memcpy(buf + pos, (void*)&msg, sizeof msg);
    pos += sizeof msg;
    uint16_t crc = calculateCRC16_X25(buf, frame_size - 2);
    *(uint16_t*)(buf + pos) = crc;

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

void ModuleTtyProcess::ttyProcess()
{
    uint8_t* buf = packet;
    size_t buf_size = packet_size;
    int pos, read_len, frame_size;
    int ret;

    if (fd < 0 || buf == nullptr) {
        work_flag = false;
        return;
    }

    while (work_flag) {
        pos = 0;
        read_len = 1;
        frame_size = 0;
        ret = readn(buf + pos, read_len);
        if (ret != read_len || buf[pos] != 0xFF)
            continue;

        pos += ret;
        ret = readn(buf + pos, read_len);
        if (ret != read_len || buf[pos] != 0xBB)
            continue;

        pos += ret;
        ret = readn(buf + pos, read_len);
        if (ret != read_len)
            continue;

        frame_size = buf[pos];
        pos += ret;
        read_len = frame_size - pos;
        if ((size_t)frame_size > buf_size || read_len <= 0) {
            printf("Erron: frame_size: %d, pos: %d\n", frame_size, pos);
            continue;
        }

        ret = readn(buf + pos, read_len);
        if (ret != read_len) {
            printf("Failed to read %d buf : %d\n", read_len, ret);
            continue;
        }

        int crc_pos = pos + read_len - 2;
        uint16_t src_crc = *(uint16_t*)(buf + crc_pos);
        if (src_crc != calculateCRC16_X25(buf, frame_size - 2)) {
            printf("crc check err src crc:%x, cheack crc:%x\n", src_crc, calculateCRC16_X25(buf, frame_size - 2));
            continue;
        }

        // data ID
        switch (buf[pos++]) {
            case TTY_DATA_ID_GPS:  // gps info
                processGpsInfo(buf + pos, read_len - 1);
                gps_packets++;
                break;
            case TTY_DATA_ID_DEVICE:  // device info
                processDeviceInfo(buf + pos, read_len - 1);
                device_packets++;
                break;
            default:
                break;
        }

        respondHeartbeatMsg();

        if (!data_output_file.empty()) {
            if (data_output_cur_lines >= data_output_max_lines) {
                data_output_tmp_file.assign(ptsToTimeStr(0, data_output_file.c_str()) + ".txt");
                data_output_cur_lines = 0;
                createDirectory(data_output_tmp_file.c_str());
            }
            auto str = bytesToHexString(buf, frame_size);
            if (writeStringToFile(data_output_tmp_file, str) == 0)
                data_output_cur_lines++;
        }
    }

    return;
}

void ModuleTtyProcess::setDataOutputFile(const std::string& filename, int max_lines)
{
    data_output_file = filename;
    data_output_max_lines = max_lines;
    data_output_cur_lines = max_lines;
}

void ModuleTtyProcess::setGpsDataOutputFile(const std::string& filename, int lineNumber)
{
    gpsinfo_output_file = filename;
    gpsinfo_output_file_lines = lineNumber;
}

void ModuleTtyProcess::setInterface(const std::string& dev)
{
    net_dev = dev;
}

void ModuleTtyProcess::setTimeDiff(int diff)
{
    time_diff = diff;
}


int ModuleTtyProcess::writeStringToFile(const std::string& filename, const std::string& line)
{
    std::fstream outFile(filename, std::ofstream::app);
    file.open(filename, std::fstream::app);
    if (!file)
        return -1;

    outFile << line << std::endl;
    file.close();
    return 0;
}

int ModuleTtyProcess::writeStringToLine(const std::string& filename, int lineNumber, const std::string& newString)
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

std::string bytesToHexString(const uint8_t* data, size_t size)
{
    std::stringstream ss;
    ss << std::hex << std::uppercase;
    for (size_t i = 0; i < size; ++i) {
        ss << "0x" << static_cast<int>(data[i]) << " ";
    }
    return ss.str();
}

uint32_t crc16_x25_table1[256] = {0x0, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf, 0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7, 0x1081, 0x108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e, 0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876, 0x2102, 0x308b, 0x210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd, 0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5, 0x3183, 0x200a, 0x1291, 0x318, 0x77a7, 0x662e, 0x54b5, 0x453c, 0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974, 0x4204, 0x538d, 0x6116, 0x709f, 0x420, 0x15a9, 0x2732, 0x36bb, 0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3, 0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x528, 0x37b3, 0x263a, 0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72, 0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x630, 0x17b9, 0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1, 0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x738, 0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70, 0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 0x840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff, 0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036, 0x18c1, 0x948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e, 0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5, 0x2942, 0x38cb, 0xa50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd, 0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134, 0x39c3, 0x284a, 0x1ad1, 0xb58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c, 0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3, 0x4a44, 0x5bcd, 0x6956, 0x78df, 0xc60, 0x1de9, 0x2f72, 0x3efb, 0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232, 0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0xd68, 0x3ff3, 0x2e7a, 0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1, 0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0xe70, 0x1ff9, 0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330, 0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0xf78};

uint16_t calculateCRC16_X25(const uint8_t* data, size_t length)
{
    uint16_t crc = 0xFFFF;  // init
    for (size_t i = 0; i < length; i++)
        crc = (crc >> 8) ^ crc16_x25_table1[(crc ^ data[i]) & 0x0FF];

    // crc = __be16_to_cpu(crc);
    return crc ^ 0xFFFF;  // xor
}

uint32_t getLocalIPAddressUint(const char* interface)
{
    int fd = -1;
    uint32_t ip_addr = 0;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        printf("unable to create datagram socket: %d\n", errno);
        return ip_addr;
    }

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

    if (ioctl(fd, SIOCGIFADDR, &ifr) != -1)
        ip_addr = ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr;

    close(fd);
    return ip_addr;
}
