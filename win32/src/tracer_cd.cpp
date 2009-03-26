#include <stdio.h>
#include <memory.h>

#include "Cpu_68k.h"
#include "M68KD.h"
#include "Mem_M68k.h"
#include "Mem_S68k.h"
#include "vdp_io.h"
#include "luascript.h"

#define uint32 unsigned int

extern FILE* fp_trace_cd;
extern FILE* fp_call_cd;
extern FILE* fp_hook_cd;

#define STATES 3
extern unsigned int *rd_mode_cd, *wr_mode_cd, *ppu_mode_cd, *pc_mode_cd;
extern unsigned int *rd_low_cd, *rd_high_cd;
extern unsigned int *wr_low_cd, *wr_high_cd;
extern unsigned int *ppu_low_cd, *ppu_high_cd;
extern unsigned int *pc_low_cd, *pc_high_cd;
extern unsigned int *pc_start_cd;

extern bool trace_map;
extern bool hook_trace;

extern "C" {
	extern uint32 hook_address_cd;
	extern uint32 hook_value_cd;
	extern uint32 hook_pc_cd;
	
	void trace_read_byte_cd();
	void trace_read_word_cd();
	void trace_read_dword_cd();
	void trace_write_byte_cd();
	void trace_write_word_cd();
	void trace_write_dword_cd();

	void GensTrace_cd();
};

extern char *mapped_cd;
uint32 Current_PC_cd;
int Debug_CD = 2;


void DeInitDebug_cd()
{
	if( fp_hook_cd )
	{
		fclose( fp_hook_cd );
	}

	if( rd_mode_cd )
	{
		delete[] rd_mode_cd;
		delete[] wr_mode_cd;
		delete[] pc_mode_cd;
		delete[] ppu_mode_cd;

		delete[] rd_low_cd;
		delete[] wr_low_cd;
		delete[] pc_low_cd;
		delete[] ppu_low_cd;

		delete[] rd_high_cd;
		delete[] wr_high_cd;
		delete[] pc_high_cd;
		delete[] ppu_high_cd;

		delete[] pc_start_cd;
	}

	if( fp_trace_cd )
	{
		delete[] mapped_cd;
		fclose( fp_trace_cd );
	}
}

unsigned short Next_Word_T_cd(void)
{
	unsigned short val;
	
	if (Debug_CD == 1) val = M68K_RW(Current_PC_cd);
	else if (Debug_CD >= 2) val = S68K_RW(Current_PC_cd);

	Current_PC_cd += 2;

	return(val);
}

unsigned int Next_Long_T_cd(void)
{
	unsigned int val;
	
	if (Debug_CD == 1)
	{
		val = M68K_RW(Current_PC_cd);
		val <<= 16;
		val |= M68K_RW(Current_PC_cd + 2);
	}
	else if (Debug_CD >= 2)
	{
		val = S68K_RW(Current_PC_cd);
		val <<= 16;
		val |= S68K_RW(Current_PC_cd + 2);
	}

	Current_PC_cd += 4;

	return(val);
}


void Print_Instruction_cd( FILE *trace )
{
	char String [512];
	Current_PC_cd = hook_pc_cd;

	int PC;
	int OPC = S68K_RW(Current_PC_cd);

	PC = Current_PC_cd;
	sprintf( String, "%02X:%04X  %02X %02X  %-33s",
		PC >> 16, PC & 0xffff, OPC >> 8, OPC & 0xff,
		M68KDisasm2( Next_Word_T_cd, Next_Long_T_cd, hook_pc_cd ) );
	fprintf( trace, "%s", String );


	sprintf( String, "A0=%.8X A1=%.8X A2=%.8X ", sub68k_context.areg[0], sub68k_context.areg[1], sub68k_context.areg[2]);
	fprintf( trace, "%s", String );

	sprintf( String, "A3=%.8X A4=%.8X A5=%.8X ", sub68k_context.areg[3], sub68k_context.areg[4], sub68k_context.areg[5]);
	fprintf( trace, "%s", String );

	sprintf( String, "A6=%.8X A7=%.8X D0=%.8X ", sub68k_context.areg[6], sub68k_context.areg[7], sub68k_context.dreg[0]);
	fprintf( trace, "%s", String );

	sprintf( String, " ~~  " );

	sprintf( String, "D1=%.8X D2=%.8X D3=%.8X ", sub68k_context.dreg[1], sub68k_context.dreg[2], sub68k_context.dreg[3]);
	fprintf( trace, "%s", String );

	sprintf( String, "D4=%.8X D5=%.8X D6=%.8X ", sub68k_context.dreg[4], sub68k_context.dreg[5], sub68k_context.dreg[6]);
	fprintf( trace, "%s", String );

	sprintf( String, "D7=%.8X ", sub68k_context.dreg[7]);
	fprintf( trace, "%s", String );

	fprintf( trace, "%c", (sub68k_context.sr & 0x10)?'X':'x' );
	fprintf( trace, "%c", (sub68k_context.sr & 0x08)?'N':'n' );
	fprintf( trace, "%c", (sub68k_context.sr & 0x04)?'Z':'z' );
	fprintf( trace, "%c", (sub68k_context.sr & 0x02)?'V':'v' );
	fprintf( trace, "%c", (sub68k_context.sr & 0x01)?'C':'c' );

	fprintf( trace, "\n", String );
}

static void GensTrace_cd_trace()
{
	if(mapped_cd[ hook_pc_cd ] < 0x40)
	{
		Print_Instruction_cd( fp_trace_cd );
		mapped_cd[ hook_pc_cd ] ++;
	}
}

static void GensTrace_cd_hook()
{
	for( int lcv = 0; lcv < STATES; lcv++ )
	{
		FILE *out;

		// start-stop
		if( pc_mode_cd[ lcv ] & 1 )
		{
			if( hook_pc_cd == pc_low_cd[lcv] )
				pc_start_cd[lcv] = 1;

			if( !pc_start_cd[lcv] ) continue;

			out = fp_hook_cd;
		}
		// low-high
		else
		{
			// fail: outside boundaries
			if( hook_pc_cd < pc_low_cd[ lcv ] ) continue;
			if( hook_pc_cd > pc_high_cd[ lcv ] ) continue;
		}

// ------------------------------------------------------

		if( pc_mode_cd[ lcv ] >= 4 && pc_mode_cd[ lcv ] < 8 )
		{
			pc_mode_cd[ lcv ] &= 3;
			trace_map = 1;
		}

		// output file mode
		out = ( pc_mode_cd[ lcv ] <= 1 ) ? fp_hook_cd : fp_trace_cd;

		if( !out ) continue;
		if( out == fp_trace_cd )
			fprintf( out, "** " );

		Print_Instruction_cd( out );


		// end formatting
		if( hook_pc_cd == pc_high_cd[lcv] )
		{
			pc_start_cd[lcv] = 0;

			if( pc_low_cd[ lcv ] != pc_high_cd[ lcv ] )
				fprintf( out, "\n" );
		}
	} // end STATES
}

void GensTrace_cd()
{
	// Trace.txt
	if( trace_map )
		GensTrace_cd_trace();

	// Hook.txt
	if( hook_trace )
		GensTrace_cd_hook();

	CallRegisteredLuaMemHook(hook_pc_cd, 2, 0, LUAMEMHOOK_EXEC_SUB);
}


