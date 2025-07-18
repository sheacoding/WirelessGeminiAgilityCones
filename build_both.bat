@echo off
echo ================================================
echo ESP-NOW 双子星敏捷锥 - 双设备构建脚本
echo ================================================
echo.

:menu
echo 请选择构建选项:
echo 1. 构建主设备 (Master)
echo 2. 构建从设备 (Slave)
echo 3. 构建两个设备
echo 4. 构建默认版本 (根据MAC地址自动识别)
echo 5. 清理构建
echo 6. 退出
echo.

set /p choice=请输入选项 (1-6): 

if "%choice%"=="1" goto build_master
if "%choice%"=="2" goto build_slave
if "%choice%"=="3" goto build_both
if "%choice%"=="4" goto build_default
if "%choice%"=="5" goto clean
if "%choice%"=="6" goto exit

echo 无效选项，请重新选择
goto menu

:build_master
echo.
echo 正在构建主设备 (Master)...
echo ----------------------------------------
pio run -e master
if %errorlevel% == 0 (
    echo ✅ 主设备构建成功！
    echo 固件位置: .pio\build\master\firmware.bin
) else (
    echo ❌ 主设备构建失败！
)
echo.
pause
goto menu

:build_slave
echo.
echo 正在构建从设备 (Slave)...
echo ----------------------------------------
pio run -e slave
if %errorlevel% == 0 (
    echo ✅ 从设备构建成功！
    echo 固件位置: .pio\build\slave\firmware.bin
) else (
    echo ❌ 从设备构建失败！
)
echo.
pause
goto menu

:build_both
echo.
echo 正在构建两个设备...
echo ----------------------------------------
echo 构建主设备...
pio run -e master
if %errorlevel% == 0 (
    echo ✅ 主设备构建成功！
    echo 构建从设备...
    pio run -e slave
    if %errorlevel% == 0 (
        echo ✅ 从设备构建成功！
        echo.
        echo 两个设备都构建成功！
        echo 主设备固件: .pio\build\master\firmware.bin
        echo 从设备固件: .pio\build\slave\firmware.bin
    ) else (
        echo ❌ 从设备构建失败！
    )
) else (
    echo ❌ 主设备构建失败！
)
echo.
pause
goto menu

:build_default
echo.
echo 正在构建默认版本 (MAC自动识别)...
echo ----------------------------------------
pio run -e esp32-c3-devkitm-1
if %errorlevel% == 0 (
    echo ✅ 默认版本构建成功！
    echo 固件位置: .pio\build\esp32-c3-devkitm-1\firmware.bin
) else (
    echo ❌ 默认版本构建失败！
)
echo.
pause
goto menu

:clean
echo.
echo 正在清理构建目录...
echo ----------------------------------------
pio run --target clean
echo ✅ 构建目录已清理
echo.
pause
goto menu

:exit
echo.
echo 感谢使用！
pause
