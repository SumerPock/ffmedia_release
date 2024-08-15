#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <unordered_map>

#include "ini.h"
#include "ttyHandler.hpp"


#define MATCH(s, n) strcmp(s, n) == 0

/// @brief 网络配置参数
struct NetConfig {
    bool enable = false;
    uint16_t port = 0;
    std::string netDev;
    std::string remoteIpAddr;
    uint16_t remotePort = 0;
    std::string dataFile;
    int dataFileMaxLines = 0;
};

/// @brief 串口配置参数
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

/// @brief 将结构体NetConfig 和 TtConfig组合在一起，但我不清楚为什么需要组合这个
struct DemoConfig {
    TtyConfig tty;
    NetConfig net;
};

/// @brief 该函数的目标是将从 stream 中读取的布尔值赋给 conf 的 enable 字段，这个字段决定了TTY功能是否启用
///        该函数被设计为一个通用的配置设置函数，可以处理来自不同输入源的布尔值（通常是字符串形式的 true 或 false）
/// @param stream 是一个字符串流对象的引用。它通常包含了从配置文件或其他输入中读取的字符串数据。
///               在调用这个函数之前，这个流已经被初始化为包含某个配置项的值,
///               在这种情况下，stream 中包含的是一个表示是否启用 TTY 功能的字符串，
///               可能是 "true"、"false"、"1"、"0" 等可以转换为布尔值的字符串
/// @param conf   conf 是一个 TtyConfig 结构体的引用，这个结构体存储了所有与TTY相关的配置信息。
void setTtyEnable(std::stringstream& stream, TtyConfig& conf)
{
    // 使用了c++中的流运算符 >> 从stream中提取数据并将之转换为布尔值，然后赋值给conf.enable
    stream >> conf.enable;
}

/// @brief setTtyDevice 函数的核心功能是从字符串流中获取一个字符串值，并将其存储到 TtyConfig 结构体的 ttyDev 字段中
///        这个字段通常用来存储TTY设备的路径或名称，这对于后续操作（如初始化TTY设备）非常重要
///        该函数的目标是将 stream 中的字符串赋给 conf 的 ttyDev 字段，这个字段存储TTY设备的路径或名称
/// @param stream stream 是一个字符串流对象的引用，通常包含从配置文件或其他输入中读取的字符串数据。
///               在调用这个函数之前，这个流已经被初始化为包含某个配置项的值。
///               在这种情况下，stream 中包含的是一个表示TTY设备路径或名称的字符串，如 "/dev/ttyS0"
/// @param conf   conf 是一个 TtyConfig 结构体的引用，这个结构体存储了所有与TTY相关的配置信息。
///
void setTtyDevice(std::stringstream& stream, TtyConfig& conf)
{
    // 这行代码使用 stream.str() 从字符串流中获取完整的字符串，并将其赋值给 conf.ttyDev。
    conf.ttyDev = stream.str();
}

/// @brief
/// @param stream  stream 是一个字符串流对象的引用，通常包含从配置文件或其他输入中读取的字符串数据。在调用这个函数之前，这个流已经被初始化为包含某个配置项的值。
///                在这种情况下，stream 中包含的是一串用于设置串口参数的值，可能是类似于 "9600 8 1 N" 这样的字符串
/// @param conf    conf 是一个 TtyConfig 结构体的引用，这个结构体存储了所有与TTY相关的配置信息。
///                ttyPara 是 TtyConfig 结构体中的一个子结构体（TtyHandler::ttyPara），
///                它包含了与串口通信相关的参数，如波特率（speed）、数据位（databits）、停止位（stopbits）、校验位（parity）等。
void setTtyPara(std::stringstream& stream, TtyConfig& conf)
{
    // 这行代码使用流运算符（>>）从 stream 中依次提取值，并将这些值赋给 conf.ttyPara 的各个成员
    // 具体来说，stream 中的第一个值被提取并赋给 speed（波特率），第二个值赋给 databits（数据位），第三个值赋给 stopbits（停止位），第四个值赋给 parity（校验位）。
    stream >> conf.ttyPara.speed >> conf.ttyPara.databits >> conf.ttyPara.stopbits >> conf.ttyPara.parity;
}

