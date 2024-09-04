#include <stdio.h>
#include <string.h>
#include <sys/statvfs.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <fts.h>
#include <sys/time.h>

#include <iostream>
#include <fstream>

#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "system_common.hpp"

/// @brief 获取给定文件路径所在磁盘的可用空间大小
/// @param file_name
/// @return
uint64_t getSystemFreeSize(const char* file_name)
{
    uint64_t size = 0;
    struct statvfs stat;
    char *dirc, *dname;
    dirc = strdup(file_name);
    if (dirc == nullptr)
        return 0;

    dname = dirname(dirc);
    if (statvfs(dname, &stat) == 0) {
        size = stat.f_bsize * stat.f_bavail;
    }

    free(dirc);
    return size;
}

/// @brief  删除给定目录中最旧的文件
/// @param file_path
/// @return
int removeDirectoryOldestFile(const char* file_path)
{
    char *dirc = NULL, *dname;
    bool isFile = true;
    struct stat pstat;

    FTS* fts = NULL;
    FTSENT* ent;
    char oldest_file_path[PATH_MAX];
    time_t oldest_time = INT64_MAX;

    do {
        if (stat(file_path, &pstat) != -1 && S_ISDIR(pstat.st_mode)) {
            isFile = false;
        }

        if (isFile) {
            dirc = strdup(file_path);
            if (dirc == NULL)
                break;
            dname = dirname(dirc);
        } else {
            dname = (char*)file_path;
        }

        char* path[] = {dname, NULL};
        if ((fts = fts_open(path, FTS_PHYSICAL | FTS_NOCHDIR, NULL)) == NULL) {
            fprintf(stderr, "Failed to fts_open %s, error: %s\n", dname, strerror(errno));
            break;
        }

        while ((ent = fts_read(fts)) != NULL) {
            if (ent->fts_info == FTS_F && ent->fts_statp->st_mtim.tv_sec < oldest_time) {
                oldest_time = ent->fts_statp->st_mtim.tv_sec;
                strcpy(oldest_file_path, ent->fts_path);
            }
        }

    } while (0);

    if (fts)
        fts_close(fts);

    if (dirc)
        free(dirc);

    if (oldest_time != INT64_MAX && remove(oldest_file_path) == 0) {
        fprintf(stdout, "remove %s file\n", oldest_file_path);
        return 1;
    }

    return 0;
}

/// @brief 创建一个目录（包括所有需要的父目录）。
/// @param dir
/// @return
static int mkdirP(const char* dir)
{
    char tmp[PATH_MAX];
    char* p = NULL;
    size_t len;
    int ret;

    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/') {
            *p = 0;
            if (access(tmp, F_OK) == -1) {
                ret = mkdir(tmp, 0775);
                if (ret != 0) {
                    fprintf(stderr, "Failed to mkdir %s, error: %s\n", tmp, strerror(errno));
                    return ret;
                }
            }
            *p = '/';
        }

    ret = mkdir(tmp, 0775);
    if (ret != 0)
        fprintf(stderr, "Failed to mkdir %s, error: %s\n", tmp, strerror(errno));
    return ret;
}

/// @brief 创建一个目录及其父目录（如果它们不存在）
/// @param path
/// @return
int createDirectory(const char* path)
{
    char *dirc = NULL, *dname;
    int ret = 0;
    dirc = strdup(path);
    if (dirc == NULL)
        return -1;

    dname = dirname(dirc);
    if (access(dname, F_OK) == -1) {
        ret = mkdirP(dname);
        if (ret == 0) {
            fprintf(stdout, "mkdir %s\n", dname);
        }
    }

    free(dirc);
    return ret;
}

/// @brief 获取指定网络接口的本地IP地址
/// @param const char* interface
/// @return
std::string getLocalIPAddress(const char* interface)
{
    int fd = -1;
    std::string ip_addr;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        fprintf(stderr, "unable to create datagram socket, error: %s\n", strerror(errno));
        return ip_addr;
    }

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

    if (ioctl(fd, SIOCGIFADDR, &ifr) != -1)
        ip_addr = inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr);

    close(fd);
    return ip_addr;
}

/// @brief 向网络广播消息
/// @param port
/// @param message
/// @param
/// @return
int sendBroadcastMessage(int port, const char* message, const char* interface)
{
    int ret;
    int fd;
    struct sockaddr_in broadcast_addr;
    int broadcast_enable = 1;

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        fprintf(stderr, "unable to create datagram socket, error: %s\n", strerror(errno));
        return -1;
    }
    /*
    struct ifreq ifr;
    char ip_addr[32];
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);
    ioctl(fd, SIOCGIFADDR, &ifr);
    strncpy(ip_addr, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), sizeof(ip_addr) - 1);
    */

    if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        fprintf(stderr, "setsockopt(SO_REUSEADDR) error: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");  // 广播地址
    broadcast_addr.sin_port = htons(port);

    ret = sendto(fd, message, strlen(message), 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
    close(fd);
    return ret;
}

