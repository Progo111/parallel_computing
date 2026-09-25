	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 5
	.globl	_mat_add_e2_w4i1                ; -- Begin function mat_add_e2_w4i1
	.p2align	2
_mat_add_e2_w4i1:                       ; @mat_add_e2_w4i1
	.cfi_startproc
; %bb.0:
	mul	x8, x3, x3
	ands	x9, x8, #0xfffffffffffffffc
	b.eq	LBB0_3
; %bb.1:
	mov	x10, x9
	mov	x11, x2
	mov	x12, x1
	mov	x13, x0
LBB0_2:                                 ; =>This Inner Loop Header: Depth=1
	ldr	q0, [x13], #16
	ldr	q1, [x12], #16
	fadd.4s	v0, v0, v1
	str	q0, [x11], #16
	subs	x10, x10, #4
	b.ne	LBB0_2
LBB0_3:
	cmp	x9, x8
	b.eq	LBB0_5
LBB0_4:                                 ; =>This Inner Loop Header: Depth=1
	ldr	s0, [x0, x9, lsl #2]
	ldr	s1, [x1, x9, lsl #2]
	fadd	s0, s0, s1
	str	s0, [x2, x9, lsl #2]
	add	x9, x9, #1
	cmp	x9, x8
	b.lo	LBB0_4
LBB0_5:
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	_mat_sub_e2_w4i1                ; -- Begin function mat_sub_e2_w4i1
	.p2align	2
_mat_sub_e2_w4i1:                       ; @mat_sub_e2_w4i1
	.cfi_startproc
; %bb.0:
	mul	x8, x3, x3
	ands	x9, x8, #0xfffffffffffffffc
	b.eq	LBB1_3
; %bb.1:
	mov	x10, x9
	mov	x11, x2
	mov	x12, x1
	mov	x13, x0
LBB1_2:                                 ; =>This Inner Loop Header: Depth=1
	ldr	q0, [x13], #16
	ldr	q1, [x12], #16
	fsub.4s	v0, v0, v1
	str	q0, [x11], #16
	subs	x10, x10, #4
	b.ne	LBB1_2
LBB1_3:
	cmp	x9, x8
	b.eq	LBB1_5
LBB1_4:                                 ; =>This Inner Loop Header: Depth=1
	ldr	s0, [x0, x9, lsl #2]
	ldr	s1, [x1, x9, lsl #2]
	fsub	s0, s0, s1
	str	s0, [x2, x9, lsl #2]
	add	x9, x9, #1
	cmp	x9, x8
	b.lo	LBB1_4
LBB1_5:
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	_mat_addsub_e2_w4i1             ; -- Begin function mat_addsub_e2_w4i1
	.p2align	2
_mat_addsub_e2_w4i1:                    ; @mat_addsub_e2_w4i1
	.cfi_startproc
; %bb.0:
	mul	x8, x4, x4
	ands	x9, x8, #0xfffffffffffffffc
	b.eq	LBB2_3
; %bb.1:
	mov	x10, x9
	mov	x11, x3
	mov	x12, x2
	mov	x13, x1
	mov	x14, x0
LBB2_2:                                 ; =>This Inner Loop Header: Depth=1
	ldr	q0, [x14], #16
	ldr	q1, [x13], #16
	fadd.4s	v2, v0, v1
	str	q2, [x12], #16
	fsub.4s	v0, v0, v1
	str	q0, [x11], #16
	subs	x10, x10, #4
	b.ne	LBB2_2
LBB2_3:
	cmp	x9, x8
	b.eq	LBB2_5
LBB2_4:                                 ; =>This Inner Loop Header: Depth=1
	ldr	s0, [x0, x9, lsl #2]
	ldr	s1, [x1, x9, lsl #2]
	fadd	s2, s0, s1
	str	s2, [x2, x9, lsl #2]
	fsub	s0, s0, s1
	str	s0, [x3, x9, lsl #2]
	add	x9, x9, #1
	cmp	x9, x8
	b.lo	LBB2_4
LBB2_5:
	ret
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols
