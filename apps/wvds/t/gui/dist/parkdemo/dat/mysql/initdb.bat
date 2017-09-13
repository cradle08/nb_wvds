@echo off

set PATH=C:\xampp\mysql\bin;C:\Program Files\MySQL\MySQL Server 5.5\bin;%PATH%

echo.MySQL数据库初始化
echo.
echo.遇到提示时请输入root用户密码
echo.(XAMPP的MySQL默认密码为空，直接回车即可)
echo.

echo.正在初始化wsnits数据库...
mysql -uroot -p mysql < wsnits.sql
echo.成功
echo.

echo.正在更改root用户密码...
mysql -uroot -p mysql < passwd.sql
echo.成功

@pause
