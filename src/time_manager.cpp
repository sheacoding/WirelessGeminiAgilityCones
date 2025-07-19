#include "time_manager.h"

TimeManager::TimeManager() : bootTime(0), timezoneOffset(0), ntpSynced(false) {}

bool TimeManager::init() {
    setCompileTime();
    return syncWithNTP();
}

void TimeManager::setCompileTime() {
    // 解析编译日期和时间
    struct tm compileTime = {};
    
    // 解析日期 __DATE__ 格式: "Jan 15 2024"
    const char* dateStr = __DATE__;
    const char* timeStr = __TIME__;
    
    // 月份转换
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    char month[4];
    int day, year, hour, minute, second;
    
    // 解析日期
    sscanf(dateStr, "%s %d %d", month, &day, &year);
    
    // 查找月份
    for (int i = 0; i < 12; i++) {
        if (strcmp(month, months[i]) == 0) {
            compileTime.tm_mon = i;
            break;
        }
    }
    
    compileTime.tm_mday = day;
    compileTime.tm_year = year - 1900;
    
    // 解析时间 __TIME__ 格式: "12:34:56"
    sscanf(timeStr, "%d:%d:%d", &hour, &minute, &second);
    compileTime.tm_hour = hour;
    compileTime.tm_min = minute;
    compileTime.tm_sec = second;
    
    time_t t = mktime(&compileTime);
    timeval now = { .tv_sec = t };
    settimeofday(&now, nullptr);
    bootTime = time(nullptr);
    
    Serial.printf("Set compile time: %04d-%02d-%02d %02d:%02d:%02d\n", 
                  year, compileTime.tm_mon + 1, day, hour, minute, second);
}

bool TimeManager::setTime(int year, int month, int day, int hour, int minute, int second) {
    struct tm t = {};
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = second;
    time_t newTime = mktime(&t);
    if (newTime == -1) return false;
    timeval now = { .tv_sec = newTime };
    settimeofday(&now, nullptr);
    return true;
}

struct tm TimeManager::getCurrentTime() {
    time_t t = time(nullptr);
    return *localtime(&t);
}

time_t TimeManager::getUnixTime() {
    return time(nullptr);
}

String TimeManager::formatTime(const char* format) {
    char buffer[80];
    struct tm timeinfo = getCurrentTime();
    strftime(buffer, sizeof(buffer), format, &timeinfo);
    return String(buffer);
}

String TimeManager::formatDate(const char* format) {
    return formatTime(format);
}

String TimeManager::formatTimeOnly(const char* format) {
    return formatTime(format);
}

int TimeManager::getYear() {
    return getCurrentTime().tm_year + 1900;
}

int TimeManager::getMonth() {
    return getCurrentTime().tm_mon + 1;
}

int TimeManager::getDay() {
    return getCurrentTime().tm_mday;
}

int TimeManager::getHour() {
    return getCurrentTime().tm_hour;
}

int TimeManager::getMinute() {
    return getCurrentTime().tm_min;
}

int TimeManager::getSecond() {
    return getCurrentTime().tm_sec;
}

int TimeManager::getWeekday() {
    return getCurrentTime().tm_wday;
}

bool TimeManager::isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int TimeManager::getDaysInMonth(int year, int month) {
    static const int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month < 1 || month > 12) return 0;
    if (month == 2 && isLeapYear(year)) return 29;
    return daysInMonth[month - 1];
}

bool TimeManager::syncWithNTP(const char* server) {
    Serial.printf("Connecting to NTP server: %s...\n", server);
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, server);
    esp_sntp_init();
    time_t now = time(nullptr);
    for (int i = 0; i < 10 && now < 24 * 3600; ++i) {
        delay(500);
        now = time(nullptr);
    }
    ntpSynced = (now >= 24 * 3600);
    if (ntpSynced) {
        Serial.println("NTP time sync successful.");
        bootTime = now;
    } else {
        Serial.println("NTP time sync failed.");
    }
    return ntpSynced;
}

bool TimeManager::isTimeValid() {
    return getYear() > 2020;
}

void TimeManager::updateSystemSettings(SystemSettings* settings) {
    struct tm currentTime = getCurrentTime();
    settings->year = currentTime.tm_year + 1900;
    settings->month = currentTime.tm_mon + 1;
    settings->day = currentTime.tm_mday;
    settings->hour = currentTime.tm_hour;
    settings->minute = currentTime.tm_min;
}

void TimeManager::loadFromSystemSettings(const SystemSettings* settings) {
    setTime(settings->year, settings->month, settings->day, settings->hour, settings->minute);
}

time_t TimeManager::getBootTime() {
    return bootTime;
}

unsigned long TimeManager::getUptime() {
    return millis() / 1000;
}

void TimeManager::setTimezone(int offsetHours, int offsetMinutes) {
    timezoneOffset = (offsetHours * 3600) + (offsetMinutes * 60);
    char tz[16];
    sprintf(tz, "GMT%c%d", (offsetHours >= 0) ? '+' : '-', abs(offsetHours));
    setenv("TZ", tz, 1);
    tzset();
    printTimeInfo();
}

bool TimeManager::isValidDate(int year, int month, int day) {
    if (year < 1970 || month < 1 || month > 12 || day < 1) return false;
    return day <= getDaysInMonth(year, month);
}

bool TimeManager::isValidTime(int hour, int minute, int second) {
    return hour >= 0 && hour < 24 && minute >= 0 && minute < 60 && second >= 0 && second < 60;
}

void TimeManager::printTimeInfo() {
    Serial.printf("Timezone set to: %d seconds offset\n", timezoneOffset);
}

TimeManager timeManager;
