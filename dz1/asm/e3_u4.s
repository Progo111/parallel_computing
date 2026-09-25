	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 5
	.globl	_mat_add_e3_u4                  ; -- Begin function mat_add_e3_u4
	.p2align	2
_mat_add_e3_u4:                         ; @mat_add_e3_u4
	.cfi_startproc
; %bb.0:
	mul	x8, x3, x3
	cmp	x8, #16
	b.hs	LBB0_2
; %bb.1:
	mov	x9, #0                          ; =0x0
	subs	x8, x8, x9
	b.hi	LBB0_5
	b	LBB0_7
LBB0_2:
	add	x9, x2, #32
	add	x10, x0, #32
	add	x12, x1, #32
	mov	w11, #16                        ; =0x10
LBB0_3:                                 ; =>This Inner Loop Header: Depth=1
	ldp	q0, q1, [x10, #-32]
	ldp	q2, q3, [x10], #64
	ldp	q4, q5, [x12, #-32]
	ldp	q6, q7, [x12], #64
	fadd.4s	v0, v0, v4
	fadd.4s	v1, v1, v5
	fadd.4s	v2, v2, v6
	fadd.4s	v3, v3, v7
	stp	q0, q1, [x9, #-32]
	add	x11, x11, #16
	stp	q2, q3, [x9], #64
	cmp	x11, x8
	b.ls	LBB0_3
; %bb.4:
	sub	x9, x11, #16
	subs	x8, x8, x9
	b.ls	LBB0_7
LBB0_5:
	lsl	x11, x9, #2
	add	x9, x2, x11
	add	x10, x1, x11
	add	x11, x0, x11
LBB0_6:                                 ; =>This Inner Loop Header: Depth=1
	ldr	s0, [x11], #4
	ldr	s1, [x10], #4
	fadd	s0, s0, s1
	str	s0, [x9], #4
	subs	x8, x8, #1
	b.ne	LBB0_6
LBB0_7:
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	_mat_sub_e3_u4                  ; -- Begin function mat_sub_e3_u4
	.p2align	2
_mat_sub_e3_u4:                         ; @mat_sub_e3_u4
	.cfi_startproc
; %bb.0:
	mul	x8, x3, x3
	cmp	x8, #16
	b.hs	LBB1_2
; %bb.1:
	mov	x9, #0                          ; =0x0
	subs	x8, x8, x9
	b.hi	LBB1_5
	b	LBB1_7
LBB1_2:
	add	x9, x2, #32
	add	x10, x0, #32
	add	x12, x1, #32
	mov	w11, #16                        ; =0x10
LBB1_3:                                 ; =>This Inner Loop Header: Depth=1
	ldp	q0, q1, [x10, #-32]
	ldp	q2, q3, [x10], #64
	ldp	q4, q5, [x12, #-32]
	ldp	q6, q7, [x12], #64
	fsub.4s	v0, v0, v4
	fsub.4s	v1, v1, v5
	fsub.4s	v2, v2, v6
	fsub.4s	v3, v3, v7
	stp	q0, q1, [x9, #-32]
	add	x11, x11, #16
	stp	q2, q3, [x9], #64
	cmp	x11, x8
	b.ls	LBB1_3
; %bb.4:
	sub	x9, x11, #16
	subs	x8, x8, x9
	b.ls	LBB1_7
LBB1_5:
	lsl	x11, x9, #2
	add	x9, x2, x11
	add	x10, x1, x11
	add	x11, x0, x11
LBB1_6:                                 ; =>This Inner Loop Header: Depth=1
	ldr	s0, [x11], #4
	ldr	s1, [x10], #4
	fsub	s0, s0, s1
	str	s0, [x9], #4
	subs	x8, x8, #1
	b.ne	LBB1_6
LBB1_7:
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	_mat_addsub_e3_u4               ; -- Begin function mat_addsub_e3_u4
	.p2align	2
_mat_addsub_e3_u4:                      ; @mat_addsub_e3_u4
	.cfi_startproc
; %bb.0:
	mul	x8, x4, x4
	cmp	x8, #16
	b.hs	LBB2_2
; %bb.1:
	mov	x9, #0                          ; =0x0
	subs	x8, x8, x9
	b.hi	LBB2_5
	b	LBB2_7
LBB2_2:
	add	x9, x3, #32
	add	x10, x0, #32
	add	x11, x2, #32
	add	x13, x1, #32
	mov	w12, #16                        ; =0x10
LBB2_3:                                 ; =>This Inner Loop Header: Depth=1
	ldp	q0, q1, [x10, #-32]
	ldp	q2, q3, [x10], #64
	ldp	q4, q5, [x13, #-32]
	ldp	q6, q7, [x13], #64
	fadd.4s	v16, v0, v4
	fadd.4s	v17, v1, v5
	fadd.4s	v18, v2, v6
	fadd.4s	v19, v3, v7
	fsub.4s	v0, v0, v4
	fsub.4s	v1, v1, v5
	fsub.4s	v2, v2, v6
	fsub.4s	v3, v3, v7
	stp	q16, q17, [x11, #-32]
	stp	q0, q1, [x9, #-32]
	stp	q2, q3, [x9], #64
	add	x12, x12, #16
	stp	q18, q19, [x11], #64
	cmp	x12, x8
	b.ls	LBB2_3
; %bb.4:
	sub	x9, x12, #16
	subs	x8, x8, x9
	b.ls	LBB2_7
LBB2_5:
	lsl	x12, x9, #2
	add	x9, x3, x12
	add	x10, x2, x12
	add	x11, x1, x12
	add	x12, x0, x12
LBB2_6:                                 ; =>This Inner Loop Header: Depth=1
	ldr	s0, [x12], #4
	ldr	s1, [x11], #4
	fadd	s2, s0, s1
	str	s2, [x10], #4
	fsub	s0, s0, s1
	str	s0, [x9], #4
	subs	x8, x8, #1
	b.ne	LBB2_6
LBB2_7:
	ret
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols
