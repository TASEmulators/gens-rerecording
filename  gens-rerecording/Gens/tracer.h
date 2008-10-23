#ifndef tracer_h
#define tracer_h
extern "C" {
	void trace_read_byte();
	void trace_read_byte_cd();
	void trace_read_word();
	void trace_read_word_cd();
	void trace_read_dword();
	void trace_read_dword_cd();
	void trace_write_byte();
	void trace_write_byte_cd();
	void trace_write_word();
	void trace_write_word_cd();
	void trace_write_dword();
	void trace_write_dword_cd();

	void trace_write_vram_byte();
	void trace_write_vram_word();
	void trace_read_vram_byte();
	void trace_read_vram_word();

	void hook_dma();
	void hook_vdp_reg();

	void GensTrace();
	void GensTrace_cd();
}

void InitDebug();
void InitDebug_cd();
void DeInitDebug();
void DeInitDebug_cd();
#endif