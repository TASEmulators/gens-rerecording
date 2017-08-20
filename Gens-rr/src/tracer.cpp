#include <stdio.h>
#include <memory.h>

#include "Cpu_68k.h"
#include "M68KD.h"
#include "Mem_M68k.h"
#include "Mem_S68k.h"
#include "vdp_io.h"
#include "luascript.h"
#include "tracer.h"

#define uint32 unsigned int

FILE* fp_trace;
FILE* fp_hook;

#define STATES 3
unsigned int *rd_mode, *wr_mode, *ppu_mode, *pc_mode;
unsigned int *rd_low, *rd_high;
unsigned int *wr_low, *wr_high;
unsigned int *ppu_low, *ppu_high;
unsigned int *pc_low, *pc_high;
unsigned int *pc_start;

extern bool trace_map;
extern bool hook_trace;

extern "C" {
	extern uint32 hook_address;
	extern uint32 hook_value;
	extern uint32 hook_pc;
	
	unsigned int dma_src, dma_len;
};

char *mapped;
uint32 Current_PC;
int Debug = 1;

const char * InitTrace()
{
	if( !fp_trace )
	{
		fp_trace = fopen( "./Logs/trace.log", "a" );
		if ( !fp_trace )
			return "Can't open file ./Logs/trace.log";
		mapped = new char[ 0x100*0x10000 ];
		memset( mapped,0,0x100*0x10000 );
//		fseek(fp_trace,0,SEEK_END);
		fprintf(fp_trace,"TRACE STARTED\n\n");
	}
	return NULL;
}

void DeInitTrace()
{
	if( fp_trace )
	{
		fprintf(fp_trace,"\nTRACE STOPPED\n\n");
		fclose(fp_trace);
		delete [] (mapped);
		fp_trace = NULL;
		mapped = NULL;
	}
}

const char * InitDebug()
{
	FILE *fp1 = fopen( "./Logs/hook_log.txt", "r" );
	if( !fp1 )
		return "File ./Logs/hook_log.txt not found.";

	if( !fp_hook )
	{
		fp_hook = fopen( "./Logs/hook.txt", "a" );
		if ( !fp_hook )
		{
			fclose(fp1);
			return "Can't open file ./Logs/hook.txt";
		}
		fseek(fp_hook, 0, SEEK_END);
	}

	rd_mode = new unsigned int[ STATES ];
	wr_mode = new unsigned int[ STATES ];
	pc_mode = new unsigned int[ STATES ];
	ppu_mode = new unsigned int[ STATES ];

	rd_low = new unsigned int[ STATES ];
	wr_low = new unsigned int[ STATES ];
	pc_low = new unsigned int[ STATES ];
	ppu_low = new unsigned int[ STATES ];

	rd_high = new unsigned int[ STATES ];
	wr_high = new unsigned int[ STATES ];
	pc_high = new unsigned int[ STATES ];
	ppu_high = new unsigned int[ STATES ];

	pc_start = new unsigned int[ STATES ];

	fscanf(fp1,"hook_pc1 %x %x %x\n",&pc_mode[0],&pc_low[0],&pc_high[0]);
	fscanf(fp1,"hook_pc2 %x %x %x\n",&pc_mode[1],&pc_low[1],&pc_high[1]);
	fscanf(fp1,"hook_pc3 %x %x %x\n",&pc_mode[2],&pc_low[2],&pc_high[2]);

	fscanf(fp1,"hook_rd1 %x %x %x\n",&rd_mode[0],&rd_low[0],&rd_high[0]);
	fscanf(fp1,"hook_rd2 %x %x %x\n",&rd_mode[1],&rd_low[1],&rd_high[1]);
	fscanf(fp1,"hook_rd3 %x %x %x\n",&rd_mode[2],&rd_low[2],&rd_high[2]);

	fscanf(fp1,"hook_wr1 %x %x %x\n",&wr_mode[0],&wr_low[0],&wr_high[0]);
	fscanf(fp1,"hook_wr2 %x %x %x\n",&wr_mode[1],&wr_low[1],&wr_high[1]);
	fscanf(fp1,"hook_wr3 %x %x %x\n",&wr_mode[2],&wr_low[2],&wr_high[2]);

	fscanf(fp1,"hook_ppu1 %x %x %x\n",&ppu_mode[0],&ppu_low[0],&ppu_high[0]);
	fscanf(fp1,"hook_ppu2 %x %x %x\n",&ppu_mode[1],&ppu_low[1],&ppu_high[1]);
	fscanf(fp1,"hook_ppu3 %x %x %x\n",&ppu_mode[2],&ppu_low[2],&ppu_high[2]);

	pc_start[0] = 0;
	pc_start[1] = 0;
	pc_start[2] = 0;

	fclose( fp1 );

	fprintf(fp_hook,"MEMORY ACCESS LOGGING STARTED\n\n");

	return NULL;
}