static void trace_read_byte_cd_internal()
{
	unsigned int start, stop;

	start = hook_address_cd;
	stop = start + 0;

	for( int lcv = 0; lcv < STATES; lcv++ )
	{
		FILE *out;

		// linear map
		if( rd_mode_cd[ lcv ] & 1 )
		{
			// fail: outside boundaries
			if( stop < rd_low_cd[ lcv ] ) continue;
			if( start > rd_high_cd[ lcv ] ) continue;
		}
		// shadow map
		else
		{
			// fail: outside boundaries
			if( ( stop & 0xffff ) < ( rd_low_cd[ lcv ] & 0xffff ) ) continue;
			if( ( start & 0xffff ) > ( rd_high_cd[ lcv ] & 0xffff ) ) continue;
			
			if( ( stop >> 16 ) < ( rd_low_cd[ lcv ] >> 16 ) ) continue;
			if( ( start >> 16 ) > ( rd_high_cd[ lcv ] >> 16 ) ) continue;
		}

// ------------------------------------------------------

		// auto-trace
		if( rd_mode_cd[ lcv ] >= 4 && rd_mode_cd[ lcv ] < 8 )
		{
			rd_mode_cd[ lcv ] &= 3;
			trace_map = 1;

			hook_trace = 0;
			GensTrace_cd();
			hook_trace = 1;
		}

		// output file mode
		out = ( rd_mode_cd[ lcv ] <= 1 ) ? fp_hook_cd : fp_trace_cd;

		if( !out ) continue;
		if( out == fp_trace_cd )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s = %02X [%06X]\n",
			hook_pc_cd >> 16, hook_pc_cd & 0xffff, "R08",
			hook_value_cd & 0xff, hook_address_cd );
	}
}
void trace_read_byte_cd()
{
	if( hook_trace )
		trace_read_byte_cd_internal();
	CallRegisteredLuaMemHook(hook_address_cd, 1, hook_value_cd, LUAMEMHOOK_READ_SUB);
}


static void trace_read_word_cd_internal()
{
	unsigned int start, stop;

	start = hook_address_cd;
	stop = start + 1;

	for( int lcv = 0; lcv < STATES; lcv++ )
	{
		FILE *out;

		// linear map
		if( rd_mode_cd[ lcv ] & 1 )
		{
			// fail: outside boundaries
			if( stop < rd_low_cd[ lcv ] ) continue;
			if( start > rd_high_cd[ lcv ] ) continue;
		}
		// shadow map
		else
		{
			// fail: outside boundaries
			if( ( stop & 0xffff ) < ( rd_low_cd[ lcv ] & 0xffff ) ) continue;
			if( ( start & 0xffff ) > ( rd_high_cd[ lcv ] & 0xffff ) ) continue;
			
			if( ( stop >> 16 ) < ( rd_low_cd[ lcv ] >> 16 ) ) continue;
			if( ( start >> 16 ) > ( rd_high_cd[ lcv ] >> 16 ) ) continue;
		}

// ------------------------------------------------------

		// auto-trace
		if( rd_mode_cd[ lcv ] >= 4 && rd_mode_cd[ lcv ] < 8 )
		{
			rd_mode_cd[ lcv ] &= 3;
			trace_map = 1;

			hook_trace = 0;
			GensTrace_cd();
			hook_trace = 1;
		}

		// output file mode
		out = ( rd_mode_cd[ lcv ] <= 1 ) ? fp_hook_cd : fp_trace_cd;

		if( !out ) continue;
		if( out == fp_trace_cd )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s = %04X [%06X]\n",
			hook_pc_cd >> 16, hook_pc_cd & 0xffff, "R16",
			hook_value_cd & 0xffff, hook_address_cd );
	}
}
void trace_read_word_cd()
{
	if( hook_trace )
		trace_read_word_cd_internal();
	CallRegisteredLuaMemHook(hook_address_cd, 2, hook_value_cd, LUAMEMHOOK_READ_SUB);
}


