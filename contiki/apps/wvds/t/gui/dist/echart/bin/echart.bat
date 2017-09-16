@echo off

set ROOTDIR=%~dp0..

set PATH=%ROOTDIR%\jre\bin;%PATH%

chdir %ROOTDIR%
java -jar lib\echart.jar
