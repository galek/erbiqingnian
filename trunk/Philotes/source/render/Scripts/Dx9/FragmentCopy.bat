@echo off
REM Usage: <batchfile> $(SolutionDir) $(InputName) $(InputPath) $(ParentName)
REM e.g. source\Scripts\DX9\FragmentCopy.bat $(SolutionDir) $(InputName) $(InputPath) $(ParentName)

REM %windir%\System32\attrib -r "%1data\effects\%4\%2.fx"
cl.exe /nologo /P /EP /C "%3" > NUL 2>&1
move "%2.i" "%2.temp"
cl.exe /nologo /P /EP /C "%2.temp" > NUL 2>&1
del "%2.temp" > NUL 2>&1
move "%2.i" "%1data\effects\%4\%2.fx"