#include "osd.hpp"

#include <iostream>
#include <sstream>
#include <vector>
#include <string.h>

static void parseVDev(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.vDev;
}

static void parseADev(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.aDev;
}

static void parseIImgPara(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.iPara.width;
    lineStream >> conf.iPara.height;
}

static void parseOImgPara(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.oPara.width;
    lineStream >> conf.oPara.height;
}

static void parseORotate(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    int r = 0;
    lineStream >> r;
    conf.oRotate = RgaRotate(r);
}

static void parseTextString(std::stringstream& lineStream, std::string& str)
{
    lineStream >> str;
    if (str[0] == '"') {
        std::string::size_type pos1 = 0;
        std::string::size_type pos2 = 0;
        std::string line = lineStream.str();
        if ((pos1 = line.find('\"')) != std::string::npos) {
            pos2 = line.rfind('\"');
            if (pos2 != std::string::npos && pos2 > pos1) {
                str.assign(line, pos1 + 1, pos2 - pos1 - 1);
            }
        }
    }
}

static void parseOsdDevText(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    parseTextString(lineStream, conf.osdDev);
}

static void parseOsdEtcFile(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.osdEtc;
}

static void parseOsdFpsText(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    parseTextString(lineStream, conf.osdFps);
}

static void parseOsdFpsDiff(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.osdFpsDiff;
}

static void parseOsdTimeText(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    parseTextString(lineStream, conf.osdTime);
}

static void parseOsdTextPara(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.osdTextPara.point.x;
    lineStream >> conf.osdTextPara.point.y;
    lineStream >> conf.osdTextPara.fontFace;
    lineStream >> conf.osdTextPara.fontScale;
    lineStream >> conf.osdTextPara.color.val[0];
    lineStream >> conf.osdTextPara.color.val[1];
    lineStream >> conf.osdTextPara.color.val[2];
}

static void parseOsdTextLineStep(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.LineStep;
}

static void parseEncType(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    int type = 1;
    lineStream >> type;
    conf.eType = EncodeType(type);
}

static void parseEncFps(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.eFps;
}

static void parseFileMaxDuration(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.fMaxDuration;
    if (conf.fMaxDuration > 0) {
        conf.fMaxDuration *= 1000000;  // to s
    }
}

static void parseFileSystemFreeSize(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.fFreeSize;
}

static void parseOutPutFile(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.oFile;
}

static void parseOutPutFileDir(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.oFileDir;
}

static void parseDisplayEnable(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.dDisplayEnable;
}

static void parseDisplayFps(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.dDisplayFps;
}

static void parseAplayEnable(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.aPlayEnable;
}

static void parseAplayDevice(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.aPlayDev;
}

static void parseDisplay(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.dDisplay >> conf.dDisplayConn;
}

static void parseDisplayCorp(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.dCorp.x >> conf.dCorp.y >> conf.dCorp.w >> conf.dCorp.h;
}

static void parsePushVideoEnable(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.pushVideoEnable;
}

static void parsePushAudioEnable(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.pushAudioEnable;
}

static void parsePushFps(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.pFps;
}

static void parsePushPort(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.pPort;
}

static void parsePushPath(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.pPath;
}

static void parsePushResolution(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.pPara.width >> conf.pPara.height;
}

static void parseBroadcastPort(std::stringstream& lineStream, ModuleOsd::OsdConfPara& conf)
{
    lineStream >> conf.bPort;
}