/// @brief 
/// @param file_name 
/// @param mtime 
/// @param buf 
/// @param lines 
/// @return 
int readFileLines(const char* file_name, time_t* mtime, std::string& buf, uint32_t lines)
{
    if (file_name == nullptr)
        return -1;

    if (mtime) {
        struct stat fstat;
        if (stat(file_name, &fstat) != -1) {
            if (fstat.st_mtim.tv_sec == *mtime)
                return 0;
            *mtime = fstat.st_mtim.tv_sec;
        }
    }

    std::ifstream file(file_name);
    if (!file.is_open()) {
        fprintf(stderr, "Failed to open file(%s)\n", file_name);
        return -1;
    }

    std::string line;
    do {
        std::getline(file, line);
    } while (lines--);

    buf = line;
    file.close();
    return 0;
}

/// @brief 
/// @param file_name 
/// @param mtime 
/// @return 
std::vector<std::string> readFileLines(const char* file_name, time_t* mtime)
{
    std::vector<std::string> v;
    if (file_name == nullptr)
        return v;

    if (mtime) {
        struct stat fstat;
        if (stat(file_name, &fstat) != -1) {
            if (fstat.st_mtim.tv_sec == *mtime)
                return v;
            *mtime = fstat.st_mtim.tv_sec;
        }
    }

    std::ifstream file(file_name);
    if (!file.is_open()) {
        fprintf(stderr, "Failed to open file(%s)\n", file_name);
        return v;
    }

    std::string line;
    while (std::getline(file, line)) {
        v.push_back(line);
    }

    file.close();
    return v;
}

/// @brief 读取文件的指定行，或者将文件内容存入std::vector<std::string>
/// @param file_name
/// @param mtime
/// @param v
/// @return
int readFileLines(const char* file_name, time_t* mtime, std::vector<std::string>& v)
{
    int ret = -1;
    if (file_name == nullptr)
        return ret;

    if (mtime) {
        struct stat fstat;
        ret = stat(file_name, &fstat);
        if (ret < 0)
            return ret;

        if (fstat.st_mtim.tv_sec == *mtime)
            return 0;
        *mtime = fstat.st_mtim.tv_sec;
    }

    std::ifstream file(file_name);
    if (!file.is_open()) {
        fprintf(stderr, "Failed to open file(%s)\n", file_name);
        return -1;
    }

    std::string line;
    int count = 0;
    while (std::getline(file, line)) {
        v.push_back(line);
        count++;
    }

    file.close();
    return count;
}

/// @brief 将时间戳转换为格式化的时间字符串
/// @param pts 
/// @param format 
/// @return
std::string ptsToTimeStr(int64_t pts, const std::string& format)
{
    const int buf_len = 512;
    char time_str[buf_len];
    char time_mstr[buf_len];
    int msec, ret;

    if (pts) {
        msec = pts % 1000;
        long int sec = pts / 1000000;
        ret = strftime(time_str, buf_len, format.c_str(), localtime(&sec));
    } else {
        struct timeval current_time;
        gettimeofday(&current_time, NULL);
        msec = current_time.tv_usec / 1000;
        ret = strftime(time_str, buf_len, format.c_str(), localtime(&current_time.tv_sec));
    }

    ret = snprintf(time_mstr, buf_len, "%s_%d", (ret == 0) ? format.c_str() : time_str, msec);
    if (ret >= buf_len)
        time_mstr[buf_len - 1] = 0;
    return time_mstr;
}

/// @brief 读取文件的全部内容并返回 
/// @param url 
/// @return
char* readFile(const char* url)
{
    char* fileContext = NULL;
    FILE* fp = fopen(url, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        int len = ftell(fp);
        if (len > 0) {
            rewind(fp);
            fileContext = new char[len + 1];
            if (fread(fileContext, 1, len, fp) <= 0) {
                delete[] fileContext;
                fileContext = NULL;
            } else
                fileContext[len] = 0;
        }
        fclose(fp);
    } else {
        printf("Failed to open %s \n", url);
    }
    return fileContext;
}


/// @brief 获取系统启动以来的时间(单位：秒)
/// @return 
uint32_t getUptime()
{
    uint32_t uptime;
    FILE* fp = fopen("/proc/uptime", "r");
    if (fp == NULL) {
        perror("Failed to opening /proc/uptime");
        return 0;
    }

    if (fscanf(fp, "%u", &uptime) != 1) {
        perror("Failed to reading uptime from /proc/uptime");
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return uptime;
}