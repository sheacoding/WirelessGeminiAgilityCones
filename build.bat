@echo off
echo 正在编译 ESP-NOW 震动训练系统...
echo.

REM 清理之前的构建
echo 清理构建目录...
pio run --target clean

REM 编译项目
echo 开始编译...
pio run

REM 检查编译结果
if %errorlevel% == 0 (
    echo.
    echo ✅ 编译成功！
    echo.
    echo 固件位置: .pio\build\esp32-c3-devkitm-1\firmware.bin
    echo.
    echo 要上传固件，请运行: pio run --target upload
    echo 要监控串口，请运行: pio device monitor
) else (
    echo.
    echo ❌ 编译失败！
    echo 请检查错误信息并修复代码。
)

pause
