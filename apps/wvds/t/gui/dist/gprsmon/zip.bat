@echo off

set NAME=gprsmon

if exist "jre\bin\java.exe" goto zip
if "%JAVA_HOME%" == "" goto nojava

echo.* ���ڸ���jre...
set PATH=C:\WINDOWS\system32;C:\Windows\SysWOW64
xcopy "%JAVA_HOME%"\jre jre\ /e /c /q /y

:zip
chdir %~dp0..
if exist "C:\Program Files\7-Zip\7z.exe" goto 7z
if exist "C:\Program Files (x86)\7-Zip\7z.exe" goto 7z
if exist "C:\Program Files\WinRAR\Rar.exe" goto rar
if exist "C:\Program Files (x86)\WinRAR\Rar.exe" goto rar
goto nocomp

echo.* ����ѹ���ļ�...
:7z
set PATH=C:\Program Files\7-Zip;C:\Program Files (x86)\7-Zip
7z a %NAME%.zip %NAME% -x!%NAME%\dat\* -x!%NAME%\log\* -x!%NAME%\%NAME%.vbs2exe -x!%NAME%\zip.bat
goto end

:rar
set PATH=C:\Program Files\WinRAR;C:\Program Files (x86)\WinRAR
Rar a -x%NAME%\dat\* -x%NAME%\log\* -x%NAME%\%NAME%.vbs2exe -x%NAME%\zip.bat %NAME%.rar %NAME%
goto end

:nojava
echo "δ���廷������JAVA_HOME���붨���������"
goto end

:nocomp
echo "δ��װ7-Zip��WinRARѹ��������밲װ��������"

:end
pause
