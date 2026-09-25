	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 5
	.globl	_mat_add_e2_w4i8                ; -- Begin function mat_add_e2_w4i8
	.p2align	2
_mat_add_e2_w4i8:                       ; @mat_add_e2_w4i8
	.cfi_startproc
; %bb.0:
	mul	x8, x3, x3
	ands	x9, x8, #0xffffffffffffffe0
	b.eq	LBB0_3
; %bb.1:
	add	x10, x2, #64
	add	x11, x1, #64
	add	x12, x0, #64
	mov	x13, x9
LBB0_2:                                 ; =>This Inner Loop Header: Depth=1
	ldp	q0, q1, [x12, #-64]
	ldp	q2, q3, [x12, #-32]
	ldp	q4, q5, [x12]
	ldp	q6, q7, [x12, #32]
	ldp	q16, q17, [x11, #-64]
	ldp	q18, q19, [x11, #-32]
	ldp	q20, q21, [x11]
	ldp	q22, q23, [x11, #32]
	fadd.4s	v0, v0, v16
	fadd.4s	v1, v1, v17
	fadd.4s	v2, v2, v18
	fadd.4s	v3, v3, v19
	fadd.4s	v4, v4, v20
	fadd.4s	v5, v5, v21
	fadd.4s	v6, v6, v22
	fadd.4s	v7, v7, v23
	stp	q0, q1, [x10, #-64]
	stp	q2, q3, [x10, #-32]
	stp	q4, q5, [x10]
	stp	q6, q7, [x10, #32]
	add	x10, x10, #128
	add	x11, x11, #128
	add	x12, x12, #128
	subs	x13, x13, #32
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
	.globl	_mat_sub_e2_w4i8                ; -- Begin function mat_sub_e2_w4i8
	.p2align	2
_mat_sub_e2_w4i8:                       ; @mat_sub_e2_w4i8
	.cfi_startproc
; %bb.0:
	mul	x8, x3, x3
	ands	x9, x8, #0xffffffffffffffe0
	b.eq	LBB1_3
; %bb.1:
	add	x10, x2, #64
	add	x11, x1, #64
	add	x12, x0, #64
	mov	x13, x9
LBB1_2:                                 ; =>This Inner Loop Header: Depth=1
	ldp	q0, q1, [x12, #-64]
	ldp	q2, q3, [x12, #-32]
	ldp	q4, q5, [x12]
	ldp	q6, q7, [x12, #32]
	ldp	q16, q17, [x11, #-64]
	ldp	q18, q19, [x11, #-32]
	ldp	q20, q21, [x11]
	ldp	q22, q23, [x11, #32]
	fsub.4s	v0, v0, v16
	fsub.4s	v1, v1, v17
	fsub.4s	v2, v2, v18
	fsub.4s	v3, v3, v19
	fsub.4s	v4, v4, v20
	fsub.4s	v5, v5, v21
	fsub.4s	v6, v6, v22
	fsub.4s	v7, v7, v23
	stp	q0, q1, [x10, #-64]
	stp	q2, q3, [x10, #-32]
	stp	q4, q5, [x10]
	stp	q6, q7, [x10, #32]
	add	x10, x10, #128
	add	x11, x11, #128
	add	x12, x12, #128
	subs	x13, x13, #32
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
	.globl	_mat_addsub_e2_w4i8             ; -- Begin function mat_addsub_e2_w4i8
	.p2align	2
_mat_addsub_e2_w4i8:                    ; @mat_addsub_e2_w4i8
	.cfi_startproc
; %bb.0:
	mul	x8, x4, x4
	ands	x9, x8, #0xffffffffffffffe0
	b.eq	LBB2_3
; %bb.1:
	add	x10, x3, #64
	add	x11, x2, #64
	add	x12, x1, #64
	add	x13, x0, #64
	mov	x14, x9
LBB2_2:                                 ; =>This Inner Loop Header: Depth=1
	ldp	q0, q1, [x13, #-64]
	ldp	q2, q3, [x13, #-32]
	ldp	q4, q5, [x13]
	ldp	q6, q7, [x13, #32]
	ldp	q16, q17, [x12, #-64]
	ldp	q18, q19, [x12, #-32]
	ldp	q20, q21, [x12]
	ldp	q22, q23, [x12, #32]
	fadd.4s	v24, v0, v16
	fadd.4s	v25, v1, v17
	fadd.4s	v26, v2, v18
	fadd.4s	v27, v3, v19
	fadd.4s	v28, v4, v20
	fadd.4s	v29, v5, v21
	fadd.4s	v30, v6, v22
	fadd.4s	v31, v7, v23
	stp	q24, q25, [x11, #-64]
	stp	q26, q27, [x11, #-32]
	fsub.4s	v0, v0, v16
	fsub.4s	v1, v1, v17
	fsub.4s	v2, v2, v18
	fsub.4s	v3, v3, v19
	fsub.4s	v4, v4, v20
	fsub.4s	v5, v5, v21
	fsub.4s	v6, v6, v22
	fsub.4s	v7, v7, v23
	stp	q0, q1, [x10, #-64]
	stp	q2, q3, [x10, #-32]
	stp	q4, q5, [x10]
	stp	q6, q7, [x10, #32]
	add	x10, x10, #128
	stp	q28, q29, [x11]
	stp	q30, q31, [x11, #32]
	add	x11, x11, #128
	add	x12, x12, #128
	add	x13, x13, #128
	subs	x14, x14, #32
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
