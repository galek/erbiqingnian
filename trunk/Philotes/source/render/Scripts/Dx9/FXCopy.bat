REM Usage: <batchfile> $(SolutionDir) $(InputName) $(InputPath) $(ParentName)
REM e.g. source\Scripts\DX9\FragmentCopy.bat $(SolutionDir) $(InputName) $(InputPath) $(ParentName)

@echo off
REM %windir%\System32\attrib -r "%1data\effects\%4\%2.fx" > NUL 2>&1
copy /Y "%3" "%1data\effects\%4\%2.fx" > NUL 2>&1