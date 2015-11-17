@ECHO off
if not exist ./code/run.exe (
	START CMD /k "@ECHO off & COLOR 74 & echo Welcome! installation is now in proccess... & ping 127.0.0.1 -n 2 -w 3000 > NUL & ping 127.0.0.1 -n %1 -w 3000 > NUL & exit"
	cd code
	gcc -std=c99 -o run *.c
	cd ..
)
START /MAX CMD /k "COLOR 3f & code\run.exe %*"
