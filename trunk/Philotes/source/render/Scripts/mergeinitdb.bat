@echo off
set _PERL_="..\..\..\..\3rd Party\fmod\4.04.25\source\win32\bin\perl.exe"
%_PERL_% mergeinitdb.pl %1 %2 %3
