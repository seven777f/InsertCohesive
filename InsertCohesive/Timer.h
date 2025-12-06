#pragma once
#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

// 计时器类
class Timer {
public:
    // 构造函数：初始化计时器，设置名称
    explicit Timer(const std::string& name = "Timer") : name_(name), running_(false) {}

    // 开始计时
    void start() {
        if (!running_) {
            start_time_ = std::chrono::high_resolution_clock::now();
            running_ = true;
        }
    }

    // 停止计时，记录一次时间间隔
    void stop() {
        if (running_) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time_).count();
            durations_.push_back(duration);
            running_ = false;
        }
    }

    // 获取总时间（单位：毫秒）
    double getTotalTimeMs() const {
        double total = 0.0;
        for (auto duration : durations_) {
            total += duration / 1'000'000.0; // 纳秒转毫秒
        }
        return total;
    }

    // 获取总时间（单位：秒）
    double getTotalTimeSec() const {
        return getTotalTimeMs() / 1000.0;
    }

    // 获取最后一次时间间隔（单位：毫秒）
    double getLastTimeMs() const {
        if (durations_.empty()) return 0.0;
        return durations_.back() / 1'000'000.0;
    }
    // 获取最后一次时间间隔（单位：秒）
    void getLastTime(const std::string& str) const {
        if (durations_.empty()) return ;
        std::cout <<str << durations_.back() / 1'000'000000.0 <<"s" << std::endl;
        
    }
    // 打印计时结果（默认毫秒，可选输出到文件）
    void print(std::ostream& os = std::cout, bool in_seconds = true) const {
        if (durations_.empty()) {
            os << name_ << ": No timings recorded." << std::endl;
            return;
        }
        double total = in_seconds ? getTotalTimeSec() : getTotalTimeMs();
        std::string unit = in_seconds ? "seconds" : "milliseconds";
        os << name_ << ": Total time = " << total << " " << unit
            << ", Runs = " << durations_.size() << std::endl;
        if (durations_.size() > 1) {
            for (size_t i = 0; i < durations_.size(); ++i) {
                double time = durations_[i] / (in_seconds ? 1'000'000'000.0 : 1'000'000.0);
                os << "  Run " << i + 1 << ": " << time << " " << unit << std::endl;
            }
        }
    }

    // 重置计时器
    void reset() {
        durations_.clear();
        running_ = false;
    }

private:
    std::string name_; // 计时器名称
    bool running_; // 是否正在计时
    std::chrono::high_resolution_clock::time_point start_time_; // 开始时间点
    std::vector<long long> durations_; // 存储每次计时（纳秒）
};