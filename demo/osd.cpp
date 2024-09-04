#include "osd.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <string.h>

#include <mutex>    // 使用std::mutex进行线程同步
#include <chrono>   // 用于时间计算
#include <memory>   // 使用std::shared_ptr
#include <cstdio>   // 使用snprintf
#include <fstream>  // 用于文件读取
#include <vector>   // 用于存储多个温度值
#include <string>   // 用于字符串操作
#include <dirent.h> // 使用POSIX标准库中的dirent.h进行目录遍历
#include <cstring>  // 使用strstr函数
#include <sys/statvfs.h> // 用于获取文件系统信息
#include <cstdlib>       // 使用system函数调用shell命令

std::chrono::steady_clock::time_point last_update_time = std::chrono::steady_clock::now();
std::string cached_ssd_info = "";
double cached_cpu_temp = -1;
std::pair<double, double> cached_filesystem_usage = {-1, -1};

double getAverageCpuTemperature();

/// @brief 检查系统中是否存在挂载的SSD
/// @param mount_path
/// @return
std::string checkSsdMounted(std::string &mount_path)
{
    FILE *pipe = popen("lsblk -d -o NAME,ROTA", "r"); // 执行命令获取设备信息
    if (!pipe)
        return ""; // 如果命令执行失败，返回空字符串

    char buffer[256];
    std::string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        std::string line(buffer);
        // 清除行末尾的换行符
        line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
        std::istringstream iss(line);
        std::string name, rota;

        if (iss >> name >> rota && rota == "0") // 只提取设备名和ROTA为0的设备
        {
            if (name.find("nvme") != std::string::npos || name.find("sd") != std::string::npos)
            {
                result = name;                        // 只保留设备名
                mount_path = "/home/firefly/ssdvideo"; // 使用指定的挂载路径
                break;                                // 找到SSD后跳出循环
            }
        }
    }
    pclose(pipe);
    return result; // 返回找到的设备信息，或者空字符串表示未找到
}

// 获取文件系统的容量和未使用容量
std::pair<double, double> getFilesystemUsage(const std::string &mount_path)
{
    struct statvfs stat;
    if (statvfs(mount_path.c_str(), &stat) == 0)
    {
        double total = (double)stat.f_blocks * stat.f_frsize / (1024 * 1024 * 1024); // 总容量 (GB)
        double free = (double)stat.f_bfree * stat.f_frsize / (1024 * 1024 * 1024);   // 可用容量 (GB)
        return {total, free};
    }
    return {-1, -1}; // 返回错误
}

// 使用 smartctl 工具获取 NVMe SSD 温度
int getSsdTemperature(const std::string &device_name)
{
    char buffer[128];
    /// dev/nvme0n1
    //std::string command = "sudo smartctl -a " + device_name + " | grep -i 'Temperature Sensor 1' | awk '{print $4}'";
    std::string command = "sudo smartctl -a /dev/nvme0n1 | grep -i 'Temperature Sensor 1' | awk '{print $4}'";
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe)
        return -1;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        pclose(pipe);
        return atoi(buffer); // 将读取的温度值转换为整数
    }
    else
    {
        pclose(pipe);
        return -1; // 如果未能读取到温度值，返回 -1
    }
}

// 更新缓存的OSD数据
void updateOSDData()
{
    std::string mount_path = "/home/firefly/ssdvideo"; // 使用指定的挂载路径
    cached_ssd_info = checkSsdMounted(mount_path);     // 更新SSD信息
    cached_cpu_temp = getAverageCpuTemperature();      // 更新CPU温度
    if (!cached_ssd_info.empty())
    { // 检查cached_ssd_info 是否不为空（即存在已挂载的SSD）
        
        //std::string device_name = cached_ssd_info.substr(0, cached_ssd_info.find(' '));

        // 提取设备名称，只取第一部分，不含空格和ROTA标志
        std::istringstream iss(cached_ssd_info);
        std::string device_name;
        iss >> device_name; // 仅提取设备名称部分

        cached_filesystem_usage = getFilesystemUsage(mount_path); // 使用指定的挂载路径
        int ssdTemp = getSsdTemperature(device_name);
        printf("ssdTemp = %d", ssdTemp);

        if (ssdTemp > 0)
        {
            cached_ssd_info += " Temp: " + std::to_string(ssdTemp) + "C";
        }
        else
        {
            cached_ssd_info += " Tc: N/A";
        }
    }
}

/**以下是读取osd.conf配置 */

/// @brief 读取配置文件中的视频节点
/// @param lineStream 
/// @param conf 
static void parseVDev(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取输入，并将其赋值给 conf.vDev，即使用流提取操作符（>>）从 lineStream 中读取数据，并将其存储到 conf.vDev 中
    lineStream >> conf.vDev;
}

/// @brief 读取配置文件中的音频采集设备
/// @param lineStream 
/// @param conf 
static void parseADev(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取输入，并将其赋值给 conf.aDev
    lineStream >> conf.aDev;
}

/// @brief 读取摄像头输入的图像宽高
/// @param lineStream 
/// @param conf 
static void parseIImgPara(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取2个输入，并将其赋值给 conf.iPara.width 和  conf.iPara.height
    lineStream >> conf.iPara.width;
    lineStream >> conf.iPara.height;
}

/// @brief 读取图像分辨率
/// @param lineStream 
/// @param conf 
static void parseOImgPara(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取2个输入，并将其赋值给 conf.oPara.width 和  conf.oPara.height
    lineStream >> conf.oPara.width;
    lineStream >> conf.oPara.height;
}

/// @brief 设置图像旋转角度
/// @param lineStream 
/// @param conf 
static void parseORotate(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    int r = 0;//给定默认值0，设置为不旋转
    lineStream >> r; // 该函数从 lineStream 中读取输入，并将其赋值给 r
    conf.oRotate = RgaRotate(r);
    // 将读取到的 r 转换为 RgaRotate 枚举或类型，然后赋值给 conf 对象的 oRotate 成员
    // 这意味着 RgaRotate 是一个枚举类型或自定义类型，代表某种编码类型
}

