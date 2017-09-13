@echo off

set NAME=parkdemo

:menu
echo.-----------------
echo. i) 初始化
echo. b) 备份数据库
echo. s) 备份表定义
echo. c) 清除数据
echo. q) 退出
echo.-----------------
set /p choice=请选择: 
echo.-----------------

if %choice%==i goto init
if %choice%==b goto backup
if %choice%==s goto schema
if %choice%==c goto clean
if %choice%==q goto end
goto menu

:init
move %NAME%.db %NAME%.db~
sqlite3 %NAME%.db < %NAME%.sql
echo.初始化成功
goto menu

:backup
copy %NAME%.db %NAME%-dat.db
sqlite3 %NAME%.db ".dump" > %NAME%-dat.sql
echo.成功备份到%NAME%-dat.sql
goto menu

:schema
sqlite3 %NAME%.db ".schema" > %NAME%.sql
echo.成功备份到%NAME%.sql
goto menu

:clean
sqlite3 %NAME%.db < clean.sql
echo.清除成功
goto menu

:end
