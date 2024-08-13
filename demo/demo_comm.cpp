#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <unordered_map>

#include "ini.h"
#include "ttyHandler.hpp"


#define MATCH(s, n) strcmp(s, n) == 0


struct NetConfig {
    bool enable = false;
    uint16_t port = 0;
    std::string netDev;
    std::string remoteIpAddr;
    uint16_t remotePort = 0;
    std::string dataFile;
    int dataFileMaxLines = 0;
};

struct TtyConfig {
    bool enable = false;
    std::string ttyDev;
    TtyHandler::ttyPara ttyPara;
    std::string netDev;
    std::string gpsFile;
    int gpsFileLineNumber = 0;
    std::string dataFile;
    int dataFileMaxLines = 0;
    int timeDiff = -1;
};

struct DemoConfig {
    TtyConfig tty;
    NetConfig net;
};

void setTtyEnable(std::stringstream& stream, TtyConfig& conf)
{
    stream >> conf.enable;
}

void setTtyDevice(std::stringstream& stream, TtyConfig& conf)
{
    conf.ttyDev = stream.str();
}

void setTtyPara(std::stringstream& stream, TtyConfig& conf)
{
    stream >> conf.ttyPara.speed >> conf.ttyPara.databits >> conf.ttyPara.stopbits >> conf.ttyPara.parity;
}

void setTtyNetDevice(std::stringstream& stream, TtyConfig& conf)
{
    conf.netDev = stream.str();
}

void setTtyGpsFile(std::stringstream& stream, TtyConfig& conf)
{
    conf.gpsFile = stream.str();
}

void setTtyGpsFileLineNumber(std::stringstream& stream, TtyConfig& conf)
{
    stream >> conf.gpsFileLineNumber;
}

void setTtyDataFile(std::stringstream& stream, TtyConfig& conf)
{
    conf.dataFile = stream.str();
}

void setTtyDataFileMaxLines(std::stringstream& stream, TtyConfig& conf)
{
    stream >> conf.dataFileMaxLines;
}

void setTtyTimeDiff(std::stringstream& stream, TtyConfig& conf)
{
    stream >> conf.timeDiff;
}

std::unordered_map<std::string, void (*)(std::stringstream&, TtyConfig&)> ttySetters = {
    {"enable", setTtyEnable},
    {"ttyDev", setTtyDevice},
    {"ttyPara", setTtyPara},
    {"netDev", setTtyNetDevice},
    {"gpsFile", setTtyGpsFile},
    {"gpsFileLineNumber", setTtyGpsFileLineNumber},
    {"dataFile", setTtyDataFile},
    {"dataFileMaxLines", setTtyDataFileMaxLines},
    {"timeDiff", setTtyTimeDiff},
};

void setNetEnable(std::stringstream& stream, NetConfig& conf)
{
    stream >> conf.enable;
}

void setNetPort(std::stringstream& stream, NetConfig& conf)
{
    stream >> conf.port;
}

void setNetDevice(std::stringstream& stream, NetConfig& conf)
{
    conf.netDev = stream.str();
}

void setNetDataFile(std::stringstream& stream, NetConfig& conf)
{
    conf.dataFile = stream.str();
}

void setNetDataFileMaxLines(std::stringstream& stream, NetConfig& conf)
{
    stream >> conf.dataFileMaxLines;
}

void setNetRemoteInfo(std::stringstream& stream, NetConfig& conf)
{
    stream >> conf.remoteIpAddr >> conf.remotePort;
}

std::unordered_map<std::string, void (*)(std::stringstream&, NetConfig&)> netSetters = {
    {"enable", setNetEnable},
    {"port", setNetPort},
    {"netDev", setNetDevice},
    {"dataFile", setNetDataFile},
    {"dataFileMaxLines", setNetDataFileMaxLines},
    {"remoteInfo", setNetRemoteInfo},
};

static int handler(void* user, const char* section, const char* name,
                   const char* value)
{
    DemoConfig* config = static_cast<DemoConfig*>(user);
    if (MATCH("tty", section)) {
        if (name && value) {
            auto it = ttySetters.find(name);
            if (it != ttySetters.end()) {
                std::stringstream stream(value);
                it->second(stream, config->tty);
            }
        }
    } else if (MATCH("net", section)) {
        if (name && value) {
            std::stringstream stream(value);
            auto it = netSetters.find(name);
            if (it != netSetters.end()) {
                std::stringstream stream(value);
                it->second(stream, config->net);
            }
        }
    }
    return 1;
}

int main(int argc, char** argv)
{
    DemoConfig config;
    NetHandler* net = nullptr;
    TtyHandler* tty = nullptr;
    int ret;

    if (argc < 2) {
        printf("Usage: %s ConfFile.ini\n", argv[0]);
        return -1;
    }

    if (ini_parse(argv[1], handler, &config) < 0) {
        printf("Can't load 'test.ini'\n");
        return -1;
    }

    if (config.tty.enable && !config.tty.ttyDev.empty()) {
        printf("Tty handler device %s, speed %d, data bits %d, stop bits %d, parity %d\n",
               config.tty.ttyDev.c_str(), config.tty.ttyPara.speed,
               config.tty.ttyPara.databits, config.tty.ttyPara.stopbits,
               config.tty.ttyPara.parity);

        tty = new TtyHandler(config.tty.ttyDev, config.tty.ttyPara);
        tty->setDataOutputFile(config.tty.dataFile, config.tty.dataFileMaxLines);
        tty->setGpsDataOutputFile(config.tty.gpsFile, config.tty.gpsFileLineNumber);
        tty->setNetworkDevice(config.tty.netDev);
        tty->setTimeDiff(config.tty.timeDiff);
        ret = tty->init();
        if (ret < 0) {
            printf("Failed to init tty handler\n");
            delete tty;
            tty = nullptr;
        } else
            tty->start();
    }

    if (config.net.enable && config.net.port > 0) {
        printf("Net handler port %d\n", config.net.port);

        net = new NetHandler(config.net.port);
        net->setDataOutputFile(config.net.dataFile, config.net.dataFileMaxLines);
        net->setNetworkDevice(config.net.netDev);
        net->setRemoteDeviceInfo(config.net.remoteIpAddr, config.net.remotePort);
        ret = net->init();
        if (ret < 0) {
            printf("Failed to init net handler\n");
            delete net;
            net = nullptr;
        } else
            net->start();
    }

    if (net == nullptr && tty == nullptr)
        return -1;

    int err, sig;
    sigset_t st;
    sigemptyset(&st);
    sigaddset(&st, SIGINT);
    sigaddset(&st, SIGTERM);
    sigprocmask(SIG_SETMASK, &st, NULL);
    while (true) {
        err = sigwait(&st, &sig);
        if (err == 0) {
            printf("receive sig: %d\n", sig);
            break;
        }
    }

    if (net) {
        net->stop();
        delete net;
    }

    if (tty) {
        tty->stop();
        delete tty;
    }

    return 0;
}
