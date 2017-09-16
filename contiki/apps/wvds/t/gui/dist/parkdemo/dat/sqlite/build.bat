@echo off

set NAME=parkdemo

:menu
echo.-----------------
echo. i) ��ʼ��
echo. b) �������ݿ�
echo. s) ���ݱ���
echo. c) �������
echo. q) �˳�
echo.-----------------
set /p choice=��ѡ��: 
echo.-----------------

if %choice%==i goto init
if %choice%==b goto backup
if %choice%==s goto schema
if %choice%==c goto clean
if %choice%==q goto end
goto menu

:init
move %NAME%.db %NAME%.db~
sqlite3 %NAME%.db < %NAME%.sql
echo.��ʼ���ɹ�
goto menu

:backup
copy %NAME%.db %NAME%-dat.db
sqlite3 %NAME%.db ".dump" > %NAME%-dat.sql
echo.�ɹ����ݵ�%NAME%-dat.sql
goto menu

:schema
sqlite3 %NAME%.db ".schema" > %NAME%.sql
echo.�ɹ����ݵ�%NAME%.sql
goto menu

:clean
sqlite3 %NAME%.db < clean.sql
echo.����ɹ�
goto menu

:end
