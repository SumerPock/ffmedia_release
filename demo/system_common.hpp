#pragma once
#include <stdint.h>
#include <string>
#include <unistd.h>
#include <vector>

uint64_t getSystemFreeSize(const char* file_name);
int removeDirectoryOldestFile(const char* path);
int createDirectory(const char* path);

int readFileLines(const char* file_name, time_t* mtime, std::string& buf, uint32_t lines);
std::vector<std::string> readFileLines(const char* file_name, time_t* mtime);
int readFileLines(const char* file_name, time_t* mtime, std::vector<std::string>& v);

int sendBroadcastMessage(int port, const char* message, const char* interface);
std::string getLocalIPAddress(const char* interface);

std::string ptsToTimeStr(int64_t pts, const std::string& format);
char* readFile(const char* url);

uint32_t getUptime();