void DeInitDebug()
{
	if(fp_hook && (fp_hook != fp_trace))
	{
		fprintf(fp_hook,"\nMEMORY ACCESS LOGGING STOPPED\n\n");
		fclose(fp_hook);
		fp_hook = NULL;
	}

	if( rd_mode )
	{
		delete[] rd_mode;
		delete[] wr_mode;
		delete[] pc_mode;
		delete[] ppu_mode;

		delete[] rd_low;
		delete[] wr_low;
		delete[] pc_low;
		delete[] ppu_low;

		delete[] rd_high;
		delete[] wr_high;
		delete[] pc_high;
		delete[] ppu_high;

		delete[] pc_start;

		rd_mode = NULL;
	}
}

unsigned short Next_Word_T(void)
{
	unsigned short val;
	
	if (Debug == 1) val = M68K_RW(Current_PC);
	else if (Debug >= 2) val = S68K_RW(Current_PC);

	Current_PC += 2;

	return(val);
}

unsigned int Next_Long_T(void)
{
	unsigned int val;
	
	if (Debug == 1)
	{
		val = M68K_RW(Current_PC);
		val <<= 16;
		val |= M68K_RW(Current_PC + 2);
	}
	else if (Debug >= 2)
	{
		val = S68K_RW(Current_PC);
		val <<= 16;
		val |= S68K_RW(Current_PC + 2);
	}

	Current_PC += 4;

	return(val);
}


void Print_Instruction( FILE *trace )
{
	char String [512];
	Current_PC = hook_pc;

	int PC;
	int OPC = M68K_RW(Current_PC);

	PC = Current_PC;
	sprintf( String, "%02X:%04X  %02X %02X  %-33s",
		PC >> 16, PC & 0xffff, OPC >> 8, OPC & 0xff,
		M68KDisasm2( Next_Word_T, Next_Long_T, hook_pc ) );
	fprintf( trace, "%s", String );

	sprintf( String, "A0=%.8X A1=%.8X A2=%.8X ", main68k_context.areg[0], main68k_context.areg[1], main68k_context.areg[2]);
	fprintf( trace, "%s", String );

	sprintf( String, "A3=%.8X A4=%.8X A5=%.8X ", main68k_context.areg[3], main68k_context.areg[4], main68k_context.areg[5]);
	fprintf( trace, "%s", String );

	sprintf( String, "A6=%.8X A7=%.8X D0=%.8X ", main68k_context.areg[6], main68k_context.areg[7], main68k_context.dreg[0]);
	fprintf( trace, "%s", String );

	sprintf( String, " ~~  " );

	sprintf( String, "D1=%.8X D2=%.8X D3=%.8X ", main68k_context.dreg[1], main68k_context.dreg[2], main68k_context.dreg[3]);
	fprintf( trace, "%s", String );

	sprintf( String, "D4=%.8X D5=%.8X D6=%.8X ", main68k_context.dreg[4], main68k_context.dreg[5], main68k_context.dreg[6]);
	fprintf( trace, "%s", String );

	sprintf( String, "D7=%.8X ", main68k_context.dreg[7]);
	fprintf( trace, "%s", String );

	fprintf( trace, "%c", (main68k_context.sr & 0x10)?'X':'x' );
	fprintf( trace, "%c", (main68k_context.sr & 0x08)?'N':'n' );
	fprintf( trace, "%c", (main68k_context.sr & 0x04)?'Z':'z' );
	fprintf( trace, "%c", (main68k_context.sr & 0x02)?'V':'v' );
	fprintf( trace, "%c", (main68k_context.sr & 0x01)?'C':'c' );

	fprintf( trace, "\n", String );
}

static void GensTrace_trace()
{
	if(mapped[ hook_pc ] < 0x40)
	{
		Print_Instruction( fp_trace );
		mapped[ hook_pc ] ++;
	}
}