static void trace_read_dword_cd_internal()
{
	unsigned int start, stop;

	start = hook_address_cd;
	stop = start + 3;

	for( int lcv = 0; lcv < STATES; lcv++ )
	{
		FILE *out;

		// linear map
		if( rd_mode_cd[ lcv ] & 1 )
		{
			// fail: outside boundaries
			if( stop < rd_low_cd[ lcv ] ) continue;
			if( start > rd_high_cd[ lcv ] ) continue;
		}
		// shadow map
		else
		{
			// fail: outside boundaries
			if( ( stop & 0xffff ) < ( rd_low_cd[ lcv ] & 0xffff ) ) continue;
			if( ( start & 0xffff ) > ( rd_high_cd[ lcv ] & 0xffff ) ) continue;
			
			if( ( stop >> 16 ) < ( rd_low_cd[ lcv ] >> 16 ) ) continue;
			if( ( start >> 16 ) > ( rd_high_cd[ lcv ] >> 16 ) ) continue;
		}

// ------------------------------------------------------

		// auto-trace
		if( rd_mode_cd[ lcv ] >= 4 && rd_mode_cd[ lcv ] < 8 )
		{
			rd_mode_cd[ lcv ] &= 3;
			trace_map = 1;

			hook_trace = 0;
			GensTrace_cd();
			hook_trace = 1;
		}

		// output file mode
		out = ( rd_mode_cd[ lcv ] <= 1 ) ? fp_hook_cd : fp_trace_cd;

		if( !out ) continue;
		if( out == fp_trace_cd )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s = %08X [%06X]\n",
			hook_pc_cd >> 16, hook_pc_cd & 0xffff, "R32",
			hook_value_cd & 0xffffffff, hook_address_cd );
	}
}
void trace_read_dword_cd()
{
	if( hook_trace )
		trace_read_dword_cd_internal();
	CallRegisteredLuaMemHook(hook_address_cd, 4, hook_value_cd, LUAMEMHOOK_READ_SUB);
}


static void trace_write_byte_cd_internal()
{
	unsigned int start, stop;

	start = hook_address_cd;
	stop = start + 0;

	for( int lcv = 0; lcv < STATES; lcv++ )
	{
		FILE *out;

		// linear map
		if( wr_mode_cd[ lcv ] & 1 )
		{
			// fail: outside boundaries
			if( stop < wr_low_cd[ lcv ] ) continue;
			if( start > wr_high_cd[ lcv ] ) continue;
		}
		// shadow map
		else
		{
			// fail: outside boundaries
			if( ( stop & 0xffff ) < ( wr_low_cd[ lcv ] & 0xffff ) ) continue;
			if( ( start & 0xffff ) > ( wr_high_cd[ lcv ] & 0xffff ) ) continue;
			
			if( ( stop >> 16 ) < ( wr_low_cd[ lcv ] >> 16 ) ) continue;
			if( ( start >> 16 ) > ( wr_high_cd[ lcv ] >> 16 ) ) continue;
		}

// ------------------------------------------------------

		// auto-trace
		if( wr_mode_cd[ lcv ] >= 4 && wr_mode_cd[ lcv ] < 8 )
		{
			wr_mode_cd[ lcv ] &= 3;
			trace_map = 1;

			hook_trace = 0;
			GensTrace_cd();
			hook_trace = 1;
		}

		// output file mode
		out = ( wr_mode_cd[ lcv ] <= 1 ) ? fp_hook_cd : fp_trace_cd;

		if( !out ) continue;
		if( out == fp_trace_cd )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s = %02X [%06X]\n",
			hook_pc_cd >> 16, hook_pc_cd & 0xffff, "W08",
			hook_value_cd & 0xff, hook_address_cd );
	}
}
void trace_write_byte_cd()
{
	if( hook_trace )
		trace_write_byte_cd_internal();
	CallRegisteredLuaMemHook(hook_address_cd, 1, hook_value_cd, LUAMEMHOOK_WRITE_SUB);
}


static void trace_write_word_cd_internal()
{
	unsigned int start, stop;

	start = hook_address_cd;
	stop = start + 1;

	for( int lcv = 0; lcv < STATES; lcv++ )
	{
		FILE *out;

		// linear map
		if( wr_mode_cd[ lcv ] & 1 )
		{
			// fail: outside boundaries
			if( stop < wr_low_cd[ lcv ] ) continue;
			if( start > wr_high_cd[ lcv ] ) continue;
		}
		// shadow map
		else
		{
			// fail: outside boundaries
			if( ( stop & 0xffff ) < ( wr_low_cd[ lcv ] & 0xffff ) ) continue;
			if( ( start & 0xffff ) > ( wr_high_cd[ lcv ] & 0xffff ) ) continue;
			
			if( ( stop >> 16 ) < ( wr_low_cd[ lcv ] >> 16 ) ) continue;
			if( ( start >> 16 ) > ( wr_high_cd[ lcv ] >> 16 ) ) continue;
		}

// ------------------------------------------------------

		// auto-trace
		if( wr_mode_cd[ lcv ] >= 4 && wr_mode_cd[ lcv ] < 8 )
		{
			wr_mode_cd[ lcv ] &= 3;
			trace_map = 1;

			hook_trace = 0;
			GensTrace_cd();
			hook_trace = 1;
		}

		// output file mode
		out = ( wr_mode_cd[ lcv ] <= 1 ) ? fp_hook_cd : fp_trace_cd;

		if( !out ) continue;
		if( out == fp_trace_cd )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s = %04X [%06X]\n",
			hook_pc_cd >> 16, hook_pc_cd & 0xffff, "W16",
			hook_value_cd & 0xffff, hook_address_cd );
	}
}
void trace_write_word_cd()
{
	if( hook_trace )
		trace_write_word_cd_internal();
	CallRegisteredLuaMemHook(hook_address_cd, 2, hook_value_cd, LUAMEMHOOK_WRITE_SUB);
}