/// @brief 这段代码的主要目的是从 std::stringstream 中解析出一个字符串，并处理可能包含的引号。如果字符串是被双引号包围的，那么它会去除引号并提取其中的内容
/// @param lineStream
/// @param str
static void parseTextString(std::stringstream& lineStream, std::string& str)
{
    lineStream >> str; // 从 lineStream 中提取一个单词并存储到 str 中。这个单词是按空白字符（空格、制表符等）分隔的
    if (str[0] == '"') //双引号检查
    {
        std::string::size_type pos1 = 0;
        std::string::size_type pos2 = 0;
        std::string line = lineStream.str();
        if ((pos1 = line.find('\"')) != std::string::npos) 
        {
            pos2 = line.rfind('\"');
            if (pos2 != std::string::npos && pos2 > pos1) 
            {
                str.assign(line, pos1 + 1, pos2 - pos1 - 1);
            }
        }
    }
}

/// @brief 设置叠加字符串
/// @param lineStream 
/// @param conf 
static void parseOsdDevText(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取输入，并将其赋值给 conf.osdEtc
    parseTextString(lineStream, conf.osdDev);
}

/// @brief 其他叠加 OSD字符串的文件
/// @param lineStream 
/// @param conf 
static void parseOsdEtcFile(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取输入，并将其赋值给 conf.osdEtc
    lineStream >> conf.osdEtc;
}

/// @brief 设置叠加 FPS字符串
/// @param lineStream 
/// @param conf 
static void parseOsdFpsText(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取输入，并将其赋值给 conf.osdFps
    parseTextString(lineStream, conf.osdFps);
}

/// @brief 设置叠加FPS的差值
/// @param lineStream 
/// @param conf 
static void parseOsdFpsDiff(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取输入，并将其赋值给 conf.osdFpsDiff
    lineStream >> conf.osdFpsDiff;
}

/// @brief 叠加时间字符串
/// @param lineStream 
/// @param conf 
static void parseOsdTimeText(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    parseTextString(lineStream, conf.osdTime);
}

