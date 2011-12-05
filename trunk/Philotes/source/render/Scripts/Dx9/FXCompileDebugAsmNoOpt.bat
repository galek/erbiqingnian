@echo off
echo Usage: %0 in.fx out.fxa
fxc /Tfx_2_0 /nologo /Op /Od /DPRIME /Zi /Fc%2 %1