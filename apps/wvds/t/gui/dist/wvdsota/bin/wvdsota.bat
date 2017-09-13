@echo off

title WVDS-OTA

set ROOT=%~dp0..
set PATH=%ROOT%\jre\bin;%PATH%

chdir %ROOT%
java -jar lib\wvdsota.jar
