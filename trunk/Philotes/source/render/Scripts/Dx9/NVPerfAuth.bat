REM Usage: <batchfile> $(ProjectDir) $(ProjectName)
REM @echo off
"..\3rd Party\NVIDIA\PerfKit\NVAppAuth" %1\%2.exe
REM cmd.exe /C del %1\%2.exe
del %1\%2.exe
REM cmd.exe /C move %1\out-%2.exe %1\%2.exe
move %1\out-%2.exe %1\%2.exe
