rem Main 68000 compilation (Sub68k\star.c has been compiled before)

@cd starscream\sub68k\release
star temp.asm -hog -name sub68k_
nasm\nasmw -f win32 temp.asm -o ..\..\..\gens\libs\sub68k.obj
@cd..\..\..