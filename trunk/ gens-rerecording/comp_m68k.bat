rem Main 68000 compilation (Main68k\star.c has been compiled before)

@cd starscream\main68k\release
star temp.asm -hog -name main68k_
nasm\nasmw -f win32 temp.asm -o ..\..\..\gens\libs\main68k.obj
@cd..\..\..