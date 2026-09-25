	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 5
	.globl	_mat_add_e2_w4i4                ; -- Begin function mat_add_e2_w4i4
	.p2align	2
_mat_add_e2_w4i4:                       ; @mat_add_e2_w4i4
	.cfi_startproc
; %bb.0:
	mul	x8, x3, x3
	ands	x9, x8, #0xfffffffffffffff0
	b.eq	LBB0_3
; %bb.1:
	add	x10, x2, #32
	add	x11, x1, #32
	add	x12, x0, #32
	mov	x13, x9
LBB0_2:                                 ; =>This Inner Loop Header: Depth=1
	ldp	q0, q1, [x12, #-32]
	ldp	q2, q3, [x12], #64
	ldp	q4, q5, [x11, #-32]
	ldp	q6, q7, [x11], #64
	fadd.4s	v0, v0, v4
	fadd.4s	v1, v1, v5
	fadd.4s	v2, v2, v6
	fadd.4s	v3, v3, v7
	stp	q0, q1, [x10, #-32]
	stp	q2, q3, [x10], #64
	subs	x13, x13, #16
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
	.globl	_mat_sub_e2_w4i4                ; -- Begin function mat_sub_e2_w4i4
	.p2align	2
_mat_sub_e2_w4i4:                       ; @mat_sub_e2_w4i4
	.cfi_startproc
; %bb.0:
	mul	x8, x3, x3
	ands	x9, x8, #0xfffffffffffffff0
	b.eq	LBB1_3
; %bb.1:
	add	x10, x2, #32
	add	x11, x1, #32
	add	x12, x0, #32
	mov	x13, x9
LBB1_2:                                 ; =>This Inner Loop Header: Depth=1
	ldp	q0, q1, [x12, #-32]
	ldp	q2, q3, [x12], #64
	ldp	q4, q5, [x11, #-32]
	ldp	q6, q7, [x11], #64
	fsub.4s	v0, v0, v4
	fsub.4s	v1, v1, v5
	fsub.4s	v2, v2, v6
	fsub.4s	v3, v3, v7
	stp	q0, q1, [x10, #-32]
	stp	q2, q3, [x10], #64
	subs	x13, x13, #16
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
	.globl	_mat_addsub_e2_w4i4             ; -- Begin function mat_addsub_e2_w4i4
	.p2align	2
_mat_addsub_e2_w4i4:                    ; @mat_addsub_e2_w4i4
	.cfi_startproc
; %bb.0:
	mul	x8, x4, x4
	ands	x9, x8, #0xfffffffffffffff0
	b.eq	LBB2_3
; %bb.1:
	add	x10, x3, #32
	add	x11, x2, #32
	add	x12, x0, #32
	add	x13, x1, #32
	mov	x14, x9
LBB2_2:                                 ; =>This Inner Loop Header: Depth=1
	ldp	q0, q1, [x12, #-32]
	ldp	q2, q3, [x12], #64
	ldp	q4, q5, [x13, #-32]
	ldp	q6, q7, [x13], #64
	fadd.4s	v16, v0, v4
	fadd.4s	v17, v1, v5
	fadd.4s	v18, v2, v6
	fadd.4s	v19, v3, v7
	stp	q16, q17, [x11, #-32]
	fsub.4s	v0, v0, v4
	fsub.4s	v1, v1, v5
	fsub.4s	v2, v2, v6
	fsub.4s	v3, v3, v7
	stp	q0, q1, [x10, #-32]
	stp	q2, q3, [x10], #64
	stp	q18, q19, [x11], #64
	subs	x14, x14, #16
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
