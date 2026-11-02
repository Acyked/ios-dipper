#pragma once
#include <stdint.h>

#define FB_BASE		0x9d400000UL	/* dipper cont_splash_region@9d400000 */
#define FB_W		1080
#define FB_H		2248
#define FB_STRIDE	(FB_W * 4)

/* dipper: boot has no visible console (cmd-mode panel). Stage markers go to
 * IMEM and, once ufs_log_init() ran, to the minidump UFS partition (read back
 * with "edl.py rs 0x10000 1 ulog.bin"). */
#define DBG_BASE	0x146bf800UL
void ulog_put(unsigned idx, uint64_t v);
static inline void dbg_raw(unsigned idx, uint64_t v)
{
	*(volatile uint64_t *)(DBG_BASE + (uint64_t)idx * 8) = v;
	ulog_put(idx, v);
}
#define dbg(idx, v) do { dbg_raw(0, 0x5348494d444247ULL); dbg_raw((idx), (v)); } while (0)

/* a8r8g8b8, matches mainline simplefb */
#define RED	0xffff0000U
#define BLUE	0xff0000a0U
#define WHITE	0xffffffffU
#define YELLOW	0xffffff00U
#define GREEN	0xff00ff00U

extern volatile uint32_t *con_fb;

void con_clear(uint32_t bg);
void con_skip(void);
void con_str(const char *label, const char *s);
void con_hex(const char *label, uint64_t v);
/* label, then "a=xxxxxxxx b=xxxxxxxx" style pairs of 32-bit values */
void con_hex2(const char *label, const char *a, uint32_t va, const char *b, uint32_t vb);
