/*
 * probe.S - definitive test: gzip-wrapped stub.
 * 1) fill framebuffer 0x9d400000+8MB magenta (visible if fb addr right)
 * 2) spin ~40s
 * 3) PSHOLD reset -> loops logo = periodic blink signal
 * Screen magenta => our code executes. Logo-only => crashed post-decompress.
 */
	.text
	.globl _start
_start:
	ldr	x1, =0x9d400000		/* simplefb base */
	mov	x2, #0x100000		/* 8MB/8 stores */
	movz	x3, #0x00ff, lsl #0
	movk	x3, #0xffff, lsl #16
	movk	x3, #0x00ff, lsl #32
	movk	x3, #0xffff, lsl #48
1:	str	x3, [x1], #8
	subs	x2, x2, #1
	b.ne	1b

	movz	x1, #0xa000, lsl #0
	movk	x1, #0x04a2, lsl #16
	movk	x1, #0x0009, lsl #32	/* ~40e9 iters */
2:	subs	x1, x1, #1
	b.ne	2b

	mov	x1, #0x4000
	movk	x1, #0x0c26, lsl #16	/* PSHOLD */
	str	wzr, [x1]
3:	wfi
	b	3b
