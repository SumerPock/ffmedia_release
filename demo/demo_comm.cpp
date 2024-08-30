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

/// @brief 串口存储路径
/// @param stream 
/// @param conf 
void setTtyDataFile(std::stringstream& stream, TtyConfig& conf)
{
    conf.dataFile = stream.str();
}

/// @brief 单个存储文件最大行数
/// @param stream 
/// @param conf 
void setTtyDataFileMaxLines(std::stringstream& stream, TtyConfig& conf)
{ // 该函数从 stream 中读取参数输入，并将其赋值给 conf.dataFileMaxLines
    stream >> conf.dataFileMaxLines;
}

/// @brief 设置GPS与本机时间差值，若差值大于此值更新本机时间
/// @param stream 
/// @param conf 
void setTtyTimeDiff(std::stringstream& stream, TtyConfig& conf)
{ // 该函数从 stream 中读取参数输入，并将其赋值给 conf.timeDiff
    stream >> conf.timeDiff;
}

/// @brief 这段代码定义了一个名为 ttySetters 的 std::unordered_map，用于将字符串映射到一组函数指针。
///        每个函数指针对应一个函数，这些函数用于设置 TtyConfig 结构体的不同成员变量
std::unordered_map<std::string, void (*)(std::stringstream&, TtyConfig&)> ttySetters = {
    {"enable", setTtyEnable},   //开启tty串口
    {"ttyDev", setTtyDevice},   //tty串口设备节点
    {"ttyPara", setTtyPara},    //串口参数详细配置，波特率，数据位，停止位
    {"netDev", setTtyNetDevice},
    {"gpsFile", setTtyGpsFile}, //GPS文件存储名
    {"gpsFileLineNumber", setTtyGpsFileLineNumber}, //GPS文件存储包行数
    {"dataFile", setTtyDataFile},                   //串口存储文件路径
    {"dataFileMaxLines", setTtyDataFileMaxLines},   //单个文件最大存储包数量
    {"timeDiff", setTtyTimeDiff},   //时间差，查出GPS与本机时间差大于此值时，修正本机时间
};


/// @brief 网络UDP启动
/// @param stream 
/// @param conf 
void setNetEnable(std::stringstream& stream, NetConfig& conf)
{ // 该函数从 stream 中读取参数输入，并将其赋值给 conf.enable
    stream >> conf.enable;
}

/// @brief 网口UDP端口号
/// @param stream 
/// @param conf 
void setNetPort(std::stringstream& stream, NetConfig& conf)
{ // 该函数从 stream 中读取参数输入，并将其赋值给 conf.port
    stream >> conf.port;
}

/// @brief 网络UDP设备节点
/// @param stream 
/// @param conf 
void setNetDevice(std::stringstream& stream, NetConfig& conf)
{ // 该函数从 stream.str() 中读取参数输入，并将其赋值给 conf.netDev
    conf.netDev = stream.str();
}

/// @brief 网络UDP接收数据及存储路径
/// @param stream 
/// @param conf 
void setNetDataFile(std::stringstream& stream, NetConfig& conf)
{ // 该函数从 stream 中读取参数输入，并将其赋值给 conf.dataFile
    conf.dataFile = stream.str();
}

/// @brief 存满x行后开启新的文件
/// @param stream 
/// @param conf 
void setNetDataFileMaxLines(std::stringstream& stream, NetConfig& conf)
{ // 该函数从 stream 中读取参数输入，并将其赋值给 conf.dataFileMaxLines
    stream >> conf.dataFileMaxLines;
}

/// @brief 网络UDP远端地址及端口号
/// @param stream 
/// @param conf 
void setNetRemoteInfo(std::stringstream& stream, NetConfig& conf)
{ // 该函数从 stream 中读取2个参数输入，并将其赋值给 conf.remoteIpAddr 和 conf.remotePort
    stream >> conf.remoteIpAddr >> conf.remotePort;
}

std::unordered_map<std::string, void (*)(std::stringstream&, NetConfig&)> netSetters = {
    {"enable", setNetEnable}, //网络UDP启动
    {"port", setNetPort},     //网络UDP端口号
    {"netDev", setNetDevice}, //网络UDP设备节点
    {"dataFile", setNetDataFile},                   //网络UDP接收数据存储文件及路径
    {"dataFileMaxLines", setNetDataFileMaxLines},   //存满500行后，建立新的文件存储
    {"remoteInfo", setNetRemoteInfo},               //网络UDP远端地址及端口号
};

