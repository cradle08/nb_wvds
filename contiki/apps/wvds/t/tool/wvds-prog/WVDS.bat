@echo off

title WVDS烧录

set MCU=MSP430F5438A

:menu
echo.
echo.===============================
echo.  1) VD节点: device-boot.txt
echo.  2) RP节点: router-boot.txt
echo.  3) AP节点: gateway-boot.txt
echo.  4) 监听节点: sniffer.txt
echo.  5) 更新节点: updater.txt
echo.  6) 激活节点: activator.txt
echo.  7) 算法测试: parking.txt
echo.  8) 测试用AP: gateway-uart.txt
echo.  9) 恢复出厂: freset.txt
echo.  q) 退出
echo.===============================
set /p cho=请选择: 

if %cho%==1 goto case1
if %cho%==2 goto case2
if %cho%==3 goto case3
if %cho%==4 goto case4
if %cho%==5 goto case5
if %cho%==6 goto case6
if %cho%==7 goto case7
if %cho%==8 goto case8
if %cho%==9 goto case9
if %cho%==q goto end

echo.错误输入, 请输入1-7或q，再按回车
goto menu

:case1
set FILE=Data\device-boot.txt
goto check

:case2
set FILE=Data\router-boot.txt
goto check

:case3
set FILE=Data\gateway-boot.txt
goto check

:case4
set FILE=Data\sniffer.txt
goto check

:case5
set FILE=Data\updater.txt
goto check

:case6
set FILE=Data\activator.txt
goto check

:case7
set FILE=Data\parking.txt
goto check

:case8
set FILE=Data\gateway-uart.txt
goto check

:case9
set FILE=Data\freset.txt
goto check

:check
if exist %FILE% goto prog
echo.-------------------------------
echo.文件%FILE%不存在
goto menu

:prog
MSP430Flasher -g -n %MCU% -w %FILE% -v -z [RESET,VCC]
goto menu

:end
