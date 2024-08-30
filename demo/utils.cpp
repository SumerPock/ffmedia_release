#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <linux/types.h>
#include "utils.hpp"
#include "base/ff_log.h"

/// @brief 将视频缓冲区的内容直接写入文件中，不像之前复杂的dump_videobuffer_to_file函数需要根据格式进行额外的处理
/// @param buffer 一个指向VideoBuffer对象的共享指针，包含需要写入文件的视频数据
/// @param fp 一个指向文件对象的指针，视频数据将被写入这个文件
void dump_normalbuffer_to_file(shared_ptr<VideoBuffer> buffer, FILE* fp)
{
    // 防御性编程的手法，用来防止函数在尝试写入一个不存在的文件或者访问一个空的缓冲区时导致运行时错误或程序崩溃
    if (NULL == fp || NULL == buffer)
        return;

    // 获取有效数据和大小
    unsigned char* base = (unsigned char*)(buffer->getActiveData());

    // 调用返回有效数据的大小，表示需要写入文件的字节数
    size_t size = buffer->getActiveSize();

    // 将视频缓冲区的数据写入到文件中
    fwrite(base, 1, size, fp);
}

/// @brief 将视频缓冲区的数据写入到文件中。它的主要逻辑是根据不同的像素格式（例如V4L2_PIX_FMT_NV16, V4L2_PIX_FMT_NV21, V4L2_PIX_FMT_NV12等）将视频数据从缓冲区拷贝到指定的文件中
/// @param buffer shared_ptr<VideoBuffer> buffer：这是一个VideoBuffer对象的共享指针，表示要导出的视频缓冲区
/// @param fp     FILE* fp：这是一个标准C的文件指针，用于指向打开的文件，函数将视频数据写入该文件
void dump_videobuffer_to_file(shared_ptr<VideoBuffer> buffer, FILE* fp)
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t h_stride = 0;
    uint32_t v_stride = 0;
    uint32_t fmt = 0;
    unsigned char *base = NULL; // 指向视频缓冲区的起始地址

    // 防御性编程的做法，确保程序不会在无效指针上操作
    if (NULL == fp || NULL == buffer)
        return;

    width = buffer->getImagePara().width;              // 从buffer的getImagePara()方法中获取视频图像的宽度和高度
    height = buffer->getImagePara().height;
    h_stride = buffer->getImagePara().hstride;         // 水平和垂直步幅，这些步幅通常用于图像处理中的对齐和内存布局
    v_stride = buffer->getImagePara().vstride;
    fmt = buffer->getImagePara().v4l2Fmt;              // 从buffer的getImagePara()中获取视频的像素格式
    base = (unsigned char *)(buffer->getActiveData()); // 获取视频数据的起始地址（活动数据指针），并将其转换为unsigned char*类型

    switch (fmt) //根据fmt的值（视频的像素格式），进入不同的分支处理逻辑
    {
        case V4L2_PIX_FMT_NV16:
        { // V4L2_PIX_FMT_NV16表示YUV 4:2:2半平面格式（YUV422SP）。在这种格式中，Y平面和UV平面数据是交织的
            /* YUV422SP -> YUV422P for better display */
            uint32_t i, j;
            unsigned char *base_y = base; // 指向Y平面的数据起始位置（base）
            unsigned char* base_c = base + h_stride * v_stride;
            // 指向UV平面的数据起始位置（base + Y平面的大小，即h_stride * v_stride）
            //用于存储临时的UV平面数据的内存块，大小是h_stride * height * 2字节。这里将YUV422SP转换为YUV422P格式需要两个平面存储U和V
            unsigned char* tmp = (unsigned char*)malloc(h_stride * height * 2);
            // 分别指向临时存储的U平面和V平面数据的位置，tmp_u从tmp的开始位置开始，而tmp_v从U平面数据之后开始
            unsigned char* tmp_u = tmp;
            unsigned char* tmp_v = tmp + width * height / 2;
            // 写入Y平面数据
            // 循环height次，每次将一行Y平面数据（width字节）写入文件fp。base_y指针每次增加h_stride（表示跳到下一行数据的起始地址）
            for (i = 0; i < height; i++, base_y += h_stride)
                fwrite(base_y, 1, width, fp);

            // 这里将UV交织格式数据拆分为独立的U和V平面。循环遍历每一行的UV数据
            // base_c指向UV数据的开始，每次增加h_stride，移动到下一行
            // 内层循环遍历UV数据的每一对（每两个字节为一对，U和V各一个字节）
            for (i = 0; i < height; i++, base_c += h_stride) 
            {
                for (j = 0; j < width / 2; j++) 
                {
                    tmp_u[j] = base_c[2 * j + 0]; // 将当前行的U数据拷贝到tmp_u，V数据拷贝到tmp_v。
                    tmp_v[j] = base_c[2 * j + 1];
                } // tmp_u和tmp_v指针分别移动到下一行位置
                tmp_u += width / 2;
                tmp_v += width / 2;
            }
            // 将转换后的UV数据写入文件fp。因为tmp中存放的是连续的U和V数据，所以写入的大小是width * height字节。
            fwrite(tmp, 1, width * height, fp);
            free(tmp);
        }
        break;

        // 处理V4L2_PIX_FMT_NV21和V4L2_PIX_FMT_NV12格式
        // 这些格式是YUV 4:2:0的半平面格式（分别为NV21和NV12）。数据格式略有不同，但处理方式相似
        case V4L2_PIX_FMT_NV21:
        case V4L2_PIX_FMT_NV12: 
        {
            uint32_t i;
            unsigned char *base_y = base; // 指向Y平面的数据起始位置
            unsigned char *base_c = base + h_stride * v_stride; // 指向UV平面的数据起始位置

            // 与之前类似，循环写入Y平面数据到文件中
            for (i = 0; i < height; i++, base_y += h_stride) 
            {
                fwrite(base_y, 1, width, fp);
            }
            // 写入UV平面数据，注意这里每次跳过h_stride步长，因为UV平面的高度是height / 2
            for (i = 0; i < height / 2; i++, base_c += h_stride) 
            {
                fwrite(base_c, 1, width, fp);
            }
        } 
        break;

        // 处理V4L2_PIX_FMT_YUV420格式,YUV 4:2:0的平面格式，其中Y、U、V数据分别独立存储
        case V4L2_PIX_FMT_YUV420: 
        {
            uint32_t i;
            unsigned char *base_y = base; // base_y指向Y数据
            unsigned char *base_c = base + h_stride * v_stride; // base_c指向U数据

            // 写入Y平面数据
            for (i = 0; i < height; i++, base_y += h_stride) 
            {
                fwrite(base_y, 1, width, fp);
            }
            // 写入U平面数据
            // U平面数据的步长是h_stride / 2，宽度是width / 2
            for (i = 0; i < height / 2; i++, base_c += h_stride / 2) 
            {
                fwrite(base_c, 1, width / 2, fp);
            }
            // 写入V平面数据
            // V平面数据的步长和宽度同样是h_stride / 2和width / 2
            for (i = 0; i < height / 2; i++, base_c += h_stride / 2) 
            {
                fwrite(base_c, 1, width / 2, fp);
            }
        } 
        break;

        // 处理V4L2_PIX_FMT_NV24格式
        // YUV 4:4:4半平面格式，需要将其转换为YUV 4:4:4平面格式
        case V4L2_PIX_FMT_NV24: 
        {
            /* YUV444SP -> YUV444P for better display */
            uint32_t i, j;
            unsigned char* base_y = base;
            unsigned char* base_c = base + h_stride * v_stride;
            unsigned char* tmp = (unsigned char*)malloc(h_stride * height * 2);
            unsigned char* tmp_u = tmp;
            unsigned char* tmp_v = tmp + width * height;
            // 写入Y平面数据
            for (i = 0; i < height; i++, base_y += h_stride)
                fwrite(base_y, 1, width, fp);
            // 处理UV平面数据
            // 拆分UV交织数据为独立的U和V平面，类似之前的逻辑
            for (i = 0; i < height; i++, base_c += h_stride * 2) {
                for (j = 0; j < width; j++) {
                    tmp_u[j] = base_c[2 * j + 0];
                    tmp_v[j] = base_c[2 * j + 1];
                }
                tmp_u += width;
                tmp_v += width;
            }
            // 写入转换后的UV数据并释放内存
            fwrite(tmp, 1, width * height * 2, fp);
            free(tmp);
        } 
        break;

        // 处理V4L2_PIX_FMT_GREY格式
        
        case V4L2_PIX_FMT_GREY: 
        {
            uint32_t i;
            // base_y指向灰度数据的起始位置
            unsigned char *base_y = base; // 灰度格式，没有UV数据
            // 写入灰度数据
            for (i = 0; i < height; i++, base_y += h_stride)
                fwrite(base_y, 1, width, fp);

        } 
        break;

        case V4L2_PIX_FMT_ARGB32:
        case V4L2_PIX_FMT_ABGR32:
        case V4L2_PIX_FMT_RGB32:
        case V4L2_PIX_FMT_BGR32:
        { // 处理各种32位RGB格式
            uint32_t i;
            unsigned char* base_y = base;
            // base_y指向RGB数据的起始位置
            for (i = 0; i < height; i++, base_y += h_stride * 4)
                fwrite(base_y, 1, width * 4, fp); // 每行的步长是h_stride * 4，每个像素占4字节
        }
        break;

        case V4L2_PIX_FMT_RGB565:
        case V4L2_PIX_FMT_RGB555:
        case V4L2_PIX_FMT_RGB444:
        { // 处理各种16位RGB格式
            uint32_t i;
            unsigned char* base_y = base;
            // 写入RGB数据
            // 每行的步长是h_stride * 2，每个像素占2字节
            for (i = 0; i < height; i++, base_y += h_stride * 2)
                fwrite(base_y, 1, width * 2, fp);
        }
        break;

        default: 
        {
            ff_error("not supported format %d\n", fmt);
        } 
        break;
    }
}
