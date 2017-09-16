@echo off

title Sniffer

set ROOT=%~dp0
set LIBDIR=%ROOT%lib

set CLASSPATH=%ROOT%bin;%ROOT%\sniffer.jar;%LIBDIR%\commons-io-2.4.jar;%LIBDIR%\dom4j-1.6.1.jar;%LIBDIR%\fastjson-1.1.24.jar;%LIBDIR%\jaxen-1.1.3.jar;%LIBDIR%\jcommon-1.0.17.jar;%LIBDIR%\jfreechart-1.0.14.jar;%LIBDIR%\logback-classic-1.0.13.jar;%LIBDIR%\logback-core-1.0.13.jar;%LIBDIR%\slf4j-api-1.7.5.jar;%LIBDIR%\sqlite-jdbc-3.7.2.jar

java com.cadre.wvds.sniffer.Main
