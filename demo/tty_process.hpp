
#pragma once
#include <stdint.h>
#include <time.h>
#include <string>
#include <thread>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "system_common.hpp"

class ModuleTtyProcess
{
public:
    struct ttyPara {
        int speed = 115200;
        int databits = 8;
        int stopbits = 1;
        int parity = 0;
    };

public:
    ModuleTtyProcess(const std::string& dev, ttyPara para);
    ~ModuleTtyProcess();
    void setDataOutputFile(const std::string& filename, int max_lines = 0);
    void setGpsDataOutputFile(const std::string& filename, int lineNumber = 0);
    void setInterface(const std::string& dev);
    void setTimeDiff(int diff);
    int init();
    void start();
    void stop();

protected:
    ssize_t readn(uint8_t* buffer, size_t n);
    ssize_t writen(const uint8_t* buffer, size_t n);

    void ttyProcess();
    int processGpsInfo(uint8_t* buf, size_t bytes);
    int processDeviceInfo(uint8_t* buf, size_t bytes);
    int respondHeartbeatMsg();

    int writeStringToFile(const std::string& filename, const std::string& line);
    int writeStringToLine(const std::string& filename, int lineNumber, const std::string& newString);

private:
    int open();
    void close();

    int setParity();
    int setSpeed();

private:
    std::string device;
    ttyPara para;
    int fd;
    int time_out;  // ms

    bool work_flag;
    std::thread* work_thread;

    uint8_t* packet;
    size_t packet_size;

    std::fstream file;
    std::string net_dev;
    std::string data_output_file;
    std::string data_output_tmp_file;
    int data_output_max_lines;
    int data_output_cur_lines;
    std::string gpsinfo_output_file;
    int gpsinfo_output_file_lines;

    int time_diff;

    uint32_t gps_packets;
    uint32_t device_packets;

    struct timeval start_tv;
    struct timeval cur_tv;
};
