@echo off

title WVDS编译

set PATH="C:\Program Files (x86)\IAR Systems\Embedded Workbench 7.2\common\bin";"C:\Program Files\IAR Systems\Embedded Workbench 7.2\common\bin";C:\cygwin\bin;%PATH%

:menu
echo.=============================
echo.  1) VD程序: device
echo.  2) RP程序: router
echo.  3) AP程序: gateway
echo.  4) 监听程序: sniffer
echo.  5) 更新程序: updater
echo.  6) 激活程序: activator
echo.  7) 算法测试: parking
echo.  b) 引导程序: boot
echo.  p) 产品程序: VD/RP/AP
echo.  a) 所有程序
echo.  q) 退出
echo.=============================
set /p cho=请选择程序: 

if %cho%==1 goto device
if %cho%==2 goto err
if %cho%==3 goto err
if %cho%==4 goto sniffer
if %cho%==5 goto updater
if %cho%==6 goto activator
if %cho%==7 goto parking
if %cho%==b goto boot
if %cho%==p goto prod
if %cho%==a goto all
if %cho%==q goto end

:device
echo.-----------------------------
echo.* 正在为串行泊位编译VD程序...
sed -i '/^#define SERIAL_SPACE/s/^\(#define\s\+SERIAL_SPACE\s\+\)[01]\(.*\)/\11\2/' device/VehicleDetection.h
sed -i '/^#define PARALLEL_SPACE/s/^\(#define\s\+PARALLEL_SPACE\s\+\)[01]\(.*\)/\10\2/' device/VehicleDetection.h
IarBuild device\EW430\device.ewp -build Release -log warnings
echo.-----------------------------
echo.* 正在为并行泊位编译VD程序...
sed -i '/^#define SERIAL_SPACE/s/^\(#define\s\+SERIAL_SPACE\s\+\)[01]\(.*\)/\10\2/' device/VehicleDetection.h
sed -i '/^#define PARALLEL_SPACE/s/^\(#define\s\+PARALLEL_SPACE\s\+\)[01]\(.*\)/\11\2/' device/VehicleDetection.h
IarBuild device\EW430\device.ewp -build Release -log warnings
goto menu

:router
IarBuild router\EW430\router.ewp -build Release -log warnings
goto menu

:gateway
IarBuild gateway\EW430\gateway.ewp -build Release -log warnings
goto menu

:sniffer
IarBuild ..\sniffer\EW430\sniffer.ewp -build Release -log warnings
goto menu

:updater
IarBuild updater\EW430\updater.ewp -build Release -log warnings
goto menu

:activator
IarBuild activator\EW430\activator.ewp -build Release -log warnings
goto menu

:parking
IarBuild parking\EW430\parking.ewp -build Release -log warnings
goto menu

:boot
echo.-----------------------------
echo.* 正在为VD编译引导程序...
sed -i 's/^#define BOARD_CADRE1120_\(..\)\s\+[01]/#define BOARD_CADRE1120_\1  0/' boot/main.c
sed -i 's/^#define BOARD_CADRE1120_VD\s\+[01]/#define BOARD_CADRE1120_VD  1/' boot/main.c
IarBuild boot\EW430\boot.ewp -build Release -log warnings
rem echo.-----------------------------
rem echo.* 正在为RP编译引导程序...
rem sed -i 's/^#define BOARD_CADRE1120_\(..\)\s\+[01]/#define BOARD_CADRE1120_\1  0/' boot/main.c
rem sed -i 's/^#define BOARD_CADRE1120_RP\s\+[01]/#define BOARD_CADRE1120_RP  1/' boot/main.c
rem IarBuild boot\EW430\boot.ewp -build Release -log warnings
rem echo.-----------------------------
rem echo.* 正在为AP编译引导程序...
rem sed -i 's/^#define BOARD_CADRE1120_\(..\)\s\+[01]/#define BOARD_CADRE1120_\1  0/' boot/main.c
rem sed -i 's/^#define BOARD_CADRE1120_AP\s\+[01]/#define BOARD_CADRE1120_AP  1/' boot/main.c
rem IarBuild boot\EW430\boot.ewp -build Release -log warnings
rem echo.-----------------------------
rem sed -i 's/^#define BOARD_CADRE1120_\(..\)\s\+[01]/#define BOARD_CADRE1120_\1  0/' boot/main.c
rem sed -i 's/^#define BOARD_CADRE1120_VD\s\+[01]/#define BOARD_CADRE1120_VD  1/' boot/main.c
sed -i 's/$/\r/' boot/main.c
goto menu

:prod
echo.-----------------------------
echo.* 正在为串行泊位编译VD程序...
sed -i '/^#define SERIAL_SPACE/s/^\(#define\s\+SERIAL_SPACE\s\+\)[01]\(.*\)/\11\2/' device/VehicleDetection.h
sed -i '/^#define PARALLEL_SPACE/s/^\(#define\s\+PARALLEL_SPACE\s\+\)[01]\(.*\)/\10\2/' device/VehicleDetection.h
IarBuild device\EW430\device.ewp -build Release -log warnings
echo.-----------------------------
echo.* 正在为并行泊位编译VD程序...
sed -i '/^#define SERIAL_SPACE/s/^\(#define\s\+SERIAL_SPACE\s\+\)[01]\(.*\)/\10\2/' device/VehicleDetection.h
sed -i '/^#define PARALLEL_SPACE/s/^\(#define\s\+PARALLEL_SPACE\s\+\)[01]\(.*\)/\11\2/' device/VehicleDetection.h
IarBuild device\EW430\device.ewp -build Release -log warnings
rem IarBuild router\EW430\router.ewp -build Release -log warnings
rem IarBuild gateway\EW430\gateway.ewp -build Release -log warnings
goto menu

:all
echo.-----------------------------
echo.* 正在为串行泊位编译VD程序...
sed -i '/^#define SERIAL_SPACE/s/^\(#define\s\+SERIAL_SPACE\s\+\)[01]\(.*\)/\11\2/' device/VehicleDetection.h
sed -i '/^#define PARALLEL_SPACE/s/^\(#define\s\+PARALLEL_SPACE\s\+\)[01]\(.*\)/\10\2/' device/VehicleDetection.h
IarBuild device\EW430\device.ewp -build Release -log warnings
echo.-----------------------------
echo.* 正在为并行泊位编译VD程序...
sed -i '/^#define SERIAL_SPACE/s/^\(#define\s\+SERIAL_SPACE\s\+\)[01]\(.*\)/\10\2/' device/VehicleDetection.h
sed -i '/^#define PARALLEL_SPACE/s/^\(#define\s\+PARALLEL_SPACE\s\+\)[01]\(.*\)/\11\2/' device/VehicleDetection.h
IarBuild device\EW430\device.ewp -build Release -log warnings
rem IarBuild router\EW430\router.ewp -build Release -log warnings
rem IarBuild gateway\EW430\gateway.ewp -build Release -log warnings
IarBuild ..\sniffer\EW430\sniffer.ewp -build Release -log warnings
IarBuild updater\EW430\updater.ewp -build Release -log warnings
IarBuild activator\EW430\activator.ewp -build Release -log warnings
IarBuild parking\EW430\parking.ewp -build Release -log warnings
goto menu

:err
echo.-----------------------------
echo.不支持该程序编译
echo.
goto menu

:end
