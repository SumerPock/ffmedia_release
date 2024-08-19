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
    output = last_vmod->getOutputImagePara();
    if (output.v4l2Fmt == V4L2_PIX_FMT_MJPEG || output.v4l2Fmt == V4L2_PIX_FMT_H264
        || output.v4l2Fmt == V4L2_PIX_FMT_HEVC) {
        shared_ptr<ModuleMppDec> dec = make_shared<ModuleMppDec>(); // 初始化视频解码器ModuleMppDec
        dec->setProductor(last_vmod);//将解码器设置为视频模块的生产者
        dec->setBufferCount(10);
        ret = dec->init();
        if (ret < 0) {
            ff_error("Dec init failed\n");
            return -1;
        }
        last_vmod = dec;
    }

    //图像参数处理与RGA模块初始化
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

    //显示模块初始化
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

        enc_r = make_shared<ModuleMppEnc>(ENCODE_TYPE_H264,
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
