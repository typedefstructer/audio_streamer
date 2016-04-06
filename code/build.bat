@echo off
if not exist ..\build mkdir ..\build
pushd ..\build
      REM cl -nologo -FC -Zi ..\code\server.cpp ws2_32.lib dsound.lib winmm.lib user32.lib ws2_32.lib
      cl -nologo -FC -Zi ..\code\client.cpp ws2_32.lib dsound.lib winmm.lib user32.lib ws2_32.lib user32.lib gdi32.lib
popd
