#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// 硬件引脚定义
#define VIBRATION_SENSOR_PIN    2     // 震动传感器引脚
#define BUTTON_PIN              5     // 按钮引脚 (GPIO5，高电平触发)
#define LED_PIN                 1     // WS2812B LED引脚
#define BUZZER_PIN              4     // 蜂鸣器引脚
#define OLED_SDA_PIN            8     // OLED SDA引脚
#define OLED_SCL_PIN            9     // OLED SCL引脚

// LED配置
#define LED_COUNT               12    // LED数量
#define LED_BRIGHTNESS          50    // LED亮度 (0-255)

// OLED配置
#define OLED_WIDTH              128   // OLED宽度
#define OLED_HEIGHT             64    // OLED高度
#define OLED_RESET_PIN          -1    // OLED复位引脚

// 震动传感器配置
#define VIBRATION_THRESHOLD     100   // 震动阈值
#define VIBRATION_DEBOUNCE_MS   50    // 震动防抖时间

// 按钮配置
#define BUTTON_DEBOUNCE_MS      50    // 按钮防抖时间
#define BUTTON_CLICK_MS         400   // 点击间隔时间（双击检测窗口）
#define BUTTON_DOUBLE_CLICK_MS  300   // 双击时间间隔 (ms)
#define BUTTON_LONG_PRESS_MS    1000  // 长按时间阈值

// 计时配置
#define TIMING_READY_DELAY_MS   3000  // 准备时间
#define TIMING_TIMEOUT_MS       30000 // 超时时间
#define TIMING_ALERT_INTERVAL   5000  // 提醒间隔

// ESP-NOW配置
#define ESPNOW_CHANNEL          1     // ESP-NOW信道
#define ESPNOW_ENCRYPT          false // 是否加密
#define DEVICE_A_MAC            {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01}
#define DEVICE_B_MAC            {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x02}

// 声音配置
#define BEEP_FREQUENCY          2000  // 蜂鸣器频率
#define BEEP_DURATION           100   // 蜂鸣器持续时间

// 系统状态
enum SystemState {
    STATE_INIT,           // 初始化
    STATE_MENU,           // 菜单模式
    STATE_READY,          // 准备状态
    STATE_TRAINING,       // 训练中
    STATE_TIMING,         // 计时中
    STATE_COMPLETE,       // 完成
    STATE_ERROR           // 错误
};

// 训练模式
enum TrainingMode {
    MODE_SINGLE_TIMER,    // 单次计时
    MODE_VIBRATION_TRAINING, // 震动训练
    MODE_DUAL_TRAINING,   // 双设备训练
    MODE_SETTINGS         // 设置模式
};

// 菜单项
enum MenuItems {
    MENU_START_TRAINING,
    MENU_HISTORY_DATA,
    MENU_SYSTEM_SETTINGS,
    MENU_ITEM_COUNT
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
    CMD_ROLE_SWITCH = 0x05,
    CMD_HEARTBEAT = 0x06,
    CMD_ERROR = 0xFF
};

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

// 系统设置项
enum SettingsItems {
    SETTING_SOUND_TOGGLE,
    SETTING_LED_COLOR,
    SETTING_LED_BRIGHTNESS,
    SETTING_DATE_TIME,
    SETTING_ALERT_DURATION,
    SETTING_BACK,
    SETTING_ITEM_COUNT
};

// LED颜色选项
enum LedColorOption {
    LED_COLOR_RED,
    LED_COLOR_GREEN,
    LED_COLOR_BLUE,
    LED_COLOR_YELLOW,
    LED_COLOR_PURPLE,
    LED_COLOR_CYAN,
    LED_COLOR_COUNT
};

// 系统设置结构
typedef struct {
    bool soundEnabled;
    LedColorOption ledColor;
    uint8_t ledBrightness;  // 0-100
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint16_t alertDuration; // 达标提醒时长，分钟
} SystemSettings;

// 默认设置
#define DEFAULT_SOUND_ENABLED   true
#define DEFAULT_LED_COLOR       LED_COLOR_GREEN
#define DEFAULT_LED_BRIGHTNESS  60
#define DEFAULT_ALERT_DURATION  10

#endif // CONFIG_H
