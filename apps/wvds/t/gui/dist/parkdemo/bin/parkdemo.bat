@echo off

title ParkDemo

set ROOT=%~dp0..
set PATH=%ROOT%\jre\bin;C:\Program Files\Java\jdk1.8.0_60\jre\bin;C:\Program Files (x86)\Java\jdk1.8.0_60\jre\bin;%PATH%

set LIBDIR=%ROOT%\lib
set CLASSPATH=%LIBDIR%\parkdemo.jar;%LIBDIR%\charting-0.94.jar;%LIBDIR%\commons-collections4-4.1.jar;%LIBDIR%\commons-io-2.4.jar;%LIBDIR%\commons-lang3-3.5.jar;%LIBDIR%\fastjson-1.1.8.jar;%LIBDIR%\jasperreports-4.6.0.jar;%LIBDIR%\jcalendar-1.4.jar;%LIBDIR%\jcommon-1.0.17.jar;%LIBDIR%\jeromq-0.3.5.jar;%LIBDIR%\jfreechart-1.0.14.jar;%LIBDIR%\jgoodies-common-1.2.0.jar;%LIBDIR%\jgoodies-looks-2.4.1.jar;%LIBDIR%\logback-classic-1.0.7.jar;%LIBDIR%\logback-core-1.0.7.jar;%LIBDIR%\mysql-connector-java-3.1.14-bin.jar;%LIBDIR%\slf4j-api-1.7.2.jar;%LIBDIR%\sqlite-jdbc-3.7.2.jar

chdir %ROOT%
java net.tinyos.demo.MainClass
