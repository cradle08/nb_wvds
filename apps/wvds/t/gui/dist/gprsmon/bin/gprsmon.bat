@echo off

title GPRSMon

set ROOT=%~dp0..
set PATH=%ROOT%\jre\bin;C:\Program Files\Java\jdk1.8.0_60\jre\bin;C:\Program Files (x86)\Java\jdk1.8.0_60\jre\bin;%PATH%

chdir %ROOT%
java -jar lib\gprsmon.jar
