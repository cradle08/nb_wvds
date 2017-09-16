USE mysql;
UPDATE user SET Password=password('123456') WHERE User='root';
FLUSH PRIVILEGES;