static void GensTrace_hook()
{
	for( int lcv = 0; lcv < STATES; lcv++ )
	{
		FILE *out;

		// start-stop
		if( pc_mode[ lcv ] & 1 )
		{
			if( hook_pc == pc_low[lcv] )
				pc_start[lcv] = 1;

			if( !pc_start[lcv] ) continue;

			out = fp_hook;
		}
		// low-high
		else
		{
			// fail: outside boundaries
			if( hook_pc < pc_low[ lcv ] ) continue;
			if( hook_pc > pc_high[ lcv ] ) continue;
		}

// ------------------------------------------------------

		if( pc_mode[ lcv ] >= 4 && pc_mode[ lcv ] < 8 )
		{
			pc_mode[ lcv ] &= 3;
			trace_map = 1;
		}

		// output file mode
		out = ( pc_mode[ lcv ] <= 1 ) ? fp_hook : fp_trace;

		if( !out ) continue;
		if( out == fp_trace )
			fprintf( out, "** " );

		Print_Instruction( out );


		// end formatting
		if( hook_pc == pc_high[lcv] )
		{
			pc_start[lcv] = 0;

			if( pc_low[ lcv ] != pc_high[ lcv ] )
				fprintf( out, "\n" );
		}
	} // end STATES
}

void GensTrace()
{
	// Trace.txt
	if( trace_map )
		GensTrace_trace();

	// Hook.txt
	if( hook_trace )
		GensTrace_hook();

	CallRegisteredLuaMemHook(hook_pc, 2, 0, LUAMEMHOOK_EXEC);
}

static void trace_read_byte_internal()
{
	unsigned int start, stop;

	start = hook_address;
	stop = start + 0;

	for( int lcv = 0; lcv < STATES; lcv++ )
	{
		FILE *out;

		// linear map
		if( rd_mode[ lcv ] & 1 )
		{
			// fail: outside boundaries
			if( stop < rd_low[ lcv ] ) continue;
			if( start > rd_high[ lcv ] ) continue;
		}
		// shadow map
		else
		{
			// fail: outside boundaries
			if( ( stop & 0xffff ) < ( rd_low[ lcv ] & 0xffff ) ) continue;
			if( ( start & 0xffff ) > ( rd_high[ lcv ] & 0xffff ) ) continue;
			
			if( ( stop >> 16 ) < ( rd_low[ lcv ] >> 16 ) ) continue;
			if( ( start >> 16 ) > ( rd_high[ lcv ] >> 16 ) ) continue;
		}

// ------------------------------------------------------

		// auto-trace
		if( rd_mode[ lcv ] >= 4 && rd_mode[ lcv ] < 8 )
		{
			rd_mode[ lcv ] &= 3;
			//trace_map = 1;

			hook_trace = 0;
			GensTrace();
			hook_trace = 1;
		}

		// output file mode
		out = ( rd_mode[ lcv ] <= 1 ) ? fp_hook : fp_trace;

		if( !out ) continue;
		if( out == fp_trace )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s = %02X [%06X]\n",
			hook_pc >> 16, hook_pc & 0xffff, "R08",
			hook_value & 0xff, hook_address );
	}
}


void trace_read_byte()
{
	if( hook_trace && rd_mode )
		trace_read_byte_internal();
	CallRegisteredLuaMemHook(hook_address, 1, hook_value, LUAMEMHOOK_READ);
}

static void trace_read_word_internal()
{
	unsigned int start, stop;

	start = hook_address;
	stop = start + 1;

	for( int lcv = 0; lcv < STATES; lcv++ )
	{
		FILE *out;

		// linear map
		if( rd_mode[ lcv ] & 1 )
		{
			// fail: outside boundaries
			if( stop < rd_low[ lcv ] ) continue;
			if( start > rd_high[ lcv ] ) continue;
		}
		// shadow map
		else
		{
			// fail: outside boundaries
			if( ( stop & 0xffff ) < ( rd_low[ lcv ] & 0xffff ) ) continue;
			if( ( start & 0xffff ) > ( rd_high[ lcv ] & 0xffff ) ) continue;
			
			if( ( stop >> 16 ) < ( rd_low[ lcv ] >> 16 ) ) continue;
			if( ( start >> 16 ) > ( rd_high[ lcv ] >> 16 ) ) continue;
		}

// ------------------------------------------------------

		// auto-trace
		if( rd_mode[ lcv ] >= 4 && rd_mode[ lcv ] < 8 )
		{
			rd_mode[ lcv ] &= 3;
			//trace_map = 1;

			hook_trace = 0;
			GensTrace();
			hook_trace = 1;
		}

		// output file mode
		out = ( rd_mode[ lcv ] <= 1 ) ? fp_hook : fp_trace;

		if( !out ) continue;
		if( out == fp_trace )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s = %04X [%06X]\n",
			hook_pc >> 16, hook_pc & 0xffff, "R16",
			hook_value & 0xffff, hook_address );
	}
}
void trace_read_word()
{
	if( hook_trace && rd_mode )
		trace_read_word_internal();
	CallRegisteredLuaMemHook(hook_address, 2, hook_value, LUAMEMHOOK_READ);
}


