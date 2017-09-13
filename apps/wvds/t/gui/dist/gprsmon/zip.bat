@echo off

set NAME=gprsmon

if exist "jre\bin\java.exe" goto zip
if "%JAVA_HOME%" == "" goto nojava

echo.* 正在复制jre...
set PATH=C:\WINDOWS\system32;C:\Windows\SysWOW64
xcopy "%JAVA_HOME%"\jre jre\ /e /c /q /y

:zip
chdir %~dp0..
if exist "C:\Program Files\7-Zip\7z.exe" goto 7z
if exist "C:\Program Files (x86)\7-Zip\7z.exe" goto 7z
if exist "C:\Program Files\WinRAR\Rar.exe" goto rar
if exist "C:\Program Files (x86)\WinRAR\Rar.exe" goto rar
goto nocomp

echo.* 正在压缩文件...
:7z
set PATH=C:\Program Files\7-Zip;C:\Program Files (x86)\7-Zip
7z a %NAME%.zip %NAME% -x!%NAME%\dat\* -x!%NAME%\log\* -x!%NAME%\%NAME%.vbs2exe -x!%NAME%\zip.bat
goto end

:rar
set PATH=C:\Program Files\WinRAR;C:\Program Files (x86)\WinRAR
Rar a -x%NAME%\dat\* -x%NAME%\log\* -x%NAME%\%NAME%.vbs2exe -x%NAME%\zip.bat %NAME%.rar %NAME%
goto end

:nojava
echo "未定义环境变量JAVA_HOME，请定义后再运行"
goto end

:nocomp
echo "未安装7-Zip或WinRAR压缩软件，请安装后再运行"

:end
pause
