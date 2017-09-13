@echo off

set PATH=C:\cygwin\bin;%PATH%

chdir %~dp0..
sh prebuild.sh