static void trace_read_dword_internal()
{
	unsigned int start, stop;

	start = hook_address;
	stop = start + 3;

	for( int lcv = 0; lcv < STATES; lcv++ )
	{
		FILE *out;

		// linear map
		if( rd_mode[ lcv ] & 1 )
		{
			// fail: outside boundaries
			if( stop < rd_low[ lcv ] ) continue;
			if( start > rd_high[ lcv ] ) continue;
		}
		// shadow map
		else
		{
			// fail: outside boundaries
			if( ( stop & 0xffff ) < ( rd_low[ lcv ] & 0xffff ) ) continue;
			if( ( start & 0xffff ) > ( rd_high[ lcv ] & 0xffff ) ) continue;
			
			if( ( stop >> 16 ) < ( rd_low[ lcv ] >> 16 ) ) continue;
			if( ( start >> 16 ) > ( rd_high[ lcv ] >> 16 ) ) continue;
		}

// ------------------------------------------------------

		// auto-trace
		if( rd_mode[ lcv ] >= 4 && rd_mode[ lcv ] < 8 )
		{
			rd_mode[ lcv ] &= 3;
			//trace_map = 1;

			hook_trace = 0;
			GensTrace();
			hook_trace = 1;
		}

		// output file mode
		out = ( rd_mode[ lcv ] <= 1 ) ? fp_hook : fp_trace;

		if( !out ) continue;
		if( out == fp_trace )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s = %08X [%06X]\n",
			hook_pc >> 16, hook_pc & 0xffff, "R32",
			hook_value & 0xffffffff, hook_address );
	}
}
void trace_read_dword()
{
	if( hook_trace && rd_mode )
		trace_read_dword_internal();
	CallRegisteredLuaMemHook(hook_address, 4, hook_value, LUAMEMHOOK_READ);
}


static void trace_write_byte_internal()
{
	unsigned int start, stop;

	if (hook_address >= 0x00e00000) hook_address |= 0x00ff0000;

	start = hook_address;
	stop = start + 0;

	for( int lcv = 0; lcv < STATES; lcv++ )
	{
		FILE *out;

		// linear map
		if( wr_mode[ lcv ] & 1 )
		{
			// fail: outside boundaries
			if( stop < wr_low[ lcv ] ) continue;
			if( start > wr_high[ lcv ] ) continue;
		}
		// shadow map
		else
		{
			// fail: outside boundaries
			if( ( stop & 0xffff ) < ( wr_low[ lcv ] & 0xffff ) ) continue;
			if( ( start & 0xffff ) > ( wr_high[ lcv ] & 0xffff ) ) continue;
			
			if( ( stop >> 16 ) < ( wr_low[ lcv ] >> 16 ) ) continue;
			if( ( start >> 16 ) > ( wr_high[ lcv ] >> 16 ) ) continue;
		}

// ------------------------------------------------------

		// auto-trace
		if( wr_mode[ lcv ] >= 4 && wr_mode[ lcv ] < 8 )
		{
			wr_mode[ lcv ] &= 3;
			//trace_map = 1;

			hook_trace = 0;
			GensTrace();
			hook_trace = 1;
		}

		// output file mode
		out = ( wr_mode[ lcv ] <= 1 ) ? fp_hook : fp_trace;

		if( !out ) continue;
		if( out == fp_trace )
			fprintf( out, "** " );

		if (hook_pc)
			fprintf( out, "[%02X:%04X] %s = %02X [%06X]\n",
				hook_pc >> 16, hook_pc & 0xffff, "W08",
				hook_value & 0xff, hook_address );
		else
			fprintf( out, "[GGWRITE] %s = %02X [%06X]\n",
				"W08", hook_value & 0xff, hook_address );

	}
}
void trace_write_byte()
{
	if( hook_trace && wr_mode )
		trace_write_byte_internal();
	CallRegisteredLuaMemHook(hook_address, 1, hook_value, LUAMEMHOOK_WRITE);
}


