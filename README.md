# ESP-NOW 无线双子星敏捷锥

## 项目简介

这是一个基于ESP32-C3的智能训练计时系统，集成了震动传感器、按钮菜单、OLED显示屏、LED指示灯和蜂鸣器。系统支持单次计时、震动训练和双设备协作训练三种模式。

## 核心特性

### 🎯 多种训练模式
- **单次计时**: 简单的震动触发计时器
- **震动训练**: 带有时间提醒和视觉反馈的训练模式  
- **双设备训练**: 通过ESP-NOW协议实现设备间通信

### 🔧 硬件功能
- **震动传感器**: 灵敏度可调，50ms防抖处理
- **按钮菜单**: 支持短按/长按，直观的菜单导航
- **OLED显示**: 128×64像素，实时显示状态和计时
- **LED指示**: 12个WS2812B RGB LED，多彩状态指示
- **蜂鸣器**: 音频提醒，不同状态对应不同提示音

### 📡 无线通信
- **ESP-NOW协议**: 超低延迟(<10ms)，无需路由器
- **双向通信**: 主从设备角色自动识别
- **通信距离**: 50-100米(开阔环境)
- **高可靠性**: 传输成功率>99%

## 硬件配置

### 主控芯片
- **ESP32-C3-MINI**: 低功耗，集成WiFi和蓝牙

### 传感器和输入
- **震动传感器**: GPIO2，模拟输入
- **按钮**: GPIO0 (BOOT按钮)

### 输出设备
- **LED灯环**: GPIO1，WS2812B × 12
- **蜂鸣器**: GPIO4，无源蜂鸣器
- **OLED屏**: I2C接口 (SDA: GPIO8, SCL: GPIO9)

### 电源系统
- **电池**: 3.7V 1000mAh锂电池
- **充电**: TP4056充电管理模块
- **续航**: >8小时连续使用

## 软件架构

### 分层设计
```
┌─────────────────────────────────────────┐
│         应用层 (Application)             │
├─────────────────────────────────────────┤
│         业务层 (Business Logic)          │
├─────────────────────────────────────────┤
│         通信层 (Communication)           │
├─────────────────────────────────────────┤
│         硬件抽象层 (HAL)                 │
├─────────────────────────────────────────┤
│         ESP32-C3 硬件层                  │
└─────────────────────────────────────────┘
```

### 核心模块
- **HardwareManager**: 硬件控制和抽象
- **MenuManager**: 菜单系统和用户界面
- **VibrationTrainingManager**: 震动训练逻辑
- **ESP-NOW Communication**: 设备间通信

## 快速开始

