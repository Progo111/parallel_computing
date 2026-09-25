	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 5
	.globl	_mat_add_e0_O1                  ; -- Begin function mat_add_e0_O1
	.p2align	2
_mat_add_e0_O1:                         ; @mat_add_e0_O1
	.cfi_startproc
; %bb.0:
	cbz	x3, LBB0_5
; %bb.1:
	mov	x8, #0                          ; =0x0
	lsl	x9, x3, #2
LBB0_2:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_3 Depth 2
	mov	x10, #0                         ; =0x0
LBB0_3:                                 ;   Parent Loop BB0_2 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s0, [x0, x10, lsl #2]
	ldr	s1, [x1, x10, lsl #2]
	fadd	s0, s0, s1
	str	s0, [x2, x10, lsl #2]
	add	x10, x10, #1
	cmp	x3, x10
	b.ne	LBB0_3
; %bb.4:                                ;   in Loop: Header=BB0_2 Depth=1
	add	x8, x8, #1
	add	x2, x2, x9
	add	x1, x1, x9
	add	x0, x0, x9
	cmp	x8, x3
	b.ne	LBB0_2
LBB0_5:
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	_mat_sub_e0_O1                  ; -- Begin function mat_sub_e0_O1
	.p2align	2
_mat_sub_e0_O1:                         ; @mat_sub_e0_O1
	.cfi_startproc
; %bb.0:
	cbz	x3, LBB1_5
; %bb.1:
	mov	x8, #0                          ; =0x0
	lsl	x9, x3, #2
LBB1_2:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB1_3 Depth 2
	mov	x10, #0                         ; =0x0
LBB1_3:                                 ;   Parent Loop BB1_2 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s0, [x0, x10, lsl #2]
	ldr	s1, [x1, x10, lsl #2]
	fsub	s0, s0, s1
	str	s0, [x2, x10, lsl #2]
	add	x10, x10, #1
	cmp	x3, x10
	b.ne	LBB1_3
; %bb.4:                                ;   in Loop: Header=BB1_2 Depth=1
	add	x8, x8, #1
	add	x2, x2, x9
	add	x1, x1, x9
	add	x0, x0, x9
	cmp	x8, x3
	b.ne	LBB1_2
LBB1_5:
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	_mat_addsub_e0_O1               ; -- Begin function mat_addsub_e0_O1
	.p2align	2
_mat_addsub_e0_O1:                      ; @mat_addsub_e0_O1
	.cfi_startproc
; %bb.0:
	cbz	x4, LBB2_5
; %bb.1:
	mov	x8, #0                          ; =0x0
	lsl	x9, x4, #2
LBB2_2:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB2_3 Depth 2
	mov	x10, #0                         ; =0x0
LBB2_3:                                 ;   Parent Loop BB2_2 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s0, [x0, x10, lsl #2]
	ldr	s1, [x1, x10, lsl #2]
	fadd	s2, s0, s1
	str	s2, [x2, x10, lsl #2]
	fsub	s0, s0, s1
	str	s0, [x3, x10, lsl #2]
	add	x10, x10, #1
	cmp	x4, x10
	b.ne	LBB2_3
; %bb.4:                                ;   in Loop: Header=BB2_2 Depth=1
	add	x8, x8, #1
	add	x3, x3, x9
	add	x2, x2, x9
	add	x1, x1, x9
	add	x0, x0, x9
	cmp	x8, x4
	b.ne	LBB2_2
LBB2_5:
	ret
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols
