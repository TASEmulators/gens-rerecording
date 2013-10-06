del /s Gens11svn326.zip
cd Output
copy Gens.exe ..
..\dependencies\upx Gens.exe
IF ERRORLEVEL 1 IF NOT ERRORLEVEL 2 GOTO UPXFailed
..\dependencies\zip -X -9 -r ..\Gens11svn326.zip Gens.exe LuaSamples TracerHelp HookLog language.dat changelog.txt
move /y ..\Gens.exe .
GOTO end

:UPXFailed
CLS
echo.
echo GENS.EXE is either compiling or running.
echo Close it or let it finish before trying this script again.
pause

:end
