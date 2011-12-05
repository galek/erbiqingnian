@echo off
echo Usage: %0 in.fx out.fxa
fxc /Tfx_2_0 /nologo /Op /DPRIME /Zi /Fc%2 %1

