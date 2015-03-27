RedComet explains it best:
A 'soft debugger' system that automatically logs program execution
and memory breakpoints to files, requiring less user intervention.

Stef wrote the original code for Gens 2.12a.
This has merely been modified for translation specialists.

For Sega CD battery RAM files, make sure your chosen folder exists and is created.

ATTENTION!
- Memory access to BRAM from the M68K is now disabled. Fixes Shadowrun.
  If this causes a problem, please fix it in your build.

To use:
gens.exe

hook_log.txt must be kept with the executable.
hook_log_cd.txt must be kept with the executable.

Emulator keys:
- 'Ctrl-/' = toggles instruction logging to 'trace.log'
- 'Ctrl-.' = toggles memory logging to 'hook.txt' (set by 'hook_log.txt')

Instruction Logging:
- To reduce size of log file, the trace will only log the instructions at any given ROM offset once per toggle.
- Toggle on and off to get loops logged under different conditions

Memory logging:
Tracks memory changes between a reference range set by 'low' and 'high'.
Also monitors VRAM and VDP registers.

hook_log.txt has more information.
This assumes that you have some understanding of the target platform.


Memory logging tip:
- Hit 'Ctrl-/' or 'Ctrl-.' several times to turn it on and off. It will leave a
  'TRACE STOPPED' message. This can be useful to isolate parts of
  program code in context to the game.

- Setting 'hook_pc* 1 <start address> -1' will blindly log all instructions
  when it first reaches the start address. Turning off memory logging will stop.

  Can be done to understand difficult compression routines or troubleshooting
  gremlin bugs in code routines.

- You can enable logging before the game starts.

- Enable step frame. Load your save state. Start tracing a few frames for finicky
  problems. Smaller log files.