static void trace_write_dword_cd_internal()
{
	unsigned int start, stop;

	start = hook_address_cd;
	stop = start + 3;

	for( int lcv = 0; lcv < STATES; lcv++ )
	{
		FILE *out;

		// linear map
		if( wr_mode_cd[ lcv ] & 1 )
		{
			// fail: outside boundaries
			if( stop < wr_low_cd[ lcv ] ) continue;
			if( start > wr_high_cd[ lcv ] ) continue;
		}
		// shadow map
		else
		{
			// fail: outside boundaries
			if( ( stop & 0xffff ) < ( wr_low_cd[ lcv ] & 0xffff ) ) continue;
			if( ( start & 0xffff ) > ( wr_high_cd[ lcv ] & 0xffff ) ) continue;
			
			if( ( stop >> 16 ) < ( wr_low_cd[ lcv ] >> 16 ) ) continue;
			if( ( start >> 16 ) > ( wr_high_cd[ lcv ] >> 16 ) ) continue;
		}

// ------------------------------------------------------

		// auto-trace
		if( wr_mode_cd[ lcv ] >= 4 && wr_mode_cd[ lcv ] < 8 )
		{
			wr_mode_cd[ lcv ] &= 3;
			trace_map = 1;

			hook_trace = 0;
			GensTrace_cd();
			hook_trace = 1;
		}

		// output file mode
		out = ( wr_mode_cd[ lcv ] <= 1 ) ? fp_hook_cd : fp_trace_cd;

		if( !out ) continue;
		if( out == fp_trace_cd )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s = %08X [%06X]\n",
			hook_pc_cd >> 16, hook_pc_cd & 0xffff, "W32",
			hook_value_cd & 0xffffffff, hook_address_cd );
	}
}
void trace_write_dword_cd()
{
	if( hook_trace )
		trace_write_dword_cd_internal();
	CallRegisteredLuaMemHook(hook_address_cd, 4, hook_value_cd, LUAMEMHOOK_WRITE_SUB);
}

