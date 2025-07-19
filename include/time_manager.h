#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <time.h>
#include <sys/time.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include "config.h"

// 时间管理类
class TimeManager {
public:
    TimeManager();
    
    // 初始化时间系统
    bool init();
    
    // 设置编译时间为初始时间
    void setCompileTime();
    
    // 手动设置时间
    bool setTime(int year, int month, int day, int hour, int minute, int second = 0);
    
    // 获取当前时间
    struct tm getCurrentTime();
    
    // 获取Unix时间戳
    time_t getUnixTime();
    
    // 格式化时间字符串
    String formatTime(const char* format = "%Y-%m-%d %H:%M:%S");
    String formatDate(const char* format = "%Y-%m-%d");
    String formatTimeOnly(const char* format = "%H:%M:%S");
    
    // 获取单独的时间分量
    int getYear();
    int getMonth();
    int getDay();
    int getHour();
    int getMinute();
    int getSecond();
    int getWeekday(); // 0=Sunday, 1=Monday, ..., 6=Saturday
    
    // 时间计算
    bool isLeapYear(int year);
    int getDaysInMonth(int year, int month);
    
    // 时间校准（通过NTP服务器）
    bool syncWithNTP(const char* server = "pool.ntp.org");
    
    // 检查时间是否有效
    bool isTimeValid();
    
    // 更新系统设置中的时间
    void updateSystemSettings(SystemSettings* settings);
    
    // 从系统设置中加载时间
    void loadFromSystemSettings(const SystemSettings* settings);
    
    // 获取启动时间
    time_t getBootTime();
    
    // 获取运行时间（秒）
    unsigned long getUptime();
    
    // 时区设置
    void setTimezone(int offsetHours, int offsetMinutes = 0);
    
    // 打印时间信息
    void printTimeInfo();
    
private:
    time_t bootTime;
    int timezoneOffset; // 时区偏移（秒）
    bool ntpSynced;
    
    // 内部辅助函数
    bool isValidDate(int year, int month, int day);
    bool isValidTime(int hour, int minute, int second);
};

// 全局时间管理器实例
extern TimeManager timeManager;

#endif // TIME_MANAGER_H