static void trace_write_word_internal()
{
	unsigned int start, stop;

	start = hook_address;
	stop = start + 1;

	for( int lcv = 0; lcv < STATES; lcv++ )
	{
		FILE *out;

		// linear map
		if( wr_mode[ lcv ] & 1 )
		{
			// fail: outside boundaries
			if( stop < wr_low[ lcv ] ) continue;
			if( start > wr_high[ lcv ] ) continue;
		}
		// shadow map
		else
		{
			// fail: outside boundaries
			if( ( stop & 0xffff ) < ( wr_low[ lcv ] & 0xffff ) ) continue;
			if( ( start & 0xffff ) > ( wr_high[ lcv ] & 0xffff ) ) continue;
			
			if( ( stop >> 16 ) < ( wr_low[ lcv ] >> 16 ) ) continue;
			if( ( start >> 16 ) > ( wr_high[ lcv ] >> 16 ) ) continue;
		}

// ------------------------------------------------------

		// auto-trace
		if( wr_mode[ lcv ] >= 4 && wr_mode[ lcv ] < 8 )
		{
			wr_mode[ lcv ] &= 3;
			//trace_map = 1;

			hook_trace = 0;
			GensTrace();
			hook_trace = 1;
		}

		// output file mode
		out = ( wr_mode[ lcv ] <= 1 ) ? fp_hook : fp_trace;

		if( !out ) continue;
		if( out == fp_trace )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s = %04X [%06X]\n",
			hook_pc >> 16, hook_pc & 0xffff, "W16",
			hook_value & 0xffff, hook_address );
	}
}
void trace_write_word()
{
	if( hook_trace && wr_mode )
		trace_write_word_internal();
	CallRegisteredLuaMemHook(hook_address, 2, hook_value, LUAMEMHOOK_WRITE);
}


static void trace_write_dword_internal()
{
	unsigned int start, stop;

	start = hook_address;
	stop = start + 3;

	for( int lcv = 0; lcv < STATES; lcv++ )
	{
		FILE *out;

		// linear map
		if( wr_mode[ lcv ] & 1 )
		{
			// fail: outside boundaries
			if( stop < wr_low[ lcv ] ) continue;
			if( start > wr_high[ lcv ] ) continue;
		}
		// shadow map
		else
		{
			// fail: outside boundaries
			if( ( stop & 0xffff ) < ( wr_low[ lcv ] & 0xffff ) ) continue;
			if( ( start & 0xffff ) > ( wr_high[ lcv ] & 0xffff ) ) continue;
			
			if( ( stop >> 16 ) < ( wr_low[ lcv ] >> 16 ) ) continue;
			if( ( start >> 16 ) > ( wr_high[ lcv ] >> 16 ) ) continue;
		}

// ------------------------------------------------------

		// auto-trace
		if( wr_mode[ lcv ] >= 4 && wr_mode[ lcv ] < 8 )
		{
			wr_mode[ lcv ] &= 3;
			//trace_map = 1;

			hook_trace = 0;
			GensTrace();
			hook_trace = 1;
		}

		// output file mode
		out = ( wr_mode[ lcv ] <= 1 ) ? fp_hook : fp_trace;

		if( !out ) continue;
		if( out == fp_trace )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s = %08X [%06X]\n",
			hook_pc >> 16, hook_pc & 0xffff, "W32",
			hook_value & 0xffffffff, hook_address );
	}
}
void trace_write_dword()
{
	if( hook_trace && wr_mode )
		trace_write_dword_internal();
	CallRegisteredLuaMemHook(hook_address, 4, hook_value, LUAMEMHOOK_WRITE);
}