/// @brief
/// @param stream stream 是一个字符串流对象的引用，通常包含从配置文件或其他输入中读取的字符串数据。在调用这个函数之前，这个流已经被初始化为包含某个配置项的值。
///               在这种情况下，stream 中包含的是一个表示网络设备名称或路径的字符串，比如 "eth0" 或 "wlan0"
/// @param conf   conf 是一个 TtyConfig 结构体的引用，这个结构体存储了所有与TTY相关的配置信息
///               netDev 是 TtyConfig 结构体中的一个字段，用于存储网络设备的名称或路径。
void setTtyNetDevice(std::stringstream& stream, TtyConfig& conf)
{
    // 这行代码使用 stream.str() 从字符串流中获取完整的字符串，并将其赋值给 conf.netDev。
    // stream.str() 返回流中存储的整个字符串内容。例如，如果流包含 "eth0"，那么这行代码会将 netDev 设置为 "eth0"。
    conf.netDev = stream.str();
}

/// @brief setTtyGpsFile 函数的核心功能是从字符串流中获取一个表示文件路径或名称的字符串值，并将其存储到 TtyConfig 结构体的 gpsFile 字段中。
///        这个字段通常用于指定存储GPS数据的文件路径或名称。在实际应用中，GPS数据可能会被记录到这个指定的文件中以供后续分析或调试。
/// @param stream stream 是一个字符串流对象的引用，通常包含从配置文件或其他输入中读取的字符串数据。
///               在调用这个函数之前，这个流已经被初始化为包含某个配置项的值。
///               在这种情况下，stream 中包含的是一个表示GPS数据文件路径或文件名的字符串，
///               比如 "gps_data.log" 或 "/var/log/gps_data.txt"。
/// @param conf   conf 是一个 TtyConfig 结构体的引用，这个结构体存储了所有与TTY相关的配置信息。
///               gpsFile 是 TtyConfig 结构体中的一个字段，用于存储与GPS数据相关的文件路径或文件名。
void setTtyGpsFile(std::stringstream& stream, TtyConfig& conf)
{
    // 这行代码使用 stream.str() 从字符串流中获取完整的字符串，并将其赋值给 conf.gpsFile。
    // stream.str() 返回流中存储的整个字符串内容。
    //例如，如果流包含 "gps_data.log"，那么这行代码会将 gpsFile 设置为 "gps_data.log"。
    conf.gpsFile = stream.str();
}

/// @brief setTtyGpsFileLineNumber 函数的核心功能是从字符串流中读取一个整数值，并将其存储到 TtyConfig 结构体的 gpsFileLineNumber 字段中。
///        这个字段可能用于指定与GPS数据文件相关的某种行数限制或配置，例如指定读取或写入的行数。在实际应用中，这可能用于控制日志文件的大小、分段处理数据等。
/// @param stream  conf 是一个 TtyConfig 结构体的引用，这个结构体存储了所有与TTY相关的配置信息。
/// @param conf    gpsFileLineNumber 是 TtyConfig 结构体中的一个字段，用于存储与GPS数据文件相关的行数信息。
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

/// @brief 这段代码定义了一个名为 ttySetters 的 std::unordered_map，用于将字符串映射到一组函数指针。
///        每个函数指针对应一个函数，这些函数用于设置 TtyConfig 结构体的不同成员变量
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

// @brief 配置文件解析
/// @param user    这是一个通用指针，通常用于传递用户自定义的数据。在这个例子中，
///                它被转换为 DemoConfig* 类型，指向存储配置的结构体
/// @param section 这是配置文件中的节名称。INI文件通常以 [section] 的形式定义节
/// @param name    这是配置项的名称
/// @param value   这是配置项的值
/// @return
static int handler(void* user, const char* section, const char* name,
                   const char* value)
{
    // 将通用指针 user 转换为指向 DemoConfig 结构体的指针，以便在后续代码中使用
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
    //解析配置文件
    if (ini_parse(argv[1], handler, &config) < 0) {
        printf("Can't load 'test.ini'\n");
        return -1;
    }
    // 初始化和启动 TtyHandler 串口处理器
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

    // 初始化和启动 NetHandler 网络处理器
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
    //检查是否初始化了处理器
    if (net == nullptr && tty == nullptr)
        return -1;
    //信号处理与程序主循环
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
    //停止并清理处理器
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