// @brief 配置文件解析
/// @param user    这是一个通用指针，通常用于传递用户自定义的数据。在这个例子中，
///                它被转换为 DemoConfig* 类型，指向存储配置的结构体
/// @param section 这是配置文件中的节名称。INI文件通常以 [section] 的形式定义节
/// @param name    这是配置项的名称，即特定配置项的名字
/// @param value   这是配置项的值，即特定配置项的值
/// @return
static int handler(void* user, const char* section, const char* name,
                   const char* value)
{
    // 将通用指针 user 转换为指向 DemoConfig 结构体的指针，以便在后续代码中使用
    DemoConfig* config = static_cast<DemoConfig*>(user);
    if (MATCH("tty", section))
    { // MATCH是一个宏或函数，用于比较section字符串与"tty"是否相同。如果相同，进入该分支处理，检查当前处理的配置节是否为"tty"
        if (name && value) 
        {
            // ttySetters.find(name)：查找name在ttySetters中的位置
            auto it = ttySetters.find(name);
            if (it != ttySetters.end()) 
            {
                std::stringstream stream(value);
                it->second(stream, config->tty);
            }
        }
    }
    else if (MATCH("net", section))
    { // 检查当前处理的配置节是否为"net"
        if (name && value) 
        {
            std::stringstream stream(value);
            auto it = netSetters.find(name); // 查找并使用netSetters映射中的函数对象来处理name对应的配置项
            if (it != netSetters.end()) 
            {
                std::stringstream stream(value);
                it->second(stream, config->net); // 使用value作为输入，将其转换并设置到config->net中
            }
        }
    }
    return 1;
}

int main(int argc, char** argv)
{
    DemoConfig config;         // 用于存储从配置文件中读取的配置
    NetHandler *net = nullptr; // 网络处理器的实例化，初始化为空指针（nullptr）
    TtyHandler *tty = nullptr; // 串口处理器的实例化，初始化为空指针（nullptr）
    int ret;

    if (argc < 2) 
    {//表示用户没有提供配置文件的路径
        printf("Usage: %s ConfFile.ini\n", argv[0]);
        return -1;
    }

    if (ini_parse(argv[1], handler, &config) < 0)
    { // 调用ini_parse函数解析配置文件，文件路径由argv[1]提供 ，
      // handler是解析过程中使用的回调函数，
      // &config是传递给回调函数的用户数据（即DemoConfig实例)
        printf("Can't load 'test.ini'\n");
        return -1;
    }
    // 初始化和启动 TtyHandler 串口处理器,ttyDev不为空字符串
    if (config.tty.enable && !config.tty.ttyDev.empty()) 
    {
        printf("Tty handler device %s, speed %d, data bits %d, stop bits %d, parity %d\n",
               config.tty.ttyDev.c_str(), config.tty.ttyPara.speed,
               config.tty.ttyPara.databits, config.tty.ttyPara.stopbits,
               config.tty.ttyPara.parity);

        // 使用new运算符动态分配TtyHandler对象，并使用从配置中读取的设备名和参数初始化
        tty = new TtyHandler(config.tty.ttyDev, config.tty.ttyPara);

        //设置串口数据输出文件的最大行数
        tty->setDataOutputFile(config.tty.dataFile, config.tty.dataFileMaxLines);
        //设置GPS数据输出文件的最大行数
        tty->setGpsDataOutputFile(config.tty.gpsFile, config.tty.gpsFileLineNumber);
        //设置网络设备
        tty->setNetworkDevice(config.tty.netDev);
        //设置时间差
        tty->setTimeDiff(config.tty.timeDiff);
        ret = tty->init(); // 调用tty对象的init方法进行初始化，返回值存储在ret中
        if (ret < 0) 
        {
            printf("Failed to init tty handler\n");
            delete tty; // 释放TTY处理器对象，并将指针置为nullptr。
            tty = nullptr;
        } else
            tty->start(); // 调用start方法启动TTY处理器
    }

    // 初始化和启动 NetHandler 网络处理器
    if (config.net.enable && config.net.port > 0) 
    {
        printf("Net handler port %d\n", config.net.port);
        // 使用new运算符动态分配NetHandler对象，并使用从配置中读取的端口号初始化
        net = new NetHandler(config.net.port);
        //设置数据输出文件和最大行数
        net->setDataOutputFile(config.net.dataFile, config.net.dataFileMaxLines);
        //设置网络设备节点
        net->setNetworkDevice(config.net.netDev);
        // 设置远程设备的IP地址和端口号
        net->setRemoteDeviceInfo(config.net.remoteIpAddr, config.net.remotePort);
        // 调用net对象的init方法进行初始化，返回值存储在ret中
        ret = net->init();
        if (ret < 0) 
        {
            printf("Failed to init net handler\n");
            delete net; // 释放网络处理器对象，并将指针置为nullptr
            net = nullptr;
        } else
            net->start(); // 调用start方法启动网络处理器
    }
    //检查是否初始化了处理器
    if (net == nullptr && tty == nullptr)
        return -1;
    
    //信号处理与程序主循环
    int err, sig;
    sigset_t st;             // 信号集。用于存储要处理的信号
    sigemptyset(&st);        // 初始化信号集为空
    sigaddset(&st, SIGINT);  // 添加SIGINT（Ctrl+C）信号到信号集
    sigaddset(&st, SIGTERM); // 添加SIGTERM（终止信号）到信号集
    sigprocmask(SIG_SETMASK, &st, NULL); // 将当前进程的信号屏蔽字设置为st，屏蔽SIGINT和SIGTERM信号
    while (true) 
    {
        err = sigwait(&st, &sig); // 使用sigwait等待信号集中的信号
        if (err == 0) 
        {
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
