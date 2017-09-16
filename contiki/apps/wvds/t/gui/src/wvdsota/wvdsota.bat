@echo off

title WVDS-OTA

set ROOT=%~dp0

set LIBDIR=%ROOT%\lib
set CLASSPATH=%ROOT%\bin;%ROOT%\wvdsota.jar;%LIBDIR%\commons-io-2.4.jar;%LIBDIR%\logback-classic-1.0.7.jar;%LIBDIR%\logback-core-1.0.7.jar;%LIBDIR%\slf4j-api-1.7.2.jar;%LIBDIR%\JTattoo-1.6.11.jar;%LIBDIR%\sqlite-jdbc-3.7.2.jar

java com.cadre.wvds.wvdsota.Main