#if 0
static void hook_dma_cd()
{
	unsigned int start, stop;
	int lcv;

	hook_pc_cd &= 0x00ffffff;
	hook_address_cd &= 0x00ffffff;

	// VDP area
	hook_value_cd &= 3;

	// Memory breakpoints
	start = VDP_Reg.DMA_Address << 1;
	stop = start + ( VDP_Reg.DMA_Lenght << 1 ) - 1;

	for( lcv = 0; lcv < STATES; lcv++ )
	{
		FILE *out;

		// linear map
		if( rd_mode_cd[ lcv ] & 1 )
		{
			// fail: outside boundaries
			if( stop < rd_low_cd[ lcv ] ) continue;
			if( start > rd_high_cd[ lcv ] ) continue;
		}
		// shadow map
		else
		{
			// fail: outside boundaries
			if( ( stop & 0xffff ) < ( rd_low_cd[ lcv ] & 0xffff ) ) continue;
			if( ( start & 0xffff ) > ( rd_high_cd[ lcv ] & 0xffff ) ) continue;
			
			if( ( stop >> 16 ) < ( rd_low_cd[ lcv ] >> 16 ) ) continue;
			if( ( start >> 16 ) > ( rd_high_cd[ lcv ] >> 16 ) ) continue;
		}

// ------------------------------------------------------

		// auto-trace
		if( rd_mode_cd[ lcv ] >= 4 && rd_mode_cd[ lcv ] < 8 )
		{
			rd_mode_cd[ lcv ] &= 3;
			trace_map = 1;

			hook_trace = 0;
			GensTrace_cd();
			hook_trace = 1;
		}

		// output file mode
		out = ( rd_mode_cd[ lcv ] <= 1 ) ? fp_hook_cd : fp_trace_cd;

		if( !out ) continue;
		if( out == fp_trace_cd )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s DMA from %06X to %04X [%04X]\n",
			hook_pc_cd >> 16, hook_pc_cd & 0xffff,
			( hook_value_cd <= 1 ) ? "VRAM" : ( hook_value_cd == 2 ) ? "CRAM" : "VSRAM",
			start & 0xffffff,
			Ctrl.Address & 0xffff,
			( VDP_Reg.DMA_Lenght << 1 ) & 0xffff );

		fprintf( out, "- SRC_H = [%02X:%04X]  ~~  LEN_H = [%02X:%04X]\n",
			dma_src >> 16, dma_src & 0xffff,
			dma_len >> 16, dma_len & 0xffff );

		fprintf( out, "\n" );
	}

	/**************************************************/
	/**************************************************/

	unsigned int start_l, stop_l;

	start = Ctrl.Address;
	stop = start + ( VDP_Reg.DMA_Lenght << 1 ) - 1;

	// local linear
	if( hook_value_cd <= 1 )
	{
		// VRAM
		start_l = start + 0x00000;
		stop_l = stop + 0x00000;
	}
	else if( hook_value_cd == 2 )
	{
		// CRAM
		start_l = start + 0x10000;
		stop_l = stop + 0x10000;
	}
	else if( hook_value_cd == 3 )
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
			trace_map = 1;

			hook_trace = 0;
			GensTrace_cd();
			hook_trace = 1;
		}

		// output file mode
		out = ( ppu_mode[ lcv ] <= 1 ) ? fp_hook_cd : fp_trace_cd;

		if( !out ) continue;
		if( out == fp_trace_cd )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s DMA from %06X to %04X [%04X]\n",
			hook_pc_cd >> 16, hook_pc_cd & 0xffff,
			( hook_value_cd <= 1 ) ? "VRAM" : ( hook_value_cd <= 2 ) ? "CRAM" : "VSRAM",
			( VDP_Reg.DMA_Address << 1 ) & 0xffffff,
			start & 0xffff, ( VDP_Reg.DMA_Lenght << 1 ) & 0xffff );

		fprintf( out, "- SRC_H = $%02X:%04X  ~~  LEN_H = $%02X:%04X\n",
			dma_src >> 16, dma_src & 0xffff,
			dma_len >> 16, dma_len & 0xffff );

		fprintf( out, "\n" );
	}
}
void hook_dma_cd()
{
	if( hook_trace )
		hook_dma_cd_internal();
}


static void trace_write_vram_byte_cd_internal()
{
	unsigned int start, stop;
	unsigned int start_l, stop_l;

	hook_pc_cd &= 0x00ffffff;
	hook_address_cd &= 0x00ffffff;

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
			trace_map = 1;

			hook_trace = 0;
			GensTrace_cd();
			hook_trace = 1;
		}

		// output file mode
		out = ( ppu_mode[ lcv ] <= 1 ) ? fp_hook_cd : fp_trace_cd;

		if( !out ) continue;
		if( out == fp_trace_cd )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s W08 = %02X [%04X]\n",
			hook_pc_cd >> 16, hook_pc_cd & 0xffff,
			( Ctrl.Access == 9 ) ? "VRAM" : ( Ctrl.Access == 10 ) ? "CRAM" : "VSRAM",
			hook_value_cd & 0xff, Ctrl.Address & 0xffff );
	}
}
void trace_write_vram_byte_cd()
{
	if( hook_trace )
		trace_write_vram_byte_cd_internal();
}


