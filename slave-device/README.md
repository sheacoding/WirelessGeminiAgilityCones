# 从机设备程序

这是敏捷锥训练系统的从机设备程序，包含最精简的硬件组件和功能。

## 硬件组件

- **ESP32-C3** 微控制器
- **震动传感器** (GPIO2) - 检测用户完成训练
- **WS2812B LED灯带** (GPIO1) - 状态指示和视觉反馈
- **蜂鸣器** (GPIO4) - 音频提示

## 功能特性

### 核心功能
- ESP-NOW无线通信
- 震动传感器检测
- LED状态指示
- 音频提示反馈

### 状态管理
- `SLAVE_INIT` - 初始化状态
- `SLAVE_IDLE` - 空闲等待主设备命令
- `SLAVE_READY` - 准备训练状态
- `SLAVE_TRAINING` - 训练进行中
- `SLAVE_COMPLETE` - 训练完成
- `SLAVE_ERROR` - 错误状态

### 连接监控
- 自动心跳包通信
- 连接状态LED指示
- 连接超时重试机制

## 编译和上传

```bash
# 编译从机程序
pio run -e slave-device

# 上传到设备
pio run -e slave-device --target upload

# 监控串口输出
pio device monitor --baud 115200
```

## LED状态指示

| 颜色 | 状态 |
|------|------|
| 蓝色 | 空闲等待 |
| 绿色呼吸 | 准备就绪 |
| 紫色 | 训练中 |
| 青色 | 训练完成 |
| 红色闪烁 | 错误状态 |
| 黄色呼吸 | 连接中 |
| 绿色 | 已连接 |
| 橙色 | 连接超时 |
| 红色 | 连接错误 |

## 音频提示

- **启动音** - 1000Hz + 1200Hz
- **连接成功音** - 800Hz + 1000Hz + 1200Hz  
- **训练开始音** - 1000Hz + 1200Hz
- **训练完成音** - 1500Hz + 1800Hz + 2000Hz
- **震动检测音** - 1500Hz短音
- **错误音** - 500Hz + 400Hz长音

## 通信协议

从机设备支持以下ESP-NOW命令：
- `CMD_START_TASK` - 开始训练
- `CMD_RESET` - 重置状态
- `CMD_HEARTBEAT` - 心跳包
- `CMD_PAIRING_REQUEST` - 配对请求

发送的命令：
- `CMD_TASK_COMPLETE` - 训练完成（带用时数据）
- `CMD_HEARTBEAT_ACK` - 心跳应答
- `CMD_DEVICE_INFO` - 设备信息回应

## 配置参数

主要配置参数在 `include/config.h` 中：
- `VIBRATION_THRESHOLD` - 震动检测阈值
- `TIMING_TIMEOUT_MS` - 训练超时时间
- `HEARTBEAT_INTERVAL_MS` - 心跳间隔
- `LED_COUNT` - LED数量
- `LED_BRIGHTNESS` - LED亮度

## 硬件连接

```
ESP32-C3 引脚连接：
- GPIO2: 震动传感器数据引脚
- GPIO1: WS2812B LED数据引脚  
- GPIO4: 蜂鸣器控制引脚
- 3.3V: 电源正极
- GND: 电源负极
```