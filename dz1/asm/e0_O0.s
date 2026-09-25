	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 5
	.globl	_mat_add_e0_O0                  ; -- Begin function mat_add_e0_O0
	.p2align	2
_mat_add_e0_O0:                         ; @mat_add_e0_O0
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #48
	.cfi_def_cfa_offset 48
	str	x0, [sp, #40]
	str	x1, [sp, #32]
	str	x2, [sp, #24]
	str	x3, [sp, #16]
	str	xzr, [sp, #8]
	b	LBB0_1
LBB0_1:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_3 Depth 2
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #16]
	subs	x8, x8, x9
	b.hs	LBB0_8
	b	LBB0_2
LBB0_2:                                 ;   in Loop: Header=BB0_1 Depth=1
	str	xzr, [sp]
	b	LBB0_3
LBB0_3:                                 ;   Parent Loop BB0_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	x8, [sp]
	ldr	x9, [sp, #16]
	subs	x8, x8, x9
	b.hs	LBB0_6
	b	LBB0_4
LBB0_4:                                 ;   in Loop: Header=BB0_3 Depth=2
	ldr	x8, [sp, #40]
	ldr	x9, [sp, #8]
	ldr	x10, [sp, #16]
	mul	x9, x9, x10
	ldr	x10, [sp]
	add	x9, x9, x10
	ldr	s0, [x8, x9, lsl #2]
	ldr	x8, [sp, #32]
	ldr	x9, [sp, #8]
	ldr	x10, [sp, #16]
	mul	x9, x9, x10
	ldr	x10, [sp]
	add	x9, x9, x10
	ldr	s1, [x8, x9, lsl #2]
	fadd	s0, s0, s1
	ldr	x8, [sp, #24]
	ldr	x9, [sp, #8]
	ldr	x10, [sp, #16]
	mul	x9, x9, x10
	ldr	x10, [sp]
	add	x9, x9, x10
	str	s0, [x8, x9, lsl #2]
	b	LBB0_5
LBB0_5:                                 ;   in Loop: Header=BB0_3 Depth=2
	ldr	x8, [sp]
	add	x8, x8, #1
	str	x8, [sp]
	b	LBB0_3
LBB0_6:                                 ;   in Loop: Header=BB0_1 Depth=1
	b	LBB0_7
LBB0_7:                                 ;   in Loop: Header=BB0_1 Depth=1
	ldr	x8, [sp, #8]
	add	x8, x8, #1
	str	x8, [sp, #8]
	b	LBB0_1
LBB0_8:
	add	sp, sp, #48
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	_mat_sub_e0_O0                  ; -- Begin function mat_sub_e0_O0
	.p2align	2
_mat_sub_e0_O0:                         ; @mat_sub_e0_O0
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #48
	.cfi_def_cfa_offset 48
	str	x0, [sp, #40]
	str	x1, [sp, #32]
	str	x2, [sp, #24]
	str	x3, [sp, #16]
	str	xzr, [sp, #8]
	b	LBB1_1
LBB1_1:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB1_3 Depth 2
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #16]
	subs	x8, x8, x9
	b.hs	LBB1_8
	b	LBB1_2
LBB1_2:                                 ;   in Loop: Header=BB1_1 Depth=1
	str	xzr, [sp]
	b	LBB1_3
LBB1_3:                                 ;   Parent Loop BB1_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	x8, [sp]
	ldr	x9, [sp, #16]
	subs	x8, x8, x9
	b.hs	LBB1_6
	b	LBB1_4
LBB1_4:                                 ;   in Loop: Header=BB1_3 Depth=2
	ldr	x8, [sp, #40]
	ldr	x9, [sp, #8]
	ldr	x10, [sp, #16]
	mul	x9, x9, x10
	ldr	x10, [sp]
	add	x9, x9, x10
	ldr	s0, [x8, x9, lsl #2]
	ldr	x8, [sp, #32]
	ldr	x9, [sp, #8]
	ldr	x10, [sp, #16]
	mul	x9, x9, x10
	ldr	x10, [sp]
	add	x9, x9, x10
	ldr	s1, [x8, x9, lsl #2]
	fsub	s0, s0, s1
	ldr	x8, [sp, #24]
	ldr	x9, [sp, #8]
	ldr	x10, [sp, #16]
	mul	x9, x9, x10
	ldr	x10, [sp]
	add	x9, x9, x10
	str	s0, [x8, x9, lsl #2]
	b	LBB1_5
LBB1_5:                                 ;   in Loop: Header=BB1_3 Depth=2
	ldr	x8, [sp]
	add	x8, x8, #1
	str	x8, [sp]
	b	LBB1_3
LBB1_6:                                 ;   in Loop: Header=BB1_1 Depth=1
	b	LBB1_7
LBB1_7:                                 ;   in Loop: Header=BB1_1 Depth=1
	ldr	x8, [sp, #8]
	add	x8, x8, #1
	str	x8, [sp, #8]
	b	LBB1_1
LBB1_8:
	add	sp, sp, #48
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	_mat_addsub_e0_O0               ; -- Begin function mat_addsub_e0_O0
	.p2align	2
_mat_addsub_e0_O0:                      ; @mat_addsub_e0_O0
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #64
	.cfi_def_cfa_offset 64
	str	x0, [sp, #56]
	str	x1, [sp, #48]
	str	x2, [sp, #40]
	str	x3, [sp, #32]
	str	x4, [sp, #24]
	str	xzr, [sp, #16]
	b	LBB2_1
LBB2_1:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB2_3 Depth 2
	ldr	x8, [sp, #16]
	ldr	x9, [sp, #24]
	subs	x8, x8, x9
	b.hs	LBB2_8
	b	LBB2_2
LBB2_2:                                 ;   in Loop: Header=BB2_1 Depth=1
	str	xzr, [sp, #8]
	b	LBB2_3
LBB2_3:                                 ;   Parent Loop BB2_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #24]
	subs	x8, x8, x9
	b.hs	LBB2_6
	b	LBB2_4
LBB2_4:                                 ;   in Loop: Header=BB2_3 Depth=2
	ldr	x8, [sp, #56]
	ldr	x9, [sp, #16]
	ldr	x10, [sp, #24]
	mul	x9, x9, x10
	ldr	x10, [sp, #8]
	add	x9, x9, x10
	ldr	s0, [x8, x9, lsl #2]
	str	s0, [sp, #4]
	ldr	x8, [sp, #48]
	ldr	x9, [sp, #16]
	ldr	x10, [sp, #24]
	mul	x9, x9, x10
	ldr	x10, [sp, #8]
	add	x9, x9, x10
	ldr	s0, [x8, x9, lsl #2]
	str	s0, [sp]
	ldr	s0, [sp, #4]
	ldr	s1, [sp]
	fadd	s0, s0, s1
	ldr	x8, [sp, #40]
	ldr	x9, [sp, #16]
	ldr	x10, [sp, #24]
	mul	x9, x9, x10
	ldr	x10, [sp, #8]
	add	x9, x9, x10
	str	s0, [x8, x9, lsl #2]
	ldr	s0, [sp, #4]
	ldr	s1, [sp]
	fsub	s0, s0, s1
	ldr	x8, [sp, #32]
	ldr	x9, [sp, #16]
	ldr	x10, [sp, #24]
	mul	x9, x9, x10
	ldr	x10, [sp, #8]
	add	x9, x9, x10
	str	s0, [x8, x9, lsl #2]
	b	LBB2_5
LBB2_5:                                 ;   in Loop: Header=BB2_3 Depth=2
	ldr	x8, [sp, #8]
	add	x8, x8, #1
	str	x8, [sp, #8]
	b	LBB2_3
LBB2_6:                                 ;   in Loop: Header=BB2_1 Depth=1
	b	LBB2_7
LBB2_7:                                 ;   in Loop: Header=BB2_1 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #1
	str	x8, [sp, #16]
	b	LBB2_1
LBB2_8:
	add	sp, sp, #64
	ret
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols
