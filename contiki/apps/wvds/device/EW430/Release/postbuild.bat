@echo off

set PATH=C:\cygwin\bin;%PATH%

chdir %~dp0
sh postbuild.sh
