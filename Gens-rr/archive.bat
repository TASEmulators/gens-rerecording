del /s Gens.zip
cd Output
copy Gens.exe ..
..\dependencies\upx Gens.exe
IF ERRORLEVEL 1 IF NOT ERRORLEVEL 2 GOTO UPXFailed
rename Gens.exe Gens11svn320.exe
..\dependencies\zip -X -9 -r ..\Gens.zip Gens11svn320.exe LuaSamples TracerHelp HookLog language.dat changelog.txt
del Gens11svn320.exe
move /y ..\Gens.exe .
GOTO end

:UPXFailed
CLS
echo.
echo GENS.EXE is either compiling or running.
echo Close it or let it finish before trying this script again.
pause

:end
