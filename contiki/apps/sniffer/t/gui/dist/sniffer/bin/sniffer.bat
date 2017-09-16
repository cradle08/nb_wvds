@echo off

title Sniffer

set ROOT=%~dp0..
set PATH=%ROOT%\jre\bin;C:\Program Files\Java\jdk1.8.0_60\jre\bin;C:\Program Files (x86)\Java\jdk1.8.0_60\jre\bin

chdir %ROOT%
java -jar lib\sniffer.jar