static std::unordered_map<std::string, void (*)(std::stringstream&, ModuleOsd::OsdConfPara&)> osd_parsers = {
    {"vDev", parseVDev},
    {"aDev", parseADev},
    {"iImgPara", parseIImgPara},
    {"oImgPara", parseOImgPara},
    {"oRotate", parseORotate},
    {"osdDevText", parseOsdDevText},
    {"osdFpsText", parseOsdFpsText},
    {"osdFpsDiff", parseOsdFpsDiff},
    {"osdTimeText", parseOsdTimeText},
    {"osdEtcFile", parseOsdEtcFile},
    {"osdTextPara", parseOsdTextPara},
    {"osdLineStep", parseOsdTextLineStep},
    {"encType", parseEncType},
    {"encFps", parseEncFps},
    {"fileMaxDuration", parseFileMaxDuration},
    {"systemFreeSize", parseFileSystemFreeSize},
    {"outputFile", parseOutPutFile},
    {"outputFileDir", parseOutPutFileDir},
    {"displayEnable", parseDisplayEnable},
    {"aplayEnable", parseAplayEnable},
    {"aplayDev", parseAplayDevice},
    {"display", parseDisplay},
    {"displayFps", parseDisplayFps},
    {"displayCorp", parseDisplayCorp},
    {"pushVideoEnable", parsePushVideoEnable},
    {"pushAudioEnable", parsePushAudioEnable},
    {"pushFps", parsePushFps},
    {"pushPort", parsePushPort},
    {"pushPath", parsePushPath},
    {"pushPara", parsePushResolution},
    {"broadcastPort", parseBroadcastPort}};


ModuleOsd::ModuleOsd(const std::string& filename)
    : conf_file(filename), work_flag(false), work_thread(nullptr), etc_file_mtime(0), event(0),
      time_out(3000), v_source(nullptr), a_source(nullptr), f_enc(nullptr), writer(nullptr),
      r_enc(nullptr), rtsp(nullptr), display(nullptr), current_pts(-1), start_pts(-1)
{
}

ModuleOsd::~ModuleOsd()
{
    stop();
}

int ModuleOsd::parseConfFileInfo(OsdConfPara& conf)
{
    if (conf_file.empty())
        return -1;

    char* fileData = readFile(conf_file.c_str());
    if (!fileData)
        return -1;

    std::stringstream strStream(fileData);
    delete fileData;

    std::string temp;
    const int MAX_SIZE = 512;
    char lineStr[MAX_SIZE];

    while (!strStream.eof()) {
        strStream.getline(lineStr, MAX_SIZE);

        if (lineStr[0] == '#' || lineStr[0] == '\0')
            continue;

        std::stringstream lineStream(lineStr);
        lineStream >> temp;
        auto it = osd_parsers.find(temp);
        if (it != osd_parsers.end()) {
            it->second(lineStream, conf);
        }
    }
    return 0;
}

void ModuleOsd::handletEtcText()
{
    if (!para.osdEtc.empty()) {
        std::vector<std::string> v;
        int ret = readFileLines(para.osdEtc.c_str(), &etc_file_mtime, v);
        if (ret > 0) {
            std::lock_guard<std::mutex> lk(text_mtx);
            if (para.osdDev.empty()) {
                osd_text = v;
            } else {
                osd_text.clear();
                osd_text.push_back(para.osdDev);
                for (auto it : v)
                    osd_text.push_back(it);
            }
        }
    }
}