static void hook_dma_internal()
{
	unsigned int start, stop;
	int lcv;

	// VDP area
	hook_value &= 3;

	// Memory breakpoints
	start = VDP_Reg.DMA_Address << 1;
	stop = start + ( VDP_Reg.DMA_Length << 1 ) - 1;

	for( lcv = 0; lcv < STATES; lcv++ )
	{
		FILE *out;

		// linear map
		if( rd_mode[ lcv ] & 1 )
		{
			// fail: outside boundaries
			if( stop < rd_low[ lcv ] ) continue;
			if( start > rd_high[ lcv ] ) continue;
		}
		// shadow map
		else
		{
			// fail: outside boundaries
			if( ( stop & 0xffff ) < ( rd_low[ lcv ] & 0xffff ) ) continue;
			if( ( start & 0xffff ) > ( rd_high[ lcv ] & 0xffff ) ) continue;
			
			if( ( stop >> 16 ) < ( rd_low[ lcv ] >> 16 ) ) continue;
			if( ( start >> 16 ) > ( rd_high[ lcv ] >> 16 ) ) continue;
		}

// ------------------------------------------------------

		// auto-trace
		if( rd_mode[ lcv ] >= 4 && rd_mode[ lcv ] < 8 )
		{
			rd_mode[ lcv ] &= 3;
			//trace_map = 1;

			hook_trace = 0;
			GensTrace();
			hook_trace = 1;
		}

		// output file mode
		out = ( rd_mode[ lcv ] <= 1 ) ? fp_hook : fp_trace;

		if( !out ) continue;
		if( out == fp_trace )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s DMA from %06X to %04X [%04X]\n",
			hook_pc >> 16, hook_pc & 0xffff,
			( hook_value <= 1 ) ? "VRAM" : ( hook_value == 2 ) ? "CRAM" : "VSRAM",
			start & 0xffffff,
			Ctrl.Address & 0xffff,
			( VDP_Reg.DMA_Length << 1 ) & 0xffff );

		fprintf( out, "- SRC_H = [%02X:%04X]  ~~  LEN_H = [%02X:%04X]\n",
			dma_src >> 16, dma_src & 0xffff,
			dma_len >> 16, dma_len & 0xffff );

		fprintf( out, "\n" );
	}

	/**************************************************/
	/**************************************************/

	unsigned int start_l, stop_l;

	start = Ctrl.Address;
	stop = start + ( VDP_Reg.DMA_Length << 1 ) - 1;

	// local linear
	if( hook_value <= 1 )
	{
		// VRAM
		start_l = start + 0x00000;
		stop_l = stop + 0x00000;
	}
	else if( hook_value == 2 )
	{
		// CRAM
		start_l = start + 0x10000;
		stop_l = stop + 0x10000;
	}
	else if( hook_value == 3 )
	{
		// VSRAM
		start_l = start + 0x20000;
		stop_l = stop + 0x20000;
	}
	else
	{
		// Error
		return;
	}


	// breakpoints
	for( lcv = 0; lcv < 3; lcv++ )
	{
		FILE *out;

		// VDP memory only
		if( ( ppu_mode[ lcv ] & 1 ) != 1 ) continue;

		// fail case: outside range
		if( stop_l < ppu_low[ lcv ] ) continue;
		if( start_l > ppu_high[ lcv ] ) continue;

// ------------------------------------------------------

		// auto-trace
		if( ppu_mode[ lcv ] >= 4 && ppu_mode[ lcv ] < 8 )
		{
			ppu_mode[ lcv ] &= 3;
			//trace_map = 1;

			hook_trace = 0;
			GensTrace();
			hook_trace = 1;
		}

		// output file mode
		out = ( ppu_mode[ lcv ] <= 1 ) ? fp_hook : fp_trace;

		if( !out ) continue;
		if( out == fp_trace )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s DMA from %06X to %04X [%04X]\n",
			hook_pc >> 16, hook_pc & 0xffff,
			( hook_value <= 1 ) ? "VRAM" : ( hook_value <= 2 ) ? "CRAM" : "VSRAM",
			( VDP_Reg.DMA_Address << 1 ) & 0xffffff,
			start & 0xffff, ( VDP_Reg.DMA_Length << 1 ) & 0xffff );

		fprintf( out, "- SRC_H = $%02X:%04X  ~~  LEN_H = $%02X:%04X\n",
			dma_src >> 16, dma_src & 0xffff,
			dma_len >> 16, dma_len & 0xffff );

		fprintf( out, "\n" );
	}
}
void hook_dma()
{
	if( hook_trace && rd_mode )
		hook_dma_internal();
}