static void trace_write_vram_word_cd_internal()
{
	unsigned int start, stop;
	unsigned int start_l, stop_l;

	if( !hook_trace ) return;

	hook_pc_cd &= 0x00ffffff;
	hook_address_cd &= 0x00ffffff;

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
			trace_map = 1;

			hook_trace = 0;
			GensTrace_cd();
			hook_trace = 1;
		}

		// output file mode
		out = ( ppu_mode[ lcv ] <= 1 ) ? fp_hook_cd : fp_trace_cd;

		if( !out ) continue;
		if( out == fp_trace_cd )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s W16 = %04X [%04X]\n",
			hook_pc_cd >> 16, hook_pc_cd & 0xffff,
			( Ctrl.Access == 9 ) ? "VRAM" : ( Ctrl.Access == 10 ) ? "CRAM" : "VSRAM",
			hook_value_cd & 0xffff, Ctrl.Address & 0xffff );
	}
}
void trace_write_vram_word_cd()
{
	if( hook_trace )
		trace_write_vram_word_cd_internal();
}


static void trace_read_vram_byte_cd_internal()
{
	unsigned int start, stop;
	unsigned int start_l, stop_l;

	hook_pc_cd &= 0x00ffffff;
	hook_address_cd &= 0x00ffffff;

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
			trace_map = 1;

			hook_trace = 0;
			GensTrace_cd();
			hook_trace = 1;
		}

		// output file mode
		out = ( ppu_mode[ lcv ] <= 1 ) ? fp_hook_cd : fp_trace_cd;

		if( !out ) continue;
		if( out == fp_trace_cd )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s R08 = %02X [%04X]\n",
			hook_pc_cd >> 16, hook_pc_cd & 0xffff,
			( Ctrl.Access == 5 ) ? "VRAM" : ( Ctrl.Access == 6 ) ? "CRAM" : "VSRAM",
			hook_value_cd & 0xff, Ctrl.Address & 0xffff );
	}
}
void trace_read_vram_byte_cd()
{
	if( hook_trace )
		trace_read_vram_byte_cd_internal();
}


static void trace_read_vram_word_cd_internal()
{
	unsigned int start, stop;
	unsigned int start_l, stop_l;

	if( !hook_trace ) return;

	hook_pc_cd &= 0x00ffffff;
	hook_address_cd &= 0x00ffffff;

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
			trace_map = 1;

			hook_trace = 0;
			GensTrace_cd();
			hook_trace = 1;
		}

		// output file mode
		out = ( ppu_mode[ lcv ] <= 1 ) ? fp_hook_cd : fp_trace_cd;

		if( !out ) continue;
		if( out == fp_trace_cd )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] %s R16 = %04X [%04X]\n",
			hook_pc_cd >> 16, hook_pc_cd & 0xffff,
			( Ctrl.Access == 5 ) ? "VRAM" : ( Ctrl.Access == 6 ) ? "CRAM" : "VSRAM",
			hook_value_cd & 0xffff, Ctrl.Address & 0xffff );
	}
}
void trace_read_vram_word_cd()
{
	if( hook_trace )
		trace_read_vram_word_cd_internal();
}


static void hook_vdp_reg_cd_internal()
{
	unsigned int start, stop;

	hook_pc_cd &= 0x00ffffff;
	hook_address_cd &= 0x00ffffff;

	start = hook_address_cd;
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
			trace_map = 1;

			hook_trace = 0;
			GensTrace_cd();
			hook_trace = 1;
		}

		// output file mode
		out = ( ppu_mode[ lcv ] <= 1 ) ? fp_hook_cd : fp_trace_cd;

		if( !out ) continue;
		if( out == fp_trace_cd )
			fprintf( out, "** " );

		fprintf( out, "[%02X:%04X] VDP REG W08 = %02X [%02X]\n",
			hook_pc_cd >> 16, hook_pc_cd & 0xffff,
			hook_value_cd & 0xff, hook_address_cd & 0xff );
	}
}
void hook_vdp_reg_cd()
{
	if( hook_trace )
		hook_vdp_reg_cd_internal();
}
#endif