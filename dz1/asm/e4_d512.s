	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 5
	.globl	_mat_add_e4_d512                ; -- Begin function mat_add_e4_d512
	.p2align	2
_mat_add_e4_d512:                       ; @mat_add_e4_d512
	.cfi_startproc
; %bb.0:
	mul	x8, x3, x3
	cmp	x8, #16
	b.hs	LBB0_2
; %bb.1:
	mov	x13, #0                         ; =0x0
	b	LBB0_4
LBB0_2:
	mov	x12, #0                         ; =0x0
	add	x9, x2, #32
	mov	x10, x0
	mov	x11, x1
LBB0_3:                                 ; =>This Inner Loop Header: Depth=1
	prfm	pldl1strm, [x10, #2048]
	prfm	pldl1strm, [x11, #2048]
	ldp	q0, q1, [x10]
	ldp	q2, q3, [x10, #32]
	ldp	q4, q5, [x11]
	fadd.4s	v0, v0, v4
	fadd.4s	v1, v1, v5
	stnp	q0, q1, [x9, #-32]
	ldp	q0, q1, [x11, #32]
	fadd.4s	v0, v2, v0
	fadd.4s	v1, v3, v1
	stnp	q0, q1, [x9]
	add	x13, x12, #16
	add	x14, x12, #32
	add	x9, x9, #64
	add	x11, x11, #64
	add	x10, x10, #64
	mov	x12, x13
	cmp	x14, x8
	b.ls	LBB0_3
LBB0_4:
	subs	x8, x8, x13
	b.ls	LBB0_7
; %bb.5:
	lsl	x11, x13, #2
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
	.globl	_mat_sub_e4_d512                ; -- Begin function mat_sub_e4_d512
	.p2align	2
_mat_sub_e4_d512:                       ; @mat_sub_e4_d512
	.cfi_startproc
; %bb.0:
	mul	x8, x3, x3
	cmp	x8, #16
	b.hs	LBB1_2
; %bb.1:
	mov	x13, #0                         ; =0x0
	b	LBB1_4
LBB1_2:
	mov	x12, #0                         ; =0x0
	add	x9, x2, #32
	mov	x10, x0
	mov	x11, x1
LBB1_3:                                 ; =>This Inner Loop Header: Depth=1
	prfm	pldl1strm, [x10, #2048]
	prfm	pldl1strm, [x11, #2048]
	ldp	q0, q1, [x10]
	ldp	q2, q3, [x10, #32]
	ldp	q4, q5, [x11]
	fsub.4s	v0, v0, v4
	fsub.4s	v1, v1, v5
	stnp	q0, q1, [x9, #-32]
	ldp	q0, q1, [x11, #32]
	fsub.4s	v0, v2, v0
	fsub.4s	v1, v3, v1
	stnp	q0, q1, [x9]
	add	x13, x12, #16
	add	x14, x12, #32
	add	x9, x9, #64
	add	x11, x11, #64
	add	x10, x10, #64
	mov	x12, x13
	cmp	x14, x8
	b.ls	LBB1_3
LBB1_4:
	subs	x8, x8, x13
	b.ls	LBB1_7
; %bb.5:
	lsl	x11, x13, #2
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
	.globl	_mat_addsub_e4_d512             ; -- Begin function mat_addsub_e4_d512
	.p2align	2
_mat_addsub_e4_d512:                    ; @mat_addsub_e4_d512
	.cfi_startproc
; %bb.0:
	mul	x8, x4, x4
	cmp	x8, #16
	b.hs	LBB2_2
; %bb.1:
	mov	x14, #0                         ; =0x0
	b	LBB2_4
LBB2_2:
	mov	x13, #0                         ; =0x0
	add	x9, x3, #32
	add	x10, x2, #32
	mov	x11, x1
	mov	x12, x0
LBB2_3:                                 ; =>This Inner Loop Header: Depth=1
	prfm	pldl1strm, [x12, #2048]
	prfm	pldl1strm, [x11, #2048]
	ldp	q0, q1, [x12]
	ldp	q2, q3, [x12, #32]
	ldp	q4, q5, [x11]
	ldp	q6, q7, [x11, #32]
	fadd.4s	v16, v0, v4
	fadd.4s	v17, v1, v5
	stnp	q16, q17, [x10, #-32]
	fadd.4s	v16, v2, v6
	fadd.4s	v17, v3, v7
	stnp	q16, q17, [x10]
	fsub.4s	v0, v0, v4
	fsub.4s	v1, v1, v5
	stnp	q0, q1, [x9, #-32]
	fsub.4s	v0, v2, v6
	fsub.4s	v1, v3, v7
	stnp	q0, q1, [x9]
	add	x14, x13, #16
	add	x15, x13, #32
	add	x9, x9, #64
	add	x10, x10, #64
	add	x12, x12, #64
	add	x11, x11, #64
	mov	x13, x14
	cmp	x15, x8
	b.ls	LBB2_3
LBB2_4:
	subs	x8, x8, x14
	b.ls	LBB2_7
; %bb.5:
	lsl	x12, x14, #2
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
