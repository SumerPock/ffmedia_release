#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <signal.h>
#include <thread>
#include <unordered_map>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>


#include "module/vi/module_cam.hpp"
#include "module/vi/module_rtspClient.hpp"
#include "module/vi/module_rtmpClient.hpp"
#include "module/vi/module_fileReader.hpp"
#include "module/vp/module_rga.hpp"
#include "module/vp/module_mppdec.hpp"
#include "module/vp/module_mppenc.hpp"
#include "module/vo/module_fileWriter.hpp"
#include "module/vo/module_drmDisplay.hpp"
#include "module/vo/module_rtspServer.hpp"
#include "module/vo/module_rtmpServer.hpp"

#include "module/vp/module_aacdec.hpp"
#include "module/vp/module_aacenc.hpp"
#include "module/vi/module_alsaCapture.hpp"

#include "system_common.hpp"

/// @brief 
class ModuleOsd
{
public:
    // 结构体OsdTextPara 用于定义与文本相关的参数
    struct OsdTextPara {
        cv::Point point = cv::Point(0, 25);
        int fontFace = 0;
        double fontScale = 1.0;
        cv::Scalar color = cv::Scalar(0, 255, 255);
    };

    // 结构体OsdConfPara 用于与配置文件相对应
    struct OsdConfPara
    { // 其中std::string用于表示操作字符串
        std::string vDev; // 摄像头设备节点字符串
        std::string aDev; // 音频采集设备节点字符串
        ImagePara iPara;  // 摄像头输出结构体
        ImagePara oPara;  // 调整图像分辨率结构体
        RgaRotate oRotate = RgaRotate::RGA_ROTATE_NONE; // 图像旋转角度枚举，并与之赋初始值，不旋转

        std::string osdDev;
        std::string osdEtc;           // 设置读取文本文件，并叠加到OSD上
        std::string osdFps;           // 设置叠加FPS字符串
        std::string osdTime;          // OSD时间戳
        OsdTextPara osdTextPara;      // 设定叠加字符串的参数
        OsdTextPara osdTimestampPara; // 设定叠加时间戳的参数
        OsdTextPara osdFpsPara;       // 设定帧率的参数
        OsdTextPara osdSsdDatePara;   // 设定ssd信息的字符串参数
        OsdTextPara osdCpuTempPara;   // 设定cpu温度的字符串参数
        OsdTextPara osdDevTextPart;   // 设定设备字符串的参数

        int LineStep = 30;      // 输出文本文件中多行数据时的行高
        int osdFpsDiff = 0;     // OSD输出FPS补偿数据

        EncodeType eType = EncodeType::ENCODE_TYPE_H265;// 编码类型枚举，并与之赋初始值，H265
        int eFps = 120;         // 指定编码频率
        int fMaxDuration = 0;   // 指定文件最大录制时长
        uint64_t fFreeSize = 0; // 循环空间
        std::string oFile;      // 存储文件名
        std::string oFileDir;   // 存储文件目录

        bool dDisplayEnable = false; // 屏幕显示功能使能
        bool aPlayEnable = false;    // 音频播放功能使能
        std::string aPlayDev;        // 指定音频播放设备
        int dDisplay = -1;           // 开始屏幕输出使能
        int dDisplayConn = 0;        // 指定图层和设备
        int dDisplayFps = 0;         // 设定屏幕输出FPS
        ImageCrop dCorp = {0};       // 设定屏幕显示区域

        bool pushVideoEnable = false;   // 视频推流开关
        bool pushAudioEnable = false;   // 音频推流开关
        int pFps = 30;          // 视频推流帧率设置
        int pPort = 0;          // 视频推流端口设置
        std::string pPath;      // 推流路径设置
        ImagePara pPara;        // 推流分辨率设定
        int bPort = 0;          // 设置广播IP地址

        bool pushcpuTempEnable = false; // cpu温度OSD显示使能
        bool pushssdDataEnable = false; // ssd温度容量OSD显示使能
    };

public:
    //构造
    ModuleOsd(const std::string& filename);
    //析构
    ~ModuleOsd();

    int init();
    void start();
    void stop();

protected:
    void osdProcess();
    static void osdHandlerCallback(void* arg, std::shared_ptr<MediaBuffer> buf);
    //static void osdHandlerCallback(ModuleOsd *instance, void *arg, std::shared_ptr<MediaBuffer> buf);
    //static void osdHandlerCallback(ModuleOsd *instance, void *arg, std::shared_ptr<MediaBuffer> buf);
    void handletEtcText();
private://私有成员函数
    int parseConfFileInfo(OsdConfPara &conf); // 解析配置文件信息，填充OsdConfPara结构体

private:
    std::string conf_file;
    OsdConfPara para;

    bool work_flag;
    std::thread* work_thread;

    std::mutex text_mtx;
    std::vector<std::string> osd_text;
    time_t etc_file_mtime;

    std::mutex event_mtx;
    std::condition_variable event_conv;
    uint32_t event = 0;
    uint32_t time_out = 3000;

    shared_ptr<ModuleMedia> v_source;
    shared_ptr<ModuleMedia> a_source;

    shared_ptr<ModuleMppEnc> f_enc;
    shared_ptr<ModuleFileWriter> writer;

    shared_ptr<ModuleMppEnc> r_enc;
    shared_ptr<ModuleRtspServer> rtsp;

    shared_ptr<ModuleDrmDisplay> display;

    int64_t current_pts = -1;
    int64_t start_pts = -1;
};
