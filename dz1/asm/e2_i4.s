	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 5
	.globl	_mat_add_e2_i4                  ; -- Begin function mat_add_e2_i4
	.p2align	2
_mat_add_e2_i4:                         ; @mat_add_e2_i4
	.cfi_startproc
; %bb.0:
	mul	x8, x3, x3
	ands	x10, x8, #0xfffffffffffffff0
	mov	x9, x10
	b.eq	LBB0_3
; %bb.1:
	add	x11, x2, #32
	add	x12, x1, #32
	add	x13, x0, #32
	mov	x14, x9
LBB0_2:                                 ; =>This Inner Loop Header: Depth=1
	ldp	q0, q1, [x13, #-32]
	ldp	q2, q3, [x13], #64
	ldp	q4, q5, [x12, #-32]
	ldp	q6, q7, [x12], #64
	fadd.4s	v0, v0, v4
	fadd.4s	v1, v1, v5
	fadd.4s	v2, v2, v6
	fadd.4s	v3, v3, v7
	stp	q0, q1, [x11, #-32]
	stp	q2, q3, [x11], #64
	subs	x14, x14, #16
	b.ne	LBB0_2
LBB0_3:
	cmp	x10, x8
	b.eq	LBB0_9
; %bb.4:
	orr	x11, x10, #0x1
	cmp	x8, x11
	csinc	x12, x8, x10, hi
	sub	x13, x12, x10
	cmp	x13, #3
	b.ls	LBB0_8
; %bb.5:
	and	x11, x12, #0x1
	sub	x9, x13, x11
	add	x9, x10, x9
	add	x10, x10, x11
	sub	x10, x10, x12
	lsl	x12, x8, #2
	and	x14, x12, #0xffffffffffffffc0
	add	x12, x0, x14
	add	x13, x1, x14
	add	x14, x2, x14
LBB0_6:                                 ; =>This Inner Loop Header: Depth=1
	ldr	q0, [x12], #16
	ldr	q1, [x13], #16
	fadd.4s	v0, v0, v1
	str	q0, [x14], #16
	adds	x10, x10, #4
	b.ne	LBB0_6
; %bb.7:
	cbz	x11, LBB0_9
LBB0_8:                                 ; =>This Inner Loop Header: Depth=1
	ldr	s0, [x0, x9, lsl #2]
	ldr	s1, [x1, x9, lsl #2]
	fadd	s0, s0, s1
	str	s0, [x2, x9, lsl #2]
	add	x9, x9, #1
	cmp	x9, x8
	b.lo	LBB0_8
LBB0_9:
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	_mat_sub_e2_i4                  ; -- Begin function mat_sub_e2_i4
	.p2align	2
_mat_sub_e2_i4:                         ; @mat_sub_e2_i4
	.cfi_startproc
; %bb.0:
	mul	x8, x3, x3
	ands	x10, x8, #0xfffffffffffffff0
	mov	x9, x10
	b.eq	LBB1_3
; %bb.1:
	add	x11, x2, #32
	add	x12, x1, #32
	add	x13, x0, #32
	mov	x14, x9
LBB1_2:                                 ; =>This Inner Loop Header: Depth=1
	ldp	q0, q1, [x13, #-32]
	ldp	q2, q3, [x13], #64
	ldp	q4, q5, [x12, #-32]
	ldp	q6, q7, [x12], #64
	fsub.4s	v0, v0, v4
	fsub.4s	v1, v1, v5
	fsub.4s	v2, v2, v6
	fsub.4s	v3, v3, v7
	stp	q0, q1, [x11, #-32]
	stp	q2, q3, [x11], #64
	subs	x14, x14, #16
	b.ne	LBB1_2
LBB1_3:
	cmp	x10, x8
	b.eq	LBB1_9
; %bb.4:
	orr	x11, x10, #0x1
	cmp	x8, x11
	csinc	x12, x8, x10, hi
	sub	x13, x12, x10
	cmp	x13, #3
	b.ls	LBB1_8
; %bb.5:
	and	x11, x12, #0x1
	sub	x9, x13, x11
	add	x9, x10, x9
	add	x10, x10, x11
	sub	x10, x10, x12
	lsl	x12, x8, #2
	and	x14, x12, #0xffffffffffffffc0
	add	x12, x0, x14
	add	x13, x1, x14
	add	x14, x2, x14
LBB1_6:                                 ; =>This Inner Loop Header: Depth=1
	ldr	q0, [x12], #16
	ldr	q1, [x13], #16
	fsub.4s	v0, v0, v1
	str	q0, [x14], #16
	adds	x10, x10, #4
	b.ne	LBB1_6
; %bb.7:
	cbz	x11, LBB1_9
LBB1_8:                                 ; =>This Inner Loop Header: Depth=1
	ldr	s0, [x0, x9, lsl #2]
	ldr	s1, [x1, x9, lsl #2]
	fsub	s0, s0, s1
	str	s0, [x2, x9, lsl #2]
	add	x9, x9, #1
	cmp	x9, x8
	b.lo	LBB1_8
LBB1_9:
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	_mat_addsub_e2_i4               ; -- Begin function mat_addsub_e2_i4
	.p2align	2
_mat_addsub_e2_i4:                      ; @mat_addsub_e2_i4
	.cfi_startproc
; %bb.0:
	mul	x8, x4, x4
	ands	x10, x8, #0xfffffffffffffff0
	mov	x9, x10
	b.eq	LBB2_3
; %bb.1:
	add	x11, x3, #32
	add	x12, x2, #32
	add	x13, x0, #32
	add	x14, x1, #32
	mov	x15, x9
LBB2_2:                                 ; =>This Inner Loop Header: Depth=1
	ldp	q0, q1, [x13, #-32]
	ldp	q2, q3, [x13], #64
	ldp	q4, q5, [x14, #-32]
	ldp	q6, q7, [x14], #64
	fadd.4s	v16, v0, v4
	fadd.4s	v17, v1, v5
	fadd.4s	v18, v2, v6
	fadd.4s	v19, v3, v7
	stp	q16, q17, [x12, #-32]
	fsub.4s	v0, v0, v4
	fsub.4s	v1, v1, v5
	fsub.4s	v2, v2, v6
	fsub.4s	v3, v3, v7
	stp	q0, q1, [x11, #-32]
	stp	q2, q3, [x11], #64
	stp	q18, q19, [x12], #64
	subs	x15, x15, #16
	b.ne	LBB2_2
LBB2_3:
	cmp	x10, x8
	b.eq	LBB2_9
; %bb.4:
	orr	x11, x10, #0x1
	cmp	x8, x11
	csinc	x12, x8, x10, hi
	sub	x13, x12, x10
	cmp	x13, #1
	b.ls	LBB2_8
; %bb.5:
	and	x11, x12, #0x1
	sub	x9, x13, x11
	add	x9, x10, x9
	add	x10, x10, x11
	sub	x10, x10, x12
	lsr	x12, x8, #4
	mov	w15, #4                         ; =0x4
	bfi	x15, x12, #6, #58
	add	x12, x0, x15
	add	x13, x1, x15
	add	x14, x2, x15
	add	x15, x3, x15
LBB2_6:                                 ; =>This Inner Loop Header: Depth=1
	ldp	s0, s1, [x12, #-4]
	ldp	s2, s3, [x13, #-4]
	fadd	s4, s0, s2
	fadd	s5, s1, s3
	stp	s4, s5, [x14, #-4]
	fsub	s0, s0, s2
	fsub	s1, s1, s3
	add	x12, x12, #8
	stp	s0, s1, [x15, #-4]
	add	x13, x13, #8
	add	x14, x14, #8
	add	x15, x15, #8
	adds	x10, x10, #2
	b.ne	LBB2_6
; %bb.7:
	cbz	x11, LBB2_9
LBB2_8:                                 ; =>This Inner Loop Header: Depth=1
	ldr	s0, [x0, x9, lsl #2]
	ldr	s1, [x1, x9, lsl #2]
	fadd	s2, s0, s1
	str	s2, [x2, x9, lsl #2]
	fsub	s0, s0, s1
	str	s0, [x3, x9, lsl #2]
	add	x9, x9, #1
	cmp	x9, x8
	b.lo	LBB2_8
LBB2_9:
	ret
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols
