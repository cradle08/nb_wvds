@echo off

title GPRSMon

set ROOT=%~dp0
set LIBDIR=%ROOT%\lib

set CLASSPATH=%ROOT%\bin;%ROOT%\gprsmon.jar;%LIBDIR%\JTattoo-1.6.11.jar;%LIBDIR%\logback-classic-1.0.13.jar;%LIBDIR%\logback-core-1.0.13.jar;%LIBDIR%\slf4j-api-1.7.5.jar;%LIBDIR%\commons-io-2.4.jar

java com.cadre.wvds.gprsmon.Main
