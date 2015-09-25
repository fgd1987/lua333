require('mysql')

local conn = Mysql.connect('127.0.0.1', 3306, 'ljw', '333')
print(conn)
