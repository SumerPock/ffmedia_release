// dem_comm.hpp
#ifndef DEM_COMM_HPP
#define DEM_COMM_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>

// 定义消息队列结构体
struct MessageQueue
{
    std::queue<std::string> queue;
    //std::mutex mtx;
    mutable std::mutex mtx; // 使用 mutable 关键字允许在 const 成员函数中修改
    std::condition_variable cv;

    void push(const std::string &data)
    {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(data);
        cv.notify_one();
    }

    std::string pop()
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]
                { return !queue.empty(); });
        std::string data = queue.front();
        queue.pop();
        return data;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mtx); // 移除 const 限定符
        return queue.empty();
    }
};

// 声明全局消息队列
extern MessageQueue messageQueue;

#endif // DEM_COMM_HPP
