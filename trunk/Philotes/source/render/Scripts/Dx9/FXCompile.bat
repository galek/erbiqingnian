REM Usage: <batchfile> $(SolutionDir) $(FXC) $(InputName) $(InputPath) $(ParentName)
REM e.g. source\Scripts\FXCompile.bat $(SolutionDir) %(FXC) $(InputName) $(InputPath) $(ParentName)

@echo off
REM %windir%\System32\attrib.exe -r %1data\effects\%5\%3.fxo
%1source\Engine\Scripts\%5\_FXCompileDebug.bat %2 "%1data\effects\%5\%3.fxo" "%4"