static void trace_write_vram_byte_internal()
{
	unsigned int start, stop;
	unsigned int start_l, stop_l;

	hook_pc &= 0x00ffffff;
	hook_address &= 0x00ffffff;

	start = Ctrl.Address;
	stop = start + 0;

	// local linear
	if( Ctrl.Access == 9 )
	{
		// VRAM
		start_l = start + 0x00000;
		stop_l = stop + 0x00000;
	}
	else if( Ctrl.Access == 10 )
	{
		// CRAM
		start_l = start + 0x10000;
		stop_l = stop + 0x10000;
	}
	else if( Ctrl.Access == 11 )
	{
		// VSRAM
		start_l = start + 0x20000;
		stop_l = stop + 0x20000;
	}
	else
	{
		// Error
		return;
	}


	// breakpoints
	for( int lcv = 0; lcv < 3; lcv++ )
	{
		FILE *out;

		// VRAM only
		if( ( ppu_mode[ lcv ] & 1 ) != 1 ) continue;

		// fail case: outside range
		if( stop_l < ppu_low[ lcv ] ) continue;
		if( start_l > ppu_high[ lcv ] ) continue;

// ------------------------------------------------------

		// auto-trace
		if( ppu_mode[ lcv ] >= 4 && ppu_mode[ lcv ] < 8 )
		{
			ppu_mode[ lcv ] &= 3;
			//trace_map = 1;

			hook_trace = 0;
			GensTrace();
			hook_trace = 1;
		}

		// output file mode
		out = ( ppu_mode[ lcv ] <= 1 ) ? fp_hook : fp_trace;

		if( !out ) continue;
		if( out == fp_trace )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s W08 = %02X [%04X]\n",
			hook_pc >> 16, hook_pc & 0xffff,
			( Ctrl.Access == 9 ) ? "VRAM" : ( Ctrl.Access == 10 ) ? "CRAM" : "VSRAM",
			hook_value & 0xff, Ctrl.Address & 0xffff );
	}
}
void trace_write_vram_byte()
{
	if( hook_trace && ppu_mode )
		trace_write_vram_byte_internal();
}


static void trace_write_vram_word_internal()
{
	unsigned int start, stop;
	unsigned int start_l, stop_l;

	hook_pc &= 0x00ffffff;
	hook_address &= 0x00ffffff;

	start = Ctrl.Address;
	stop = start + 1;

	// local linear
	if( Ctrl.Access == 9 )
	{
		// VRAM
		start_l = start + 0x00000;
		stop_l = stop + 0x00000;
	}
	else if( Ctrl.Access == 10 )
	{
		// CRAM
		start_l = start + 0x10000;
		stop_l = stop + 0x10000;
	}
	else if( Ctrl.Access == 11 )
	{
		// VSRAM
		start_l = start + 0x20000;
		stop_l = stop + 0x20000;
	}
	else
	{
		// Error
		return;
	}


	// breakpoints
	for( int lcv = 0; lcv < 3; lcv++ )
	{
		FILE *out;

		// VRAM only
		if( ( ppu_mode[ lcv ] & 1 ) != 1 ) continue;

		// fail case: outside range
		if( stop_l < ppu_low[ lcv ] ) continue;
		if( start_l > ppu_high[ lcv ] ) continue;

// ------------------------------------------------------

		// auto-trace
		if( ppu_mode[ lcv ] >= 4 && ppu_mode[ lcv ] < 8 )
		{
			ppu_mode[ lcv ] &= 3;
			//trace_map = 1;

			hook_trace = 0;
			GensTrace();
			hook_trace = 1;
		}

		// output file mode
		out = ( ppu_mode[ lcv ] <= 1 ) ? fp_hook : fp_trace;

		if( !out ) continue;
		if( out == fp_trace )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s W16 = %04X [%04X]\n",
			hook_pc >> 16, hook_pc & 0xffff,
			( Ctrl.Access == 9 ) ? "VRAM" : ( Ctrl.Access == 10 ) ? "CRAM" : "VSRAM",
			hook_value & 0xffff, Ctrl.Address & 0xffff );
	}
}
void trace_write_vram_word()
{
	if( hook_trace && ppu_mode )
		trace_write_vram_word_internal();
}


static void trace_read_vram_byte_internal()
{
	unsigned int start, stop;
	unsigned int start_l, stop_l;

	hook_pc &= 0x00ffffff;
	hook_address &= 0x00ffffff;

	start = Ctrl.Address;
	stop = start + 0;

	// local linear
	if( Ctrl.Access == 5 )
	{
		// VRAM
		start_l = start + 0x00000;
		stop_l = stop + 0x00000;
	}
	else if( Ctrl.Access == 6 )
	{
		// CRAM
		start_l = start + 0x10000;
		stop_l = stop + 0x10000;
	}
	else if( Ctrl.Access == 7 )
	{
		// VSRAM
		start_l = start + 0x20000;
		stop_l = stop + 0x20000;
	}
	else
	{
		// Error
		return;
	}


	// breakpoints
	for( int lcv = 0; lcv < 3; lcv++ )
	{
		FILE *out;

		// VRAM only
		if( ( ppu_mode[ lcv ] & 1 ) != 1 ) continue;

		// fail case: outside range
		if( stop_l < ppu_low[ lcv ] ) continue;
		if( start_l > ppu_high[ lcv ] ) continue;

// ------------------------------------------------------

		// auto-trace
		if( ppu_mode[ lcv ] >= 4 && ppu_mode[ lcv ] < 8 )
		{
			ppu_mode[ lcv ] &= 3;
			//trace_map = 1;

			hook_trace = 0;
			GensTrace();
			hook_trace = 1;
		}

		// output file mode
		out = ( ppu_mode[ lcv ] <= 1 ) ? fp_hook : fp_trace;

		if( !out ) continue;
		if( out == fp_trace )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s R08 = %02X [%04X]\n",
			hook_pc >> 16, hook_pc & 0xffff,
			( Ctrl.Access == 5 ) ? "VRAM" : ( Ctrl.Access == 6 ) ? "CRAM" : "VSRAM",
			hook_value & 0xff, Ctrl.Address & 0xffff );
	}
}
void trace_read_vram_byte()
{
	if( hook_trace && ppu_mode )
		trace_read_vram_byte_internal();
}


