#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// 硬件引脚定义 - 仅从机设备需要的硬件
#define VIBRATION_SENSOR_PIN    2     // 震动传感器引脚
#define LED_PIN                 1     // WS2812B LED引脚
#define BUZZER_PIN              4     // 蜂鸣器引脚

// LED配置
#define LED_COUNT               12    // LED数量
#define LED_BRIGHTNESS          50    // LED亮度 (0-255)

// 震动传感器配置
#define VIBRATION_SENSOR_TYPE   0     // 0=常闭开关量传感器，1=数值传感器
#define VIBRATION_DEBOUNCE_MS   200   // 震动防抖时间 (开关量传感器需要更长防抖)

// 计时配置
#define TIMING_READY_DELAY_MS   3000  // 准备时间
#define TIMING_TIMEOUT_MS       30000 // 超时时间

// ESP-NOW配置
#define ESPNOW_CHANNEL          1     // ESP-NOW信道
#define ESPNOW_ENCRYPT          false // 是否加密
#define DEVICE_A_MAC            {0x50, 0x78, 0x7d, 0x46, 0xd4, 0x80}  // 主机设备 COM8
#define DEVICE_B_MAC            {0x50, 0x78, 0x7d, 0x46, 0xcc, 0x90}  // 从机设备 COM3

// 声音配置
#define BEEP_FREQUENCY          2000  // 蜂鸣器频率
#define BEEP_DURATION           100   // 蜂鸣器持续时间

// 从机设备状态
enum SlaveState {
    SLAVE_INIT,           // 初始化
    SLAVE_IDLE,           // 空闲等待
    SLAVE_READY,          // 准备状态
    SLAVE_TRAINING,       // 训练中
    SLAVE_COMPLETE,       // 完成
    SLAVE_ERROR           // 错误
};

// 设备角色
enum DeviceRole {
    ROLE_MASTER,
    ROLE_SLAVE,
    ROLE_UNDEFINED
};

// 通信消息结构
typedef struct {
    uint8_t command;
    uint8_t target_id;
    uint8_t source_id;
    uint32_t timestamp;
    uint32_t data;
    uint8_t checksum;
} message_t;

// 指令类型
enum CommandType {
    CMD_INIT = 0x01,
    CMD_START_TASK = 0x02,
    CMD_TASK_COMPLETE = 0x03,
    CMD_RESET = 0x04,
    CMD_HEARTBEAT = 0x06,
    CMD_HEARTBEAT_ACK = 0x07,
    CMD_PAIRING_REQUEST = 0x10,
    CMD_PAIRING_RESPONSE = 0x11,
    CMD_PAIRING_CONFIRM = 0x12,
    CMD_DEVICE_INFO = 0x13,
    // 震动训练相关命令
    CMD_VT_START_ROUND = 0x20,    // 主机发送：开始单次计时
    CMD_VT_ROUND_COMPLETE = 0x21, // 从机发送：单次完成
    CMD_VT_TRAINING_EXIT = 0x22,  // 退出训练
    CMD_ERROR = 0xFF
};

// 连接状态
enum ConnectionStatus {
    CONN_DISCONNECTED,    // 未连接
    CONN_CONNECTING,      // 连接中
    CONN_CONNECTED,       // 已连接
    CONN_ERROR,           // 连接错误
    CONN_TIMEOUT          // 连接超时
};

// 连接监控配置
#define HEARTBEAT_INTERVAL_MS     3000    // 心跳发送间隔
#define HEARTBEAT_TIMEOUT_MS      8000    // 心跳超时时间
#define CONNECTION_RETRY_COUNT    3       // 连接重试次数
#define CONNECTION_CHECK_INTERVAL 1000    // 连接检查间隔

// 设备配对状态
enum PairingStatus {
    PAIRING_IDLE,           // 空闲状态
    PAIRING_SCANNING,       // 扫描设备中
    PAIRING_FOUND_DEVICE,   // 发现设备
    PAIRING_CONNECTING,     // 连接中
    PAIRING_SUCCESS,        // 配对成功
    PAIRING_FAILED,         // 配对失败
    PAIRING_TIMEOUT         // 配对超时
};

// 配对相关配置
#define PAIRING_SCAN_DURATION_MS    10000   // 扫描时长
#define PAIRING_TIMEOUT_MS          15000   // 配对超时
#define MAX_DISCOVERED_DEVICES      5       // 最大发现设备数量

// 发现的设备信息
typedef struct {
    uint8_t mac[6];           // MAC地址
    int8_t rssi;              // 信号强度
    char name[16];            // 设备名称
    bool isCompatible;        // 是否兼容
    uint32_t lastSeen;        // 最后发现时间
} DiscoveredDevice;

// 颜色定义
#define COLOR_BLACK             0x000000
#define COLOR_WHITE             0xFFFFFF
#define COLOR_RED               0xFF0000
#define COLOR_GREEN             0x00FF00
#define COLOR_BLUE              0x0000FF
#define COLOR_YELLOW            0xFFFF00
#define COLOR_PURPLE            0xFF00FF
#define COLOR_CYAN              0x00FFFF
#define COLOR_ORANGE            0xFF8000

#endif // CONFIG_H