int ModuleOsd::init()
{
    int ret;
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

    if (parseConfFileInfo(para)) {
        ff_error("Failed to parse Conf file\n");
        return -1;
    }

    handletEtcText();
    if (!para.osdDev.empty()) {
        std::lock_guard<std::mutex> lk(text_mtx);
        if (osd_text.size() == 0)
            osd_text.push_back(para.osdDev);
    }

    if (para.vDev.empty()) {
        printf("video device is empty\n");
        return -1;
    }

    cam = make_shared<ModuleCam>(para.vDev);
    cam->setOutputImagePara(para.iPara);  // setOutputImage
    cam->setProductor(NULL);
    cam->setBufferCount(12);
    ret = cam->init();
    if (ret < 0) {
        ff_error("camera init failed\n");
        return -1;
    }
    last_vmod = cam;

    if (!para.aDev.empty()) {
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
        auto aac_enc = make_shared<ModuleAacEnc>(info);
        aac_enc->setProductor(last_amod);
        ret = aac_enc->init();
        if (ret < 0) {
            ff_error("aac_enc init failed\n");
            return -1;
        }
        last_amod = aac_enc;
    }

    output = last_vmod->getOutputImagePara();
    if (output.v4l2Fmt == V4L2_PIX_FMT_MJPEG || output.v4l2Fmt == V4L2_PIX_FMT_H264
        || output.v4l2Fmt == V4L2_PIX_FMT_HEVC) {
        shared_ptr<ModuleMppDec> dec = make_shared<ModuleMppDec>();
        dec->setProductor(last_vmod);
        dec->setBufferCount(10);
        ret = dec->init();
        if (ret < 0) {
            ff_error("Dec init failed\n");
            return -1;
        }
        last_vmod = dec;
    }

    output = last_vmod->getOutputImagePara();
    {
        output.v4l2Fmt = V4L2_PIX_FMT_BGR24;
        if (para.oPara.width && para.oPara.height) {
            output.width = para.oPara.width;
            output.height = para.oPara.height;
            output.hstride = para.oPara.width;
            output.vstride = para.oPara.height;
        }
    }

    shared_ptr<ModuleRga> rga = make_shared<ModuleRga>(output, para.oRotate);
    rga->setProductor(last_vmod);
    rga->setBufferCount(8);
    rga->setOutputDataCallback(this, osdHandlerCallback);
    rga->setRgaSchedulerCore(ModuleRga::SCHEDULER_RGA3_DEFAULT);
    ret = rga->init();
    if (ret < 0) {
        ff_error("rga init failed\n");
        return -1;
    }

    last_vmod = rga;
    if (para.dDisplayEnable && para.dDisplay >= 0) {
        output = last_vmod->getOutputImagePara();
        drm_display = make_shared<ModuleDrmDisplay>();
        drm_display->setPlanePara(V4L2_PIX_FMT_NV12, para.dDisplay,
                                  PLANE_TYPE_OVERLAY_OR_PRIMARY, 0xFF,
                                  1, para.dDisplayConn);
        drm_display->setBufferCount(1);
        drm_display->setProductor(last_vmod);
        if (para.dDisplayFps > 0)
            drm_display->setDuration(1000000 / para.dDisplayFps);

        ret = drm_display->init();
        if (ret < 0) {
            ff_error("drm display init failed\n");
            last_vmod->removeConsumer(drm_display);
            drm_display = nullptr;
        } else {
            uint32_t t_h, t_v;
            drm_display->getPlaneSize(&t_h, &t_v);
            uint32_t w, h, x, y;

            if (para.dCorp.h > 0 && para.dCorp.w > 0) {
                x = std::min(t_h, para.dCorp.x);
                y = std::min(t_v, para.dCorp.y);
                w = std::min(t_h - x, para.dCorp.w);
                h = std::min(t_v - y, para.dCorp.h);
            } else {
                w = std::min(t_h, output.width);
                h = std::min(t_v, output.height);
                x = (t_h - w) / 2;
                y = (t_v - h) / 2;
            }
            ff_info("x y w h %d %d %d %d\n", x, y, w, h);
            drm_display->setPlaneRect(x, y, w, h);
        }
    }
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

    if (para.pushVideoEnable && para.pPort > 0) {
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

        enc_r = make_shared<ModuleMppEnc>(ENCODE_TYPE_H264,
                                          para.pFps, para.pFps << 1, (para.pFps / 30.0) * 2048);
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

    if (para.pushAudioEnable && last_amod) {
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

    if (!para.oFile.empty()) {
        enc = make_shared<ModuleMppEnc>(para.eType, para.eFps,
                                        para.eFps << 1, (para.eFps / 30.0) * 2048);
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

void ModuleOsd::osdProcess()
{
    while (work_flag) {
        {
            std::unique_lock<std::mutex> lk(event_mtx);
            if (event == 0)
                event_conv.wait_for(lk, std::chrono::milliseconds(time_out));
            if (event & (1 << 1)) {
                continue;
            } else if (event & 1) {
                event &= ~1;
                lk.unlock();
                if (f_enc != nullptr && writer != nullptr) {
                    f_enc->stop();
                    std::string filePath = para.oFileDir + ptsToTimeStr(0, para.oFile) + ".mp4";
                    createDirectory(filePath.c_str());
                    writer->changeFileName(filePath);
                    f_enc->start();
                    if (para.fFreeSize) {
                        while (para.fFreeSize > getSystemFreeSize(filePath.c_str())) {
                            if (removeDirectoryOldestFile(para.oFileDir.empty() ? filePath.c_str() : para.oFileDir.c_str()) <= 0)
                                break;
                        }
                    }
                }
            }
        }

        handletEtcText();

        if (para.bPort > 0 && (!rtsp || rtsp->ClientsIsEmpty()))
            sendBroadcastMessage(para.bPort, para.osdDev.c_str(), nullptr);
    }
}

/// @brief 回调函数，用于处理视频流中的每一帧数据，并在帧上叠加OSD（On-Screen Display）信息。这些信息可以包括文本、帧率、时间戳
/// @param arg
/// @param media_buf
void ModuleOsd::osdHandlerCallback(void* arg, std::shared_ptr<MediaBuffer> media_buf)
{
    //回调函数初始化
    ModuleOsd* osd = (ModuleOsd*)arg;
    shared_ptr<VideoBuffer> buf = static_pointer_cast<VideoBuffer>(media_buf);

#ifdef TEST_DURATION
    auto start = std::chrono::high_resolution_clock::now();
#endif

    //图像数据准备
    void* ptr = buf->getActiveData();
    uint32_t width = buf->getImagePara().hstride;
    uint32_t height = buf->getImagePara().vstride;
    cv::Mat mat(cv::Size(width, height), CV_8UC3, ptr);
    
    //文本绘制准备
    cv::Point point = osd->para.osdTextPara.point;
    int line_h = osd->para.LineStep;
    {
        //绘制OSD文本
        std::lock_guard<std::mutex> lk(osd->text_mtx);
        int etc_count = osd->osd_text.size();
        for (int i = 0; i < etc_count; i++) {
            putText(mat, osd->osd_text[i], point, osd->para.osdTextPara.fontFace,
                    osd->para.osdTextPara.fontScale, osd->para.osdTextPara.color);
            point.y += line_h;
        }
    }

    //帧率绘制
    if (!osd->para.osdFps.empty()) {
        char text[256];
        if (snprintf(text, sizeof text, osd->para.osdFps.c_str(),
                     1000000 / (buf->getPUstimestamp() - osd->current_pts)
                         + osd->para.osdFpsDiff)
            > 0) {
            putText(mat, 
                    text, 
                    point, 
                    osd->para.osdTextPara.fontFace,
                    osd->para.osdTextPara.fontScale, 
                    osd->para.osdTextPara.color);
            point.y += line_h;
        }
    }

    //绘制时间戳
    if (!osd->para.osdTime.empty())
    {
        putText(mat, 
                ptsToTimeStr(0, osd->para.osdTime.c_str()), 
                point, 
                osd->para.osdTextPara.fontFace,
                osd->para.osdTextPara.fontScale, 
                osd->para.osdTextPara.color);
    }


    //更新当前时间戳
    osd->current_pts = buf->getPUstimestamp();
    buf->invalidateDrmBuf();

#ifdef TEST_DURATION
    auto end = std::chrono::high_resolution_clock::now();
    ff_info("duration %ld\n", std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
#endif

    //条件触发事件
    if (osd->start_pts < 0)
        osd->start_pts = osd->current_pts;
    else if (osd->para.fMaxDuration && osd->current_pts - osd->start_pts > osd->para.fMaxDuration) {
        std::lock_guard<std::mutex> lk(osd->event_mtx);
        osd->event |= 1;
        osd->event_conv.notify_one();
        osd->start_pts = osd->current_pts;
    }
}
