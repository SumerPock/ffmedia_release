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

class ModuleOsd
{
public:
    struct OsdTextPara {
        cv::Point point = cv::Point(0, 25);
        int fontFace = 0;
        double fontScale = 1.0;
        cv::Scalar color = cv::Scalar(0, 255, 255);
    };
    struct OsdConfPara {
        std::string vDev;
        std::string aDev;
        ImagePara iPara;
        ImagePara oPara;
        RgaRotate oRotate = RgaRotate::RGA_ROTATE_NONE;

        std::string osdDev;
        std::string osdEtc;
        std::string osdFps;
        std::string osdTime;
        OsdTextPara osdTextPara;
        int LineStep = 30;
        int osdFpsDiff = 0;

        EncodeType eType = EncodeType::ENCODE_TYPE_H265;
        int eFps = 120;
        int fMaxDuration = 0;
        uint64_t fFreeSize = 0;
        std::string oFile;
        std::string oFileDir;

        bool dDisplayEnable = false;
        bool aPlayEnable = false;
        std::string aPlayDev;
        int dDisplay = -1;
        int dDisplayConn = 0;
        int dDisplayFps = 0;
        ImageCrop dCorp = {0};

        bool pushVideoEnable = false;
        bool pushAudioEnable = false;
        int pFps = 30;
        int pPort = 0;
        std::string pPath;
        ImagePara pPara;
        int bPort = 0;
    };

public:
    ModuleOsd(const std::string& filename);
    ~ModuleOsd();

    int init();
    void start();
    void stop();

protected:
    void osdProcess();
    static void osdHandlerCallback(void* arg, std::shared_ptr<MediaBuffer> buf);

    void handletEtcText();

private:
    int parseConfFileInfo(OsdConfPara& conf);

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
