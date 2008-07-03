# Microsoft Developer Studio Project File - Name="gens" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=gens - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "gens.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "gens.mak" CFG="gens - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gens - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "gens - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "gens - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# SUBTRACT CPP /Fr /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib wsock32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ddraw.lib dsound.lib dinput.lib dxguid.lib winmm.lib libs\zlib.lib libs\htmlhelp.lib libs\main68k.obj libs\sub68k.obj /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "gens - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ddraw.lib dsound.lib dinput.lib dxguid.lib zlib.lib winmm.lib release\mem_M68K.obj release\mem_S68K.obj release\mem_Z80.obj release\blit.obj \vc\gensCD~1\libs\main68k.obj \vc\gensCD~1\libs\sub68k.obj release\vdp_io.obj release\vdp_rend.obj release\io.obj release\Misc.obj release\z80.obj /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "gens - Win32 Release"
# Name "gens - Win32 Debug"
# Begin Group "C/C++ Sources"

# PROP Default_Filter ".c;.cpp"
# Begin Group "mp3_dec_c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\mp3_dec\common.c
# End Source File
# Begin Source File

SOURCE=.\mp3_dec\dct64_i386.c
# End Source File
# Begin Source File

SOURCE=.\mp3_dec\decode_i386.c
# End Source File
# Begin Source File

SOURCE=.\mp3_dec\interface.c
# End Source File
# Begin Source File

SOURCE=.\mp3_dec\layer3.c
# End Source File
# Begin Source File

SOURCE=.\mp3_dec\tabinit.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\CCnet.c
# End Source File
# Begin Source File

SOURCE=.\cd_aspi.cpp
# End Source File
# Begin Source File

SOURCE=.\cd_file.c
# End Source File
# Begin Source File

SOURCE=.\cd_sys.c
# End Source File
# Begin Source File

SOURCE=.\cdda_mp3.c
# End Source File
# Begin Source File

SOURCE=.\Cpu_68k.c
# End Source File
# Begin Source File

SOURCE=.\Cpu_SH2.c
# End Source File
# Begin Source File

SOURCE=.\Cpu_Z80.c
# End Source File
# Begin Source File

SOURCE=.\Debug.cpp
# End Source File
# Begin Source File

SOURCE=.\G_ddraw.cpp
# End Source File
# Begin Source File

SOURCE=.\G_dinput.cpp
# End Source File
# Begin Source File

SOURCE=.\G_dsound.cpp
# End Source File
# Begin Source File

SOURCE=.\G_main.cpp
# End Source File
# Begin Source File

SOURCE=.\Gens.cpp
# End Source File
# Begin Source File

SOURCE=.\ggenie.cpp
# End Source File
# Begin Source File

SOURCE=.\LC89510.c
# End Source File
# Begin Source File

SOURCE=.\M68KD.c
# End Source File
# Begin Source File

SOURCE=.\net.cpp
# End Source File
# Begin Source File

SOURCE=.\pcm.c
# End Source File
# Begin Source File

SOURCE=.\psg.c
# End Source File
# Begin Source File

SOURCE=.\Rom.cpp
# End Source File
# Begin Source File

SOURCE=.\save.cpp
# End Source File
# Begin Source File

SOURCE=.\scrshot.cpp
# End Source File
# Begin Source File

SOURCE=.\SH2.c
# End Source File
# Begin Source File

SOURCE=.\SH2D.c
# End Source File
# Begin Source File

SOURCE=.\unzip.c
# End Source File
# Begin Source File

SOURCE=.\wave.c
# End Source File
# Begin Source File

SOURCE=.\ym2612.c
# End Source File
# Begin Source File

SOURCE=.\z80dis.c
# End Source File
# End Group
# Begin Group "ASM Sources"

# PROP Default_Filter ".asm"
# Begin Source File

SOURCE=.\blit.asm

!IF  "$(CFG)" == "gens - Win32 Release"

