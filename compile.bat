echo off
cl *.cpp /c /I zlib
link *.obj user32.lib comctl32.lib Ws2_32.lib iphlpapi.lib zlibwapi.lib /OUT:main.exe /subsystem:console
rem mt -manifest manifest.txt -outputresource:main.exe;1