@echo off
setlocal enableextensions
for /F "tokens=* USEBACKQ" %%F in (`dependencies\git-rev-list.exe --count HEAD`) do (
	set version=%%F
)

echo #define GIT_REV "%version%" > "dependencies\gitrev.h"
endlocal
exit 0