### 环境要求
- [PlatformIO](https://platformio.org/)
- [Visual Studio Code](https://code.visualstudio.com/)
- ESP32-C3开发板

### 编译和上传

#### 方法一：使用便捷脚本（推荐）

1. **构建固件**
   ```bash
   # 运行构建脚本
   build_both.bat
   
   # 选择构建选项：
   # 1. 构建主设备 (Master)
   # 2. 构建从设备 (Slave) 
   # 3. 构建两个设备
   # 4. 构建默认版本 (MAC自动识别)
   ```

2. **上传固件**
   ```bash
   # 运行上传脚本
   upload.bat
   
   # 根据提示选择上传主设备或从设备固件
   ```

#### 方法二：使用PlatformIO命令

1. **构建主设备固件**
   ```bash
   pio run -e master
   ```

2. **构建从设备固件**
   ```bash
   pio run -e slave
   ```

3. **上传固件**
   ```bash
   # 上传主设备固件
   pio run -e master --target upload
   
   # 上传从设备固件
   pio run -e slave --target upload
   ```

4. **串口监控**
   ```bash
   pio device monitor
   ```

#### 方法三：传统方式（MAC地址识别）

```bash
# 编译默认固件
pio run -e esp32-c3-devkitm-1

# 上传到两个设备
pio run -e esp32-c3-devkitm-1 --target upload
```

### 配置说明
主要配置参数在 `include/config.h` 文件中：

```cpp
// 硬件引脚配置
#define VIBRATION_SENSOR_PIN    2
#define BUTTON_PIN              0
#define LED_PIN                 1
#define BUZZER_PIN              4
#define OLED_SDA_PIN            8
#define OLED_SCL_PIN            9

// 震动传感器配置
#define VIBRATION_THRESHOLD     100
#define VIBRATION_DEBOUNCE_MS   50

// 设备MAC地址
#define DEVICE_A_MAC            {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01}
#define DEVICE_B_MAC            {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x02}
```

## 使用方法

### 基本操作
1. **开机**: 按电源开关
2. **菜单导航**: 短按按钮选择菜单项
3. **确认选择**: 长按按钮(1秒)确认
4. **开始训练**: 在准备状态下按按钮
5. **返回菜单**: 长按按钮返回主菜单

### 训练流程
1. 开机进入菜单
2. 选择训练模式
3. 系统准备(3秒倒计时)
4. 开始训练计时
5. 震动触发停止计时
6. 显示训练结果

### LED指示说明
- **蓝色**: 菜单模式
- **绿色**: 准备就绪
- **红色**: 错误状态
- **渐变**: 训练进度

### 音频提示
- **1000Hz**: 开始提示
- **2000Hz**: 完成提示
- **400Hz**: 错误提示
- **1500Hz**: 定时提醒

## 双设备协作

### 设备角色
- **主设备(Master)**: 发送训练开始信号
- **从设备(Slave)**: 接收信号并响应

### 通信协议
```cpp
typedef struct {
    uint8_t command;      // 指令类型
    uint8_t target_id;    // 目标设备ID
    uint8_t source_id;    // 源设备ID
    uint32_t timestamp;   // 时间戳
    uint32_t data;        // 数据载荷
    uint8_t checksum;     // 校验和
} message_t;
```

### 指令类型
- `CMD_INIT`: 初始化
- `CMD_START_TASK`: 开始任务
- `CMD_TASK_COMPLETE`: 任务完成
- `CMD_RESET`: 重置
- `CMD_ROLE_SWITCH`: 角色切换
- `CMD_HEARTBEAT`: 心跳

## 性能指标

### 响应性能
- 震动检测响应: <1ms
- 按钮响应: <50ms
- LED显示延迟: <5ms
- 通信延迟: <10ms

### 功耗性能
- 工作电流: 50-200mA
- 待机电流: <10mA
- 续航时间: >8小时

## 故障排除

### 常见问题
1. **震动传感器不响应**: 检查连接和阈值设置
2. **OLED显示异常**: 检查I2C连接
3. **LED不亮**: 检查电源和数据线
4. **ESP-NOW通信失败**: 检查MAC地址配置

### 调试方法
- 使用串口监视器查看日志
- 检查硬件连接
- 确认配置参数

## 扩展功能

### 可添加功能
- 数据存储和历史记录
- 网络同步和云端存储
- 多设备组网训练
- 个性化用户设置
- OTA无线升级

### 硬件扩展
- 预留GPIO接口
- I2C/SPI总线扩展
- 模拟输入扩展
- 外部传感器接口

## 开发文档

详细的开发文档和API参考：
- [架构设计文档](docs/架构设计文档.md)
- [使用说明](docs/使用说明.md)
- [OLED UI设计](docs/OLED_UI_Design.md)

## 许可证

本项目采用MIT许可证 - 详见 [LICENSE](LICENSE) 文件

## 贡献指南

欢迎贡献代码和提出改进建议！

1. Fork 本项目
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 创建 Pull Request

## 联系方式

- 项目主页: https://github.com/sheacoding/WirelessGeminiAgilityCones
- 问题反馈: https://github.com/sheacoding/WirelessGeminiAgilityCones/issues
- 邮箱: sheacoding@gmail.com

---

**版本**: v1.0.0  
**更新日期**: 2025-07-16  
**作者**: eric 
**许可证**: MIT
