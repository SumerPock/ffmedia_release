
#pragma once
#include <stdint.h>
#include <time.h>
#include <string>
#include <thread>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "netHandler.hpp"
#include "system_common.hpp"


class TtyHandler : public NetHandler
{
public:
    struct ttyPara {
        int speed = 115200;
        int databits = 8;
        int stopbits = 1;
        int parity = 0;
    };

public:
    TtyHandler(const std::string& dev, ttyPara para);
    ~TtyHandler();
    void setDataOutputFile(const std::string& filename, int maxLines = 0);
    void setGpsDataOutputFile(const std::string& filename, int lineNumber = 0);
    void setTimeDiff(int diff);

    virtual int init() override;

protected:
    virtual void process() override;

    int writeStringToLine(const std::string& filename, int lineNumber, const std::string& newString);

private:
    ssize_t readn(uint8_t* buffer, size_t n);
    ssize_t writen(const uint8_t* buffer, size_t n);
    int processGpsInfo(uint8_t* buf, size_t bytes);
    int processDeviceInfo(uint8_t* buf, size_t bytes);
    int respondHeartbeatMsg();

    int open();
    int setParity();
    int setSpeed();

private:
    std::string device;
    ttyPara para;

    std::string gpsinfoOutputFile;
    int gpsinfoOutputFileLines;

    int timeDiff;
};