# Begin Custom Build - Assembling $(InputName).asm
InputDir=.
OutDir=.\Release
InputPath=.\blit.asm
InputName=blit

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	\nasm\nasmw -i $(InputDir) $(InputDir)\$(InputName).asm -f win32 -o $(OutDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gens - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gfx_cd.asm

!IF  "$(CFG)" == "gens - Win32 Release"

# Begin Custom Build - Assembling $(InputName).asm
InputDir=.
OutDir=.\Release
InputPath=.\gfx_cd.asm
InputName=gfx_cd

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	\nasm\nasmw -i $(InputDir) $(InputDir)\$(InputName).asm -f win32 -o $(OutDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gens - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\io.asm

!IF  "$(CFG)" == "gens - Win32 Release"

# Begin Custom Build - Assembling $(InputName).asm
InputDir=.
OutDir=.\Release
InputPath=.\io.asm
InputName=io

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	\nasm\nasmw -i $(InputDir) $(InputDir)\$(InputName).asm -f win32 -o $(OutDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gens - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Mem_M68k.asm

!IF  "$(CFG)" == "gens - Win32 Release"

USERDEP__MEM_M="mem_m68k_32x.inc"	"mem_m68k_cd.inc"	
# Begin Custom Build - Assembling $(InputName).asm
InputDir=.
OutDir=.\Release
InputPath=.\Mem_M68k.asm
InputName=Mem_M68k

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	\nasm\nasmw -i $(InputDir) $(InputDir)\$(InputName).asm -f win32 -o $(OutDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gens - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Mem_M68K_32X.inc
# End Source File
# Begin Source File

SOURCE=.\Mem_M68K_CD.inc
# End Source File
# Begin Source File

SOURCE=.\Mem_S68k.asm

!IF  "$(CFG)" == "gens - Win32 Release"

# Begin Custom Build - Assembling $(InputName).asm
InputDir=.
OutDir=.\Release
InputPath=.\Mem_S68k.asm
InputName=Mem_S68k

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	\nasm\nasmw -i $(InputDir) $(InputDir)\$(InputName).asm -f win32 -o $(OutDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gens - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Mem_SH2.asm

!IF  "$(CFG)" == "gens - Win32 Release"

# Begin Custom Build - Assembling $(InputName).asm
InputDir=.
OutDir=.\Release
InputPath=.\Mem_SH2.asm
InputName=Mem_SH2

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	\nasm\nasmw -i $(InputDir) $(InputDir)\$(InputName).asm -f win32 -o $(OutDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gens - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Mem_Z80.asm

!IF  "$(CFG)" == "gens - Win32 Release"

# Begin Custom Build - Assembling $(InputName).asm
InputDir=.
OutDir=.\Release
InputPath=.\Mem_Z80.asm
InputName=Mem_Z80

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	\nasm\nasmw -i $(InputDir) $(InputDir)\$(InputName).asm -f win32 -o $(OutDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gens - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Misc.asm

!IF  "$(CFG)" == "gens - Win32 Release"

# Begin Custom Build - Assembling $(InputName).asm
InputDir=.
OutDir=.\Release
InputPath=.\Misc.asm
InputName=Misc

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	\nasm\nasmw -i $(InputDir) $(InputDir)\$(InputName).asm -f win32 -o $(OutDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gens - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\pwm.asm

!IF  "$(CFG)" == "gens - Win32 Release"

# Begin Custom Build - Assembling $(InputName).asm
InputDir=.
OutDir=.\Release
InputPath=.\pwm.asm
InputName=pwm

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	\nasm\nasmw -i $(InputDir) $(InputDir)\$(InputName).asm -f win32 -o $(OutDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gens - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SH2_IO.inc
# End Source File
# Begin Source File

SOURCE=.\SH2a.asm

!IF  "$(CFG)" == "gens - Win32 Release"

USERDEP__SH2A_="SH2_IO.inc"	
# Begin Custom Build - Assembling $(InputName).asm
InputDir=.
OutDir=.\Release
InputPath=.\SH2a.asm
InputName=SH2a

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	\nasm\nasmw -i $(InputDir) $(InputDir)\$(InputName).asm -f win32 -o $(OutDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gens - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vdp_32X.asm

!IF  "$(CFG)" == "gens - Win32 Release"

# Begin Custom Build - Assembling $(InputName).asm
InputDir=.
OutDir=.\Release
InputPath=.\vdp_32X.asm
InputName=vdp_32X

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	\nasm\nasmw -i $(InputDir) $(InputDir)\$(InputName).asm -f win32 -o $(OutDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gens - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vdp_io.asm

!IF  "$(CFG)" == "gens - Win32 Release"

# Begin Custom Build - Assembling $(InputName).asm
InputDir=.
OutDir=.\Release
InputPath=.\vdp_io.asm
InputName=vdp_io

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	\nasm\nasmw -i $(InputDir) $(InputDir)\$(InputName).asm -f win32 -o $(OutDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gens - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vdp_rend.asm

!IF  "$(CFG)" == "gens - Win32 Release"

# Begin Custom Build - Assembling $(InputName).asm
InputDir=.
OutDir=.\Release
InputPath=.\vdp_rend.asm
InputName=vdp_rend

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	\nasm\nasmw -i $(InputDir) $(InputDir)\$(InputName).asm -f win32 -o $(OutDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gens - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\z80.asm

!IF  "$(CFG)" == "gens - Win32 Release"

# Begin Custom Build - Assembling $(InputName).asm
InputDir=.
OutDir=.\Release
InputPath=.\z80.asm
InputName=z80

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	\nasm\nasmw -i $(InputDir) $(InputDir)\$(InputName).asm -f win32 -o $(OutDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "gens - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Headers"

# PROP Default_Filter ".h"
# Begin Group "mp3_dec_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\mp3_dec\huffman.h
# End Source File
# Begin Source File

SOURCE=.\mp3_dec\mpg123.h
# End Source File
# Begin Source File

SOURCE=.\mp3_dec\mpglib.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\blit.h
# End Source File
# Begin Source File

SOURCE=.\CCnet.h
# End Source File
# Begin Source File

SOURCE=.\cd_aspi.h
# End Source File
# Begin Source File

SOURCE=.\cd_file.h
# End Source File
# Begin Source File

SOURCE=.\Cd_sys.h
# End Source File
# Begin Source File

SOURCE=.\cdda_mp3.h
# End Source File
# Begin Source File

SOURCE=.\Cpu_68k.h
# End Source File
# Begin Source File

SOURCE=.\Cpu_SH2.h
# End Source File
# Begin Source File

SOURCE=.\Cpu_Z80.h
# End Source File
# Begin Source File

SOURCE=.\Debug.h
# End Source File
# Begin Source File

SOURCE=.\G_ddraw.h
# End Source File
# Begin Source File

SOURCE=.\G_dsound.h
# End Source File
# Begin Source File

SOURCE=.\G_Input.h
# End Source File
# Begin Source File

SOURCE=.\G_main.h
# End Source File
# Begin Source File

SOURCE=.\gens.h
# End Source File
# Begin Source File

SOURCE=.\gfx_cd.h
# End Source File
# Begin Source File

SOURCE=.\ggenie.h
# End Source File
# Begin Source File

SOURCE=.\htmlhelp.h
# End Source File
# Begin Source File

SOURCE=.\io.h
# End Source File
# Begin Source File

SOURCE=.\kailleraclient.h
# End Source File
# Begin Source File

SOURCE=.\keycode.h
# End Source File
# Begin Source File

SOURCE=.\LC89510.h
# End Source File
# Begin Source File

SOURCE=.\M68KD.h
# End Source File
# Begin Source File

SOURCE=.\Mem_M68k.h
# End Source File
# Begin Source File

SOURCE=.\Mem_S68k.h
# End Source File
# Begin Source File

SOURCE=.\Mem_SH2.h
# End Source File
# Begin Source File

SOURCE=.\Mem_Z80.h
# End Source File
# Begin Source File

SOURCE=.\Misc.h
# End Source File
# Begin Source File

SOURCE=.\net.h
# End Source File
# Begin Source File

SOURCE=.\pcm.h
# End Source File
# Begin Source File

SOURCE=.\psg.h
# End Source File
# Begin Source File

SOURCE=.\pwm.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\Rom.h
# End Source File
# Begin Source File

SOURCE=.\save.h
# End Source File
# Begin Source File

SOURCE=.\scrshot.h
# End Source File
# Begin Source File

SOURCE=.\SH2.h
# End Source File
# Begin Source File

SOURCE=.\SH2D.h
# End Source File
# Begin Source File

SOURCE=.\Star_68k.h
# End Source File
# Begin Source File

SOURCE=.\unzip.h
# End Source File
# Begin Source File

SOURCE=.\vdp_32X.h
# End Source File
# Begin Source File

SOURCE=.\vdp_io.h
# End Source File
# Begin Source File

SOURCE=.\vdp_rend.h
# End Source File
# Begin Source File

SOURCE=.\wave.h
# End Source File
# Begin Source File

SOURCE=.\ym2612.h
# End Source File
# Begin Source File

SOURCE=.\z80.h
# End Source File
# Begin Source File

SOURCE=.\z80dis.h
# End Source File
# Begin Source File

SOURCE=.\zconf.h
# End Source File
# Begin Source File

SOURCE=.\zlib.h
# End Source File
# End Group
# Begin Group "Misc"

# PROP Default_Filter ""
# Begin Group "H"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\Program Files\Microsoft Visual Studio\Vc98\Include\Basetsd.h"
# End Source File
# Begin Source File

SOURCE="..\..\Program Files\Microsoft Visual Studio\Vc98\Include\Io.h"
# End Source File
# End Group
# End Group
# Begin Group "resources"

# PROP Default_Filter ""
# Begin Group "icon"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\gens.ico
# End Source File
# Begin Source File

SOURCE=.\resource\Gens2.ico
# End Source File
# Begin Source File

SOURCE=.\resource\sonic.ico
# End Source File
# End Group
# Begin Group "bitmap"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\resource\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\bmp00000.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\bmp00002.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\bmp00003.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\bmp00004.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\bmp00005.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\bmp00006.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\gens_big.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\gens_small.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\Gens.rc
# End Source File
# End Group
# End Target
# End Project