/// @brief 设置叠加字符串的参数
/// @param lineStream
/// @param conf
static void parseOsdTextPara(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{   // 该函数从 lineStream 中读取输入，并将其赋值给 conf.osdTextPara.point.x
    lineStream >> conf.osdTextPara.point.x;

    // 该函数从 lineStream 中读取输入，并将其赋值给 conf.osdTextPara.point.y
    lineStream >> conf.osdTextPara.point.y;

    // 该函数从 lineStream 中读取输入，并将其赋值给 conf.osdTextPara.fontFace
    lineStream >> conf.osdTextPara.fontFace;

    // 该函数从 lineStream 中读取输入，并将其赋值给 conf.osdTextPara.fontScale
    lineStream >> conf.osdTextPara.fontScale;

    // 该函数从 lineStream 中读取输入，并将其赋值给 conf.sdTextPara.color.val[0]
    lineStream >> conf.osdTextPara.color.val[0];

    // 该函数从 lineStream 中读取输入，并将其赋值给 conf.sdTextPara.color.val[1]
    lineStream >> conf.osdTextPara.color.val[1];

    // 该函数从 lineStream 中读取输入，并将其赋值给 conf.sdTextPara.color.val[2]
    lineStream >> conf.osdTextPara.color.val[2];
}

/// @brief 设置叠加时间戳字符串的参数
/// @param lineStream
/// @param conf
static void parseOsdTimestampPara(std::stringstream &lineStream, ModuleOsd::OsdConfPara &conf)
{
    lineStream >> conf.osdTimestampPara.point.x;
    lineStream >> conf.osdTimestampPara.point.y;
    lineStream >> conf.osdTimestampPara.fontFace;
    lineStream >> conf.osdTimestampPara.fontScale;
    lineStream >> conf.osdTimestampPara.color.val[0];
    lineStream >> conf.osdTimestampPara.color.val[1];
    lineStream >> conf.osdTimestampPara.color.val[2];
}

/// @brief 设置叠加fps信息字符串的参数
/// @param lineStream
/// @param conf
static void parseOsdFpsPara(std::stringstream &lineStream, ModuleOsd::OsdConfPara &conf)
{
    lineStream >> conf.osdFpsPara.point.x;
    lineStream >> conf.osdFpsPara.point.y;
    lineStream >> conf.osdFpsPara.fontFace;
    lineStream >> conf.osdFpsPara.fontScale;
    lineStream >> conf.osdFpsPara.color.val[0];
    lineStream >> conf.osdFpsPara.color.val[1];
    lineStream >> conf.osdFpsPara.color.val[2];
}

/// @brief 设置cpu温度信息字符串的参数
/// @param lineStream 
/// @param conf 
static void parseOsdCpuTempPara(std::stringstream &lineStream, ModuleOsd::OsdConfPara &conf)
{
    lineStream >> conf.osdCpuTempPara.point.x;
    lineStream >> conf.osdCpuTempPara.point.y;
    lineStream >> conf.osdCpuTempPara.fontFace;
    lineStream >> conf.osdCpuTempPara.fontScale;
    lineStream >> conf.osdCpuTempPara.color.val[0];
    lineStream >> conf.osdCpuTempPara.color.val[1];
    lineStream >> conf.osdCpuTempPara.color.val[2];
}

/// @brief 设置ssd温度及容量信息字符串参数
/// @param lineStream 
/// @param conf
static void parseOsdSsdDatePara(std::stringstream &lineStream, ModuleOsd::OsdConfPara &conf)
{
    lineStream >> conf.osdSsdDatePara.point.x;
    lineStream >> conf.osdSsdDatePara.point.y;
    lineStream >> conf.osdSsdDatePara.fontFace;
    lineStream >> conf.osdSsdDatePara.fontScale;
    lineStream >> conf.osdSsdDatePara.color.val[0];
    lineStream >> conf.osdSsdDatePara.color.val[1];
    lineStream >> conf.osdSsdDatePara.color.val[2];
}

/// @brief 设置设备名称OSD信息字符串的参数
/// @param lineStream 
/// @param conf 
static void parseOsdDevTextPara(std::stringstream &lineStream, ModuleOsd::OsdConfPara &conf)
{
    lineStream >> conf.osdDevTextPart.point.x;
    lineStream >> conf.osdDevTextPart.point.y;
    lineStream >> conf.osdDevTextPart.fontFace;
    lineStream >> conf.osdDevTextPart.fontScale;
    lineStream >> conf.osdDevTextPart.color.val[0];
    lineStream >> conf.osdDevTextPart.color.val[1];
    lineStream >> conf.osdDevTextPart.color.val[2];
}

    /// @brief 设置叠加字符串之间的行距
    /// @param lineStream
    /// @param conf
    static void parseOsdTextLineStep(std::stringstream &lineStream, ModuleOsd::OsdConfPara &conf)
{
    lineStream >> conf.LineStep;
}

/// @brief 编码类型指定
/// @param lineStream 
/// @param conf 
static void parseEncType(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    int type = 1;       //初始赋值1，为了默认让其设置为H265模式
    lineStream >> type; // 使用流提取运算符（>>）从 lineStream 中读取一个整数值，并存储到 type 变量中
    conf.eType = EncodeType(type);
    // 将读取到的 type 转换为 EncodeType 枚举或类型，然后赋值给 conf 对象的 eType 成员
    // 这意味着 eType 是一个枚举类型或自定义类型，代表某种编码类型
}

/// @brief 编码帧率设置
/// @param lineStream 
/// @param conf 
static void parseEncFps(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取输入，并将其赋值给 conf.eFps
    lineStream >> conf.eFps;
}

/// @brief 指定文件录制最大时长
/// @param lineStream 
/// @param conf 
static void parseFileMaxDuration(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取输入，并将其赋值给 conf.fMaxDuration
    lineStream >> conf.fMaxDuration;
    if (conf.fMaxDuration > 0) {
        conf.fMaxDuration *= 1000000;  // to s
    }
}

/// @brief 指定监控输出文件路径文件剩余大小字节，超过则输出该路径下最旧文件
/// @param lineStream
/// @param conf
static void parseFileSystemFreeSize(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取输入，并将其赋值给 conf.fFreeSize
    lineStream >> conf.fFreeSize;
}

/// @brief 指定输出文件
/// @param lineStream 
/// @param conf 
static void parseOutPutFile(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取输入，并将其赋值给 conf.oFile
    lineStream >> conf.oFile;
}

/// @brief 指定输出文件目录
/// @param lineStream 
/// @param conf 
static void parseOutPutFileDir(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取输入，并将其赋值给 conf.oFileDir
    lineStream >> conf.oFileDir;
}

/// @brief 屏幕显示功能使能
/// @param lineStream 
/// @param conf 
static void parseDisplayEnable(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.dDisplayEnable;
}

/// @brief 屏幕输出显示FPS设定
/// @param lineStream 
/// @param conf 
static void parseDisplayFps(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取输入，并将其赋值给 conf.dDisplayFps
    lineStream >> conf.dDisplayFps;
}

/// @brief 音频播放使能
/// @param lineStream
/// @param conf
static void parseAplayEnable(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.aPlayEnable;
}

/// @brief 指定音频播放的设备
/// @param lineStream
/// @param conf
static void parseAplayDevice(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.aPlayDev;
}

/// @brief 设置指定图层设备
/// @param lineStream 
/// @param conf 
static void parseDisplay(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取2个值，并将这些值存储到 conf 对象的 dDisplay 和 dDisplayConn 成员变量中
    lineStream >> conf.dDisplay >> conf.dDisplayConn;
}

/// @brief 设置屏幕限制区域
/// @param lineStream 
/// @param conf 
static void parseDisplayCorp(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取4个值，并将这些值存储到 conf 对象的 dCorp.x , dCorp.y , dCorp.w , dCorp.h 成员变量中
    lineStream >> conf.dCorp.x >> conf.dCorp.y >> conf.dCorp.w >> conf.dCorp.h;
}

/// @brief 视频推流开关
/// @param lineStream 
/// @param conf 
static void parsePushVideoEnable(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取输入，并将其赋值给 conf.pushVideoEnable
    lineStream >> conf.pushVideoEnable;
}

/// @brief 音频推流开关
/// @param lineStream 
/// @param conf 
static void parsePushAudioEnable(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.pushAudioEnable;
}

/// @brief 设置推流帧率
/// @param lineStream 
/// @param conf 
static void parsePushFps(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取输入，并将其赋值给 conf.pFps
    lineStream >> conf.pFps;
}

/// @brief 推流端口设置
/// @param lineStream 
/// @param conf 
static void parsePushPort(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取输入，并将其赋值给 conf.pPort
    lineStream >> conf.pPort;
}

/// @brief 推流路径设置
/// @param lineStream 
/// @param conf 
static void parsePushPath(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取输入，并将其赋值给 conf.pPath
    lineStream >> conf.pPath;
}

/// @brief 设置推流分辨率
/// @param lineStream 
/// @param conf 
static void parsePushResolution(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{ // 该函数从 lineStream 中读取2个输入，并将其赋值给 conf.pPara.width和conf.pPara.height
    lineStream >> conf.pPara.width >> conf.pPara.height;
}

/// @brief 
/// @param lineStream 
/// @param conf 
static void parsePushcpuTempEnable(std::stringstream &lineStream, ModuleOsd::OsdConfPara &conf)
{
    lineStream >> conf.pushcpuTempEnable;
}

/// @brief 
/// @param lineStream 
/// @param conf 
static void parsePushssdDataEnable(std::stringstream &lineStream, ModuleOsd::OsdConfPara &conf)
{
    lineStream >> conf.pushssdDataEnable;
}

/// @brief 广播ip地址
/// @param lineStream 
/// @param conf 
static void parseBroadcastPort(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.bPort;
}

static std::unordered_map<std::string, void (*)(std::stringstream &, ModuleOsd::OsdConfPara &)> osd_parsers = {
    {"vDev", parseVDev},                         // 视频节点配置
    {"aDev", parseADev},                         // 音频采集设置
    {"iImgPara", parseIImgPara},                 // 设置摄像头输出图像宽高
    {"oImgPara", parseOImgPara},                 // 设置调整图像分辨率
    {"oRotate", parseORotate},                   // 设置图像旋转角度
    {"osdDevText", parseOsdDevText},             // 设置叠加字符串
    {"osdFpsText", parseOsdFpsText},             // 设置叠加fps字符串
    {"osdFpsDiff", parseOsdFpsDiff},             // 设置叠加fps的差值
    {"osdTimeText", parseOsdTimeText},           // 叠加时间字符串
    {"osdEtcFile", parseOsdEtcFile},             // 其他叠加字符串的文件
    {"osdTextPara", parseOsdTextPara},           // 设置叠加字符串的参数
    {"osdLineStep", parseOsdTextLineStep},       // 设置叠加字符串之间行距
    {"encType", parseEncType},                   // 指定编码类型
    {"encFps", parseEncFps},                     // 指定编码帧率
    {"fileMaxDuration", parseFileMaxDuration},   // 指定文件录制最大时长
    {"systemFreeSize", parseFileSystemFreeSize}, // 指定监控输出文件路径文件剩余大小字节，超过则输出该路径下最旧文件
    {"outputFile", parseOutPutFile},             // 指定输出文件路径
    {"outputFileDir", parseOutPutFileDir},       // 指定输出文件目录
    {"displayEnable", parseDisplayEnable},       // 屏幕显示开启关闭
    {"aplayEnable", parseAplayEnable},           // 音频播放使能
    {"aplayDev", parseAplayDevice},              // 音频播放设备
    {"display", parseDisplay},                   // 设置指定图层设备
    {"displayFps", parseDisplayFps},             // 设置屏幕显示帧率
    {"displayCorp", parseDisplayCorp},           // 设置屏幕显示区域
    {"pushVideoEnable", parsePushVideoEnable},   // 视频推流开关
    {"pushAudioEnable", parsePushAudioEnable},   // 音频推流开关
    {"pushFps", parsePushFps},                   // 推流帧率设置
    {"pushPort", parsePushPort},                 // 推流端口设置
    {"pushPath", parsePushPath},                 // 推流路径设置
    {"pushPara", parsePushResolution},           // 设置推流分辨率

    {"cpuTempEnable", parsePushcpuTempEnable}, // CPU温度信息
    {"ssdDataEnable", parsePushssdDataEnable}, // SSD温度及容量信息

    {"osdTimestampPara", parseOsdTimestampPara}, // osd时间戳的位置及字体
    {"osdFpsPara", parseOsdFpsPara},             // osdfps的位置及字体
    {"osdCpuTempPara", parseOsdCpuTempPara},     // CPU温度信息的位置及字体
    {"osdSsdDatePara", parseOsdSsdDatePara},     // SSD容量及温度的位置及字体
    {"osdDevTextPart", parseOsdDevTextPara},     // 设备信息的位置及字体
    
    {"broadcastPort", parseBroadcastPort}        // 广播IP地址端口
};

ModuleOsd::ModuleOsd(const std::string& filename)
    : conf_file(filename), 
      work_flag(false), 
      work_thread(nullptr), 
      etc_file_mtime(0), 
      event(0),
      time_out(3000), 
      v_source(nullptr), 
      a_source(nullptr), 
      f_enc(nullptr), 
      writer(nullptr),
      r_enc(nullptr), 
      rtsp(nullptr), 
      display(nullptr), 
      current_pts(-1), 
      start_pts(-1)
{
}

ModuleOsd::~ModuleOsd()
{
    stop();
}

/// @brief 这段代码的目的是解析一个配置文件，将其中的配置信息加载到 OsdConfPara 对象中。
///        它逐行读取配置文件，跳过注释和空行，并根据每一行的关键字调用相应的解析函数来处理具体的配置项
/// @param conf
/// @return
int ModuleOsd::parseConfFileInfo(OsdConfPara& conf)
{
    if (conf_file.empty())  //配置文件路径是否为空
        return -1;          

    //读取配置文件内容
    char *fileData = readFile(conf_file.c_str()); // 使用 readFile 函数读取配置文件内容，并将其存储在 fileData 指针中
    if (!fileData)
        return -1;

    //创建字符串流
    std::stringstream strStream(fileData); // 将文件内容（fileData）加载到 std::stringstream 对象 strStream 中，用于逐行处理配置文件
    delete fileData;

    std::string temp;
    const int MAX_SIZE = 512;
    char lineStr[MAX_SIZE];

    // 使用 getline 方法从 strStream 中读取一行数据到 lineStr 中，直到文件末尾（eof）为止。
    while (!strStream.eof()) {
        strStream.getline(lineStr, MAX_SIZE);

        //跳过注释和空行
        if (lineStr[0] == '#' || lineStr[0] == '\0')
            continue;

        //解析每一行的数据
        std::stringstream lineStream(lineStr); // 创建一个 std::stringstream 对象 lineStream 来处理当前行的数据
        lineStream >> temp;                    // 从当前行中提取第一个单词到 temp 中（通常是配置项的关键字）
        auto it = osd_parsers.find(temp);      // 在 osd_parsers 容器（通常是一个 std::map 或 std::unordered_map）中查找该关键字
        if (it != osd_parsers.end())
        { // 如果找到匹配的关键字，则调用与之关联的解析函数 it->second，将 lineStream 和 conf 作为参数传入。这个解析函数将具体解析行中的参数并将其存储在 conf 中。
            it->second(lineStream, conf);
        }
    }
    return 0;
}

/// @brief 这段代码的主要功能是在检查 para.osdEtc 不为空的情况下，读取对应的文本文件，
///        将每一行内容存储在 osd_text 中。如果 para.osdDev 为空，则直接存储文本内容；
///        否则，将 para.osdDev 的值作为第一个元素插入，然后再插入文本内容。
///        此外，代码使用了互斥锁来确保多线程环境下的线程安全
void ModuleOsd::handletEtcText()
{
    // 首先检查 para.osdEtc 是否为空。如果为空，函数立即返回不做任何处理。para.osdEtc 可能是配置文件的路径或文件名
    if (!para.osdEtc.empty()) 
    {
        std::vector<std::string> v; // 定义一个 std::vector<std::string> 类型的变量 v，用于存储从文件中读取的每一行文本
        int ret = readFileLines(
                                para.osdEtc.c_str(), 
                                &etc_file_mtime, 
                                v);
        /* 调用 readFileLines 函数读取文件 para.osdEtc 的内容，并将每一行文本存储到 v 中。
         etc_file_mtime 是一个指向文件修改时间的指针，用来记录文件的最后修改时间。
         ret 用于存储 readFileLines 函数的返回值，通常表示成功读取的行数 */
        if (ret > 0) //文件读取结果
        {
            // 加锁与更新osd_text
            // 使用 std::lock_guard<std::mutex> 对象 lk 对 text_mtx 进行加锁，确保在多线程环境下对 osd_text 的访问是线程安全的
            std::lock_guard<std::mutex> lk(text_mtx);
            if (para.osdDev.empty())
            { // 如果 para.osdDev 为空，则直接将 v 中的内容赋值给 osd_text，即 osd_text = v;
                osd_text = v;
            }
            else
            { // 如果 para.osdDev 不为空
                osd_text.clear();                // 则先清空 osd_text
                osd_text.push_back(para.osdDev); // 然后将 para.osdDev 的值作为第一个元素插入到 osd_text 中
                for (auto it : v)                // ，接着将 v 中的内容逐行插入到 osd_text 中
                    osd_text.push_back(it);
            }
        }
    }
}

/// @brief 
/// @return 
int ModuleOsd::init()
{
    int ret;
    // shared_ptr 表示智能指针
    shared_ptr<ModuleCam> cam;
    shared_ptr<ModuleAlsaCapture> capture = nullptr;
    shared_ptr<ModuleMppEnc> enc = nullptr;
    shared_ptr<ModuleFileWriter> file_writer = nullptr;
    shared_ptr<ModuleMppEnc> enc_r = nullptr;
    shared_ptr<ModuleRtspServer> rtsp_s = nullptr;
    shared_ptr<ModuleDrmDisplay> drm_display = nullptr;

    shared_ptr<ModuleMedia> last_vmod = nullptr;
    shared_ptr<ModuleMedia> last_amod = nullptr;
    ImagePara output, input;

    //解析配置文件
    if (parseConfFileInfo(para)) {
        ff_error("Failed to parse Conf file\n");
        return -1;
    }

    //处理OSD文本信息
    handletEtcText();               // 调用 handletEtcText 处理OSD（On-Screen Display）文本
    if (!para.osdDev.empty())
    { // 如果 osdDev 不为空且 osd_text 为空，则将 osdDev 加入 osd_text。
        std::lock_guard<std::mutex> lk(text_mtx);
        if (osd_text.size() == 0)
        {
            osd_text.push_back(para.osdDev);
        }
    }

    //初始化视频设备，相机
    if (para.vDev.empty()) 
    {//若视频设备vDev为空，则返回错误
        printf("video device is empty\n");
        return -1;
    }
    //初始化视频设备模块，
    cam = make_shared<ModuleCam>(para.vDev);
    cam->setOutputImagePara(para.iPara);  // setOutputImage 设置输出图像参数
    cam->setProductor(NULL);//生产者为NULL
    cam->setBufferCount(12);//设置缓冲区数量为12
    ret = cam->init();
    if (ret < 0) {
        ff_error("camera init failed\n");
        return -1;
    }
    last_vmod = cam;

    //初始化音频设备
    if (!para.aDev.empty()) //若音频设备aDev不为空
    {
        SampleInfo info;
        info.channels = 2;
        info.fmt = SAMPLE_FMT_S16;
        info.nb_samples = 1024;
        info.sample_rate = 48000;
        capture = make_shared<ModuleAlsaCapture>(para.aDev, info);
        capture->setBufferCount(8);
        ret = capture->init();
        if (ret < 0) {
            ff_error("capture init failed\n");
            return -1;
        }
        last_amod = capture;
        auto aac_enc = make_shared<ModuleAacEnc>(info); // 初始化 AAC 编码器 ModuleAacEnc
        aac_enc->setProductor(last_amod);               // 并将其设置为音频模块的生产者
        ret = aac_enc->init();
        if (ret < 0) {
            ff_error("aac_enc init failed\n");
            return -1;
        }
        last_amod = aac_enc;
    }

    //处理视频解码
    output = last_vmod->getOutputImagePara();//获取上一个视频模块的输出图像参数output
    /*检查输出图像的格式是否为 MJPEG、H264 或 HEVC*/
    if (output.v4l2Fmt == V4L2_PIX_FMT_MJPEG || 
        output.v4l2Fmt == V4L2_PIX_FMT_H264 || 
        output.v4l2Fmt == V4L2_PIX_FMT_HEVC)
    {                                                               // 需要进行视频解码
        // 创建一个ModuleMppDec实例，这是一个视频解码器模块
        shared_ptr<ModuleMppDec> dec = make_shared<ModuleMppDec>();
        dec->setProductor(last_vmod);//将解码器设置为视频模块的生产者
        dec->setBufferCount(10);    //设置解码器模块的缓冲器数量为10
        ret = dec->init();
        if (ret < 0) 
        {
            ff_error("Dec init failed\n");
            return -1;
        }
        last_vmod = dec;
    }

    //图像参数处理与RGA模块初始化
    output = last_vmod->getOutputImagePara(); // 再次获取上一个视频模块的输出图像参数 output
    {
        output.v4l2Fmt = V4L2_PIX_FMT_BGR24; // 设置输出图像格式为 BGR24
        if (para.oPara.width && para.oPara.height)
        { // 若配置文件中给定了宽度和高度，则更新
            output.width = para.oPara.width;
            output.height = para.oPara.height;
            output.hstride = para.oPara.width;
            output.vstride = para.oPara.height;
        }
    }
    // 创建一个ModuleRga实例，这是一个图像处理模块（RGA，Rockchip Graphics Adapter）
    shared_ptr<ModuleRga> rga = make_shared<ModuleRga>(output, para.oRotate);
    rga->setProductor(last_vmod);//将RGA模块设置为上一个视频的生产者
    rga->setBufferCount(8); //RGA模块的缓冲区数量为8
    rga->setOutputDataCallback(this, osdHandlerCallback); // 设置 RGA 模块的输出数据回调函数 osdHandlerCallback
    rga->setRgaSchedulerCore(ModuleRga::SCHEDULER_RGA3_DEFAULT); // 设置 RGA 模块的调度核心为 SCHEDULER_RGA3_DEFAULT
    ret = rga->init();//初始化RGA模块
    if (ret < 0) {
        ff_error("rga init failed\n");
        return -1;
    }

    //显示模块初始化
    last_vmod = rga; // 将last_vmod更新为RGA
    if (para.dDisplayEnable && para.dDisplay >= 0) 
    {
        output = last_vmod->getOutputImagePara(); // 获取上一个视频模块的输出图像参数 output
        drm_display = make_shared<ModuleDrmDisplay>(); // 创建一个 ModuleDrmDisplay 实例，这是一个显示模块
        //设置显示模块的平面参数，格式，显示设备，平面设备，alpha值，层级，连接类型
        drm_display->setPlanePara(V4L2_PIX_FMT_NV12, 
                                  para.dDisplay,
                                  PLANE_TYPE_OVERLAY_OR_PRIMARY, 
                                  0xFF,
                                  1, 
                                  para.dDisplayConn);
        drm_display->setBufferCount(1);//设置显示模块的缓冲区数量为1
        drm_display->setProductor(last_vmod); // 将显示模块设置为上一个视频模块的生产者
        if (para.dDisplayFps > 0)
            drm_display->setDuration(1000000 / para.dDisplayFps); // 设置显示模块的持续时间为 1 秒除以帧率

        ret = drm_display->init(); // 初始化显示模块，如果初始化失败则移除消费者并返回错误
        if (ret < 0) 
        {
            ff_error("drm display init failed\n");
            last_vmod->removeConsumer(drm_display);
            drm_display = nullptr;
        } 
        else 
        {
            uint32_t t_h, t_v;
            drm_display->getPlaneSize(&t_h, &t_v); // 获取显示平面的尺寸
            uint32_t w, h, x, y;                   // 并根据配置参数 para.dCorp 计算显示区域的坐标和尺寸

            if (para.dCorp.h > 0 && para.dCorp.w > 0) 
            {
                x = std::min(t_h, para.dCorp.x);
                y = std::min(t_v, para.dCorp.y);
                w = std::min(t_h - x, para.dCorp.w);
                h = std::min(t_v - y, para.dCorp.h);
            } 
            else 
            {
                w = std::min(t_h, output.width);
                h = std::min(t_v, output.height);
                x = (t_h - w) / 2;
                y = (t_v - h) / 2;
            }
            ff_info("x y w h %d %d %d %d\n", x, y, w, h);
            drm_display->setPlaneRect(x, y, w, h);
        }
    }

    //音频播放模块初始化
    if (para.aPlayEnable && last_amod && !para.aPlayDev.empty()) {
        shared_ptr<ModuleAacDec> aac_dec = make_shared<ModuleAacDec>();
        aac_dec->setProductor(last_amod);
        aac_dec->setBufferCount(2);
        aac_dec->setAlsaDevice(para.aPlayDev);
        ret = aac_dec->init();
        if (ret < 0) {
            ff_error("Failed to init aac dec\n");
            last_amod->removeConsumer(aac_dec);
        }
    }

    //RTSP推流模块初始化
    if (para.pushVideoEnable && para.pPort > 0) 
    {
        output = last_vmod->getOutputImagePara();
        if (para.pPara.width && para.pPara.height) {
            output.width = para.pPara.width;
            output.height = para.pPara.height;
            output.hstride = output.width;
            output.vstride = output.height;
        }
        shared_ptr<ModuleRga> push_rga = make_shared<ModuleRga>(output, RgaRotate::RGA_ROTATE_NONE);
        push_rga->setProductor(last_vmod);
        push_rga->setBufferCount(2);
        if (para.pFps > 0)
            push_rga->setDuration(1000000 / para.pFps);
        ret = push_rga->init();
        if (ret < 0) {
            ff_error("Failed to init push rga\n");
            return ret;
        }
        
        /*
        enc_r = make_shared<ModuleMppEnc>(ENCODE_TYPE_H264,
                                          para.pFps, 
                                          para.pFps << 1, 
                                          (para.pFps / 30.0) * 2048);
        */
        enc_r = make_shared<ModuleMppEnc>(ENCODE_TYPE_H265,
                                          para.pFps,
                                          para.pFps << 1,
                                          (para.pFps / 30.0) * 2048);
        enc_r->setProductor(push_rga);
        enc_r->setBufferCount(6);
        enc_r->setDuration(0);
        ret = enc_r->init();
        if (ret < 0) {
            ff_error("Push Enc init failed\n");
            return ret;
        }

        rtsp_s = make_shared<ModuleRtspServer>(
            para.pPath.empty() ? "/live/0" : para.pPath.c_str(), para.pPort);
        rtsp_s->setProductor(enc_r);
        rtsp_s->setBufferCount(0);
        ret = rtsp_s->init();
        if (ret) {
            ff_error("rtsp server init failed\n");
            return ret;
        }
    }

    //音频推流模块初始化
    if (para.pushAudioEnable && last_amod) 
    {
        auto rtsp_s_a = make_shared<ModuleRtspServerExtend>(rtsp_s, para.pPath.c_str(), para.pPort);
        rtsp_s_a->setProductor(last_amod);
        rtsp_s_a->setAudioParameter(MEDIA_CODEC_AUDIO_AAC);
        ret = rtsp_s_a->init();
        if (ret) {
            ff_error("rtsp server audio init failed\n");
            last_amod->removeConsumer(rtsp_s_a);
            rtsp_s_a = nullptr;
        }
    }

    //文件写入模块与初始化
    if (!para.oFile.empty()) 
    {
        enc = make_shared<ModuleMppEnc>(para.eType, 
                                        para.eFps,
                                        para.eFps << 1, 
                                        (para.eFps / 30.0) * 2048);
        enc->setProductor(last_vmod);
        enc->setBufferCount(20);
        ret = enc->init();
        if (ret < 0) {
            ff_error("Enc init failed\n");
            return -1;
        }

        std::string filePath = para.oFileDir + ptsToTimeStr(0, para.oFile) + ".mp4";
        createDirectory(filePath.c_str());
        file_writer = make_shared<ModuleFileWriter>(filePath.c_str());
        file_writer->setProductor(enc);
        ret = file_writer->init();
        if (ret < 0) {
            ff_error("file writer init failed\n");
            return -1;
        }

        if (last_amod) {
            auto file_writer_a = make_shared<ModuleFileWriterExtend>(file_writer, string());
            file_writer_a->setProductor(last_amod);
            file_writer_a->setAudioParameter(0, 0, 0, MEDIA_CODEC_AUDIO_AAC);
            ret = file_writer_a->init();
            if (ret < 0) {
                ff_error("audio writer init failed\n");
                return -1;
            }
        }
    }

    //模块指针的报错和返回
    v_source = cam;
    a_source = capture;
    f_enc = enc;
    writer = file_writer;
    r_enc = enc_r;
    rtsp = rtsp_s;
    display = drm_display;
    return 0;
}

void ModuleOsd::start()
{
    if (v_source) {
        v_source->start();
        v_source->dumpPipe();
    }
    if (a_source) {
        a_source->dumpPipe();
        a_source->start();
    }

    work_flag = true;
    work_thread = work_thread ? work_thread : new std::thread(&ModuleOsd::osdProcess, this);
}

void ModuleOsd::stop()
{
    work_flag = false;
    {
        std::lock_guard<std::mutex> lk(event_mtx);
        event |= (1 << 1);
        event_conv.notify_one();
    }

    if (work_thread) {
        work_thread->join();
        delete work_thread;
        work_thread = nullptr;
    }

    if (a_source) {
        a_source->dumpPipeSummary();
        a_source->stop();
    }

    if (v_source) {
        v_source->dumpPipeSummary();
        v_source->stop();
    }
}

/// @brief  通过一个循环不断检查 event 的状态，根据不同的事件位进行文件操作和 OSD 文本更新，
///         同时还负责广播信息的发送
void ModuleOsd::osdProcess()
{
    while (work_flag) 
    {
        {
            //使用互斥锁 event_mtx 保护共享资源，以防止多个线程同时访问
            std::unique_lock<std::mutex> lk(event_mtx);
            if (event == 0)//如果 event 为 0，表示没有事件发生
                event_conv.wait_for(lk, std::chrono::milliseconds(time_out));
            if (event & (1 << 1)) 
            {
                continue;
            }
            else if (event & 1) 
            {
                event &= ~1;
                lk.unlock();
                //文件操作
                if (f_enc != nullptr && writer != nullptr) 
                {
                    f_enc->stop();//停止编码
                    std::string filePath = para.oFileDir + ptsToTimeStr(0, para.oFile) + ".mp4";
                    createDirectory(filePath.c_str());  //创建新的目录来保存新的文件
                    writer->changeFileName(filePath);   //更改文件写入器的文件路径
                    f_enc->start();//重新启动编码
                    if (para.fFreeSize)//磁盘空间管理
                    {                  // 如果设置了 para.fFreeSize，则循环检查当前磁盘空间是否足够，如果不够，则删除最旧的文件以释放空间
                        while (para.fFreeSize > getSystemFreeSize(filePath.c_str())) 
                        {
                            if (removeDirectoryOldestFile(para.oFileDir.empty() ? filePath.c_str() : para.oFileDir.c_str()) <= 0)
                                break;
                        }
                    }
                }
            }
        }
        handletEtcText();   //处理OSD文本信息
        // 如果 para.bPort 被设置且大于 0，并且 rtsp 对象为空或者没有客户端连接到 rtsp，则调用 sendBroadcastMessage 发送广播消息
        if (para.bPort > 0 && (!rtsp || rtsp->ClientsIsEmpty()))
            sendBroadcastMessage(para.bPort, para.osdDev.c_str(), nullptr); // 广播消息发送
    }
}

// 获取所有CPU核心的温度并计算平均温度
double getAverageCpuTemperature()
{
    std::vector<double> temperatures; // 用于存储所有核心的温度

    DIR *dir;
    struct dirent *entry;

    // 打开目录
    dir = opendir("/sys/class/thermal/");
    if (dir == nullptr)
    {
        return -1; // 如果无法打开目录，返回错误
    }

    // 遍历目录中的所有文件
    while ((entry = readdir(dir)) != nullptr)
    {
        if (strstr(entry->d_name, "thermal_zone") != nullptr)
        { // 找到thermal_zone文件夹
            std::string path = "/sys/class/thermal/";
            path += entry->d_name;
            path += "/temp";

            std::ifstream file(path);
            if (file)
            {
                double temp;
                file >> temp;                          // 读取温度数据
                temperatures.push_back(temp / 1000.0); // 转换为摄氏度并存储
            }
        }
    }

    closedir(dir); // 关闭目录

    // 计算温度平均值
    if (!temperatures.empty())
    {
        double sum = 0.0;
        for (double temp : temperatures)
        {
            sum += temp;
        }
        return sum / temperatures.size(); // 返回平均温度
    }

    return -1; // 如果没有读取到温度数据，则返回-1
}

/// @brief 回调函数，用于处理视频流中的每一帧数据，并在帧上叠加OSD（On-Screen Display）信息。这些信息可以包括文本、帧率、时间戳
/// @param arg
/// @param media_buf
void ModuleOsd::osdHandlerCallback(void* arg, std::shared_ptr<MediaBuffer> media_buf)
{
    //回调函数初始化
    ModuleOsd *osd = (ModuleOsd *)arg; // 将arg转换为ModuleOsd类型得到OSD模块的实例

    // 将media_buf强制转换为VideoBuffer类型以便访问视频帧数
    shared_ptr<VideoBuffer> buf = static_pointer_cast<VideoBuffer>(media_buf);

#ifdef TEST_DURATION
    auto start = std::chrono::high_resolution_clock::now();
#endif

    //图像数据准备
    void *ptr = buf->getActiveData(); // 使用 buf->getActiveData() 获取当前视频帧的有效数据指针
    uint32_t width = buf->getImagePara().hstride;//获取视频帧的高度
    uint32_t height = buf->getImagePara().vstride;//获取视频帧的宽度
    // 使用opencv的cv:Mat构造一个矩阵对象mat,该对象表示当前视频帧，用于后续的图像处理（如文本叠加）
    cv::Mat mat(cv::Size(width, height), CV_8UC3, ptr);
    
    //文本绘制准备
    cv::Point point = osd->para.osdTextPara.point;//初始化文本起始绘制位置point
    int line_h = osd->para.LineStep;//初始化行高
    {
        //绘制OSD文本
        std::lock_guard<std::mutex> lk(osd->text_mtx); // 使用 std::lock_guard 锁保护多线程环境下的文本数据访问
        int etc_count = osd->osd_text.size();

        //将文本文件中的OSD内容 和 设备编号OSD信息 分开显示
        point.x = osd->para.osdDevTextPart.point.x;
        point.y = osd->para.osdDevTextPart.point.y;
        putText(mat,
                osd->osd_text[0],
                point,
                osd->para.osdTextPara.fontFace,
                osd->para.osdTextPara.fontScale,
                osd->para.osdTextPara.color);

        point.x = osd->para.osdTextPara.point.x;
        point.y = osd->para.osdTextPara.point.y;
        for (int i = 1; i < etc_count; i++)
        { // 遍历 osd_text 列表，并在视频帧上逐行绘制文本
          // 其中putText 是 OpenCV 的函数，用于在图像上绘制文本。参数包括目标图像矩阵、文本内容、起始坐标、字体类型、字体大小和颜色等
            putText(mat, 
                    osd->osd_text[i], 
                    point, 
                    osd->para.osdTextPara.fontFace,
                    osd->para.osdTextPara.fontScale, 
                    osd->para.osdTextPara.color);
            point.y += line_h;
        }
        
        
    }

    //帧率绘制
    if (!osd->para.osdFps.empty()) // 检查 osdFps 参数是否为空
    {
        char text[256];
        // 通过时间戳计算当前帧率，并将其格式化为字符串
        if (snprintf(text, 
                     sizeof text,
                     osd->para.osdFps.c_str(),
                     1000000 / (buf->getPUstimestamp() - osd->current_pts) + osd->para.osdFpsDiff) > 0) 
        {
            // 更新帧率绘制位置
            //point.x = 1500; // 设置x坐标为图像宽度的一半
            //point.y = 1050;    // 设置y坐标为行高

            point.x = osd->para.osdFpsPara.point.x;
            point.y = osd->para.osdFpsPara.point.y;

            putText(mat,
                    text,
                    point,
                    osd->para.osdFpsPara.fontFace,
                    osd->para.osdFpsPara.fontScale,
                    osd->para.osdFpsPara.color);
            point.y += line_h;
        }
    }

    //绘制时间戳
    if (!osd->para.osdTime.empty()) // 检查 osdTime 参数是否为空
    {
        //point.x = 10; // 设置x坐标为图像宽度的一半
        //point.y = 1050;  // 设置y坐标为行高
        point.x = osd->para.osdTimestampPara.point.x;
        point.y = osd->para.osdTimestampPara.point.y;

        // 使用 putText 函数将时间戳信息绘制到视频帧上
        putText(mat,
                ptsToTimeStr(0, osd->para.osdTime.c_str()),
                point,
                osd->para.osdTimestampPara.fontFace,
                osd->para.osdTimestampPara.fontScale,
                osd->para.osdTimestampPara.color);
    }


    // 检查是否需要更新OSD数据 
    auto now = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(now - last_update_time).count();
    if (elapsed_time >= 10)
    { // 每10秒更新一次
        updateOSDData();
        last_update_time = now; // 更新最后一次更新时间
    }

    // 获取并绘制CPU温度
    //double cpuTemp = getAverageCpuTemperature();
    if (cached_cpu_temp > 0 && osd->para.pushcpuTempEnable == true)
    //if (cached_cpu_temp > 0 && para.pushcpuTempEnable == true)
    //if (cached_cpu_temp > 0 && instance->para.pushcpuTempEnable == true)
    { // 仅当成功获取到温度时才显示
        char tempText[50];
        snprintf(tempText, sizeof(tempText), "Avg CPU Temp: %.2fC", cached_cpu_temp);

        //point.x = 1000; // 设置x坐标为图像宽度的一半
        //point.y = 1050; // 设置y坐标为行高
        point.x = osd->para.osdCpuTempPara.point.x;
        point.y = osd->para.osdCpuTempPara.point.y;
        putText(mat,
                tempText,
                point,
                osd->para.osdCpuTempPara.fontFace,
                osd->para.osdCpuTempPara.fontScale,
                osd->para.osdCpuTempPara.color);
    }

    // 检查SSD挂载状态
    //if (!cached_ssd_info.empty() && para.pushssdDataEnable == true)
    if (!cached_ssd_info.empty() && osd->para.pushssdDataEnable == true)
    {
        char ssdInfo[100];

        /*
        snprintf(ssdInfo, 
                 sizeof(ssdInfo), 
                "SSD Info: %s, %.2fG/%.2fG Free", 
                cached_ssd_info.c_str(), 
                cached_filesystem_usage.first, 
                cached_filesystem_usage.second);
        */

        int ret = snprintf(ssdInfo,
                           sizeof(ssdInfo),
                           "SSD : %s, %.2fG/%.2fG Free",
                           cached_ssd_info.c_str(),
                           cached_filesystem_usage.first,
                           cached_filesystem_usage.second);

        //if (ret < 0 || ret >= sizeof(ssdInfo))
        if (ret < 0 || static_cast<size_t>(ret) >= sizeof(ssdInfo))
        {
            // 错误处理：snprintf 失败或输出被截断
            fprintf(stderr, "Failed to format SSD info or output was truncated.\n");
        }

        //point.x = 500; // 设置x坐标为图像宽度的一半
        //point.y = 50;  // 设置y坐标为行高
        point.x = osd->para.osdSsdDatePara.point.x;
        point.y = osd->para.osdSsdDatePara.point.y;
        putText(mat,
                ssdInfo,
                point,
                osd->para.osdSsdDatePara.fontFace,
                osd->para.osdSsdDatePara.fontScale,
                osd->para.osdSsdDatePara.color);
    }
    else
    {
        // SSD未挂载，显示-1
        //char ssdInfo[50];
        //snprintf(ssdInfo, sizeof(ssdInfo), "SSD: -1");
        //point.x = 500; // 设置x坐标为图像宽度的一半
        //point.y = 50;  // 设置y坐标为行高
        //putText(mat,
        //        ssdInfo,
        //        point,
        //        osd->para.osdTextPara.fontFace,
        //        osd->para.osdTextPara.fontScale,
        //        osd->para.osdTextPara.color);
    }

    //更新当前时间戳
    osd->current_pts = buf->getPUstimestamp(); // 更新 current_pts 为当前帧的时间戳
    buf->invalidateDrmBuf();                   // 调用 invalidateDrmBuf 方法标记缓冲区为无效

#ifdef TEST_DURATION //测试持续时间计算
    auto end = std::chrono::high_resolution_clock::now();
    ff_info("duration %ld\n", std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
#endif

    //条件触发事件
    if (osd->start_pts < 0) // 检查 start_pts 是否为负数，如果是，则初始化为当前时间戳
        osd->start_pts = osd->current_pts;
    else if (osd->para.fMaxDuration && osd->current_pts - osd->start_pts > osd->para.fMaxDuration)
    { // 如果 fMaxDuration 参数有效，且当前时间戳与起始时间戳之差大于 fMaxDuration，则触发事件
        std::lock_guard<std::mutex> lk(osd->event_mtx); // 锁定互斥量，修改事件状态，通知等待的条件变量，然后更新起始时间戳
        osd->event |= 1;
        osd->event_conv.notify_one();
        osd->start_pts = osd->current_pts;
    }
}
