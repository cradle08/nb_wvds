@echo off

set PATH=C:\xampp\mysql\bin;C:\Program Files\MySQL\MySQL Server 5.5\bin;%PATH%

echo.MySQL���ݿ��ʼ��
echo.
echo.������ʾʱ������root�û�����
echo.(XAMPP��MySQLĬ������Ϊ�գ�ֱ�ӻس�����)
echo.

echo.���ڳ�ʼ��wsnits���ݿ�...
mysql -uroot -p mysql < wsnits.sql
echo.�ɹ�
echo.

echo.���ڸ���root�û�����...
mysql -uroot -p mysql < passwd.sql
echo.�ɹ�

@pause