static void trace_read_vram_word_internal()
{
	unsigned int start, stop;
	unsigned int start_l, stop_l;

	hook_pc &= 0x00ffffff;
	hook_address &= 0x00ffffff;

	start = Ctrl.Address;
	stop = start + 1;

	// local linear
	if( Ctrl.Access == 5 )
	{
		// VRAM
		start_l = start + 0x00000;
		stop_l = stop + 0x00000;
	}
	else if( Ctrl.Access == 6 )
	{
		// CRAM
		start_l = start + 0x10000;
		stop_l = stop + 0x10000;
	}
	else if( Ctrl.Access == 7 )
	{
		// VSRAM
		start_l = start + 0x20000;
		stop_l = stop + 0x20000;
	}
	else
	{
		// Error
		return;
	}


	// breakpoints
	for( int lcv = 0; lcv < 3; lcv++ )
	{
		FILE *out;

		// VRAM only
		if( ( ppu_mode[ lcv ] & 1 ) != 1 ) continue;

		// fail case: outside range
		if( stop_l < ppu_low[ lcv ] ) continue;
		if( start_l > ppu_high[ lcv ] ) continue;

// ------------------------------------------------------

		// auto-trace
		if( ppu_mode[ lcv ] >= 4 && ppu_mode[ lcv ] < 8 )
		{
			ppu_mode[ lcv ] &= 3;
			//trace_map = 1;

			hook_trace = 0;
			GensTrace();
			hook_trace = 1;
		}

		// output file mode
		out = ( ppu_mode[ lcv ] <= 1 ) ? fp_hook : fp_trace;

		if( !out ) continue;
		if( out == fp_trace )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s R16 = %04X [%04X]\n",
			hook_pc >> 16, hook_pc & 0xffff,
			( Ctrl.Access == 5 ) ? "VRAM" : ( Ctrl.Access == 6 ) ? "CRAM" : "VSRAM",
			hook_value & 0xffff, Ctrl.Address & 0xffff );
	}
}
void trace_read_vram_word()
{
	if( hook_trace && ppu_mode )
		trace_read_vram_word_internal();
}


static void hook_vdp_reg_internal()
{
	unsigned int start, stop;

	hook_pc &= 0x00ffffff;
	hook_address &= 0x00ffffff;

	start = hook_address;
	stop = start + 0;

	// VRAM breakpoints
	for( int lcv = 0; lcv < 3; lcv++ )
	{
		FILE *out;

		// VDP registers only
		if( ( ppu_mode[ lcv ] & 1 ) != 0 ) continue;

		// fail case: outside range
		if( stop < ppu_low[ lcv ] ) continue;
		if( start > ppu_high[ lcv ] ) continue;

// ------------------------------------------------------

		// auto-trace
		if( ppu_mode[ lcv ] >= 4 && ppu_mode[ lcv ] < 8 )
		{
			ppu_mode[ lcv ] &= 3;
			//trace_map = 1;

			hook_trace = 0;
			GensTrace();
			hook_trace = 1;
		}

		// output file mode
		out = ( ppu_mode[ lcv ] <= 1 ) ? fp_hook : fp_trace;

		if( !out ) continue;
		if( out == fp_trace )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] VDP REG W08 = %02X [%02X]\n",
			hook_pc >> 16, hook_pc & 0xffff,
			hook_value & 0xff, hook_address & 0xff );
	}
}
void hook_vdp_reg()
{
	if( hook_trace && ppu_mode )
		hook_vdp_reg_internal();
}
