@echo off
echo ================================================
echo ESP-NOW 双子星敏捷锥 - 固件上传脚本
echo ================================================
echo.

:menu
echo 请选择上传选项:
echo 1. 上传主设备固件 (Master)
echo 2. 上传从设备固件 (Slave)
echo 3. 上传默认固件 (MAC自动识别)
echo 4. 串口监控
echo 5. 设备信息查看
echo 6. 退出
echo.

set /p choice=请输入选项 (1-6): 

if "%choice%"=="1" goto upload_master
if "%choice%"=="2" goto upload_slave
if "%choice%"=="3" goto upload_default
if "%choice%"=="4" goto monitor
if "%choice%"=="5" goto device_info
if "%choice%"=="6" goto exit

echo 无效选项，请重新选择
goto menu

:upload_master
echo.
echo 正在上传主设备固件...
echo ----------------------------------------
echo 请确保主设备已连接到电脑
echo 检查串口设置 (当前: COM3)
echo.
pio run -e master --target upload
if %errorlevel% == 0 (
    echo ✅ 主设备固件上传成功！
    echo.
    set /p monitor_choice=是否开始串口监控? (y/n): 
    if /i "%monitor_choice%"=="y" (
        echo 开始串口监控...
        pio device monitor -e master
    )
) else (
    echo ❌ 主设备固件上传失败！
    echo 请检查:
    echo - 设备是否正确连接
    echo - 串口号是否正确
    echo - 驱动是否正确安装
)
echo.
pause
goto menu

:upload_slave
echo.
echo 正在上传从设备固件...
echo ----------------------------------------
echo 请确保从设备已连接到电脑
echo 检查串口设置 (当前: COM3)
echo.
pio run -e slave --target upload
if %errorlevel% == 0 (
    echo ✅ 从设备固件上传成功！
    echo.
    set /p monitor_choice=是否开始串口监控? (y/n): 
    if /i "%monitor_choice%"=="y" (
        echo 开始串口监控...
        pio device monitor -e slave
    )
) else (
    echo ❌ 从设备固件上传失败！
    echo 请检查:
    echo - 设备是否正确连接
    echo - 串口号是否正确
    echo - 驱动是否正确安装
)
echo.
pause
goto menu

:upload_default
echo.
echo 正在上传默认固件...
echo ----------------------------------------
echo 请确保设备已连接到电脑
echo 检查串口设置 (当前: COM3)
echo.
pio run -e esp32-c3-devkitm-1 --target upload
if %errorlevel% == 0 (
    echo ✅ 默认固件上传成功！
    echo.
    set /p monitor_choice=是否开始串口监控? (y/n): 
    if /i "%monitor_choice%"=="y" (
        echo 开始串口监控...
        pio device monitor -e esp32-c3-devkitm-1
    )
) else (
    echo ❌ 默认固件上传失败！
    echo 请检查:
    echo - 设备是否正确连接
    echo - 串口号是否正确
    echo - 驱动是否正确安装
)
echo.
pause
goto menu

:monitor
echo.
echo 开始串口监控...
echo ----------------------------------------
echo 按 Ctrl+C 停止监控
echo.
pio device monitor
echo.
pause
goto menu

:device_info
echo.
echo 设备信息查看...
echo ----------------------------------------
echo 可用串口:
pio device list
echo.
echo 如果需要获取设备MAC地址，请先上传固件并查看串口输出
echo.
pause
goto menu

:exit
echo.
echo 感谢使用！
pause
