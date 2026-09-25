	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 5
	.globl	_mat_add_e3_u2                  ; -- Begin function mat_add_e3_u2
	.p2align	2
_mat_add_e3_u2:                         ; @mat_add_e3_u2
	.cfi_startproc
; %bb.0:
	mul	x8, x3, x3
	cmp	x8, #8
	b.hs	LBB0_2
; %bb.1:
	mov	x13, #0                         ; =0x0
	b	LBB0_4
LBB0_2:
	mov	x10, #0                         ; =0x0
	add	x9, x0, #16
	add	x11, x1, #16
	add	x12, x2, #16
LBB0_3:                                 ; =>This Inner Loop Header: Depth=1
	ldp	q0, q1, [x9, #-16]
	ldp	q2, q3, [x11, #-16]
	fadd.4s	v0, v0, v2
	fadd.4s	v1, v1, v3
	stp	q0, q1, [x12, #-16]
	add	x9, x9, #32
	add	x11, x11, #32
	add	x12, x12, #32
	add	x13, x10, #8
	add	x14, x10, #16
	mov	x10, x13
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
	.globl	_mat_sub_e3_u2                  ; -- Begin function mat_sub_e3_u2
	.p2align	2
_mat_sub_e3_u2:                         ; @mat_sub_e3_u2
	.cfi_startproc
; %bb.0:
	mul	x8, x3, x3
	cmp	x8, #8
	b.hs	LBB1_2
; %bb.1:
	mov	x13, #0                         ; =0x0
	b	LBB1_4
LBB1_2:
	mov	x10, #0                         ; =0x0
	add	x9, x0, #16
	add	x11, x1, #16
	add	x12, x2, #16
LBB1_3:                                 ; =>This Inner Loop Header: Depth=1
	ldp	q0, q1, [x9, #-16]
	ldp	q2, q3, [x11, #-16]
	fsub.4s	v0, v0, v2
	fsub.4s	v1, v1, v3
	stp	q0, q1, [x12, #-16]
	add	x9, x9, #32
	add	x11, x11, #32
	add	x12, x12, #32
	add	x13, x10, #8
	add	x14, x10, #16
	mov	x10, x13
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
	.globl	_mat_addsub_e3_u2               ; -- Begin function mat_addsub_e3_u2
	.p2align	2
_mat_addsub_e3_u2:                      ; @mat_addsub_e3_u2
	.cfi_startproc
; %bb.0:
	mul	x8, x4, x4
	cmp	x8, #8
	b.hs	LBB2_2
; %bb.1:
	mov	x9, #0                          ; =0x0
	subs	x8, x8, x9
	b.hi	LBB2_5
	b	LBB2_7
LBB2_2:
	add	x9, x0, #16
	add	x10, x1, #16
	add	x11, x3, #16
	add	x13, x2, #16
	mov	w12, #8                         ; =0x8
LBB2_3:                                 ; =>This Inner Loop Header: Depth=1
	ldp	q0, q1, [x9, #-16]
	ldp	q2, q3, [x10, #-16]
	fadd.4s	v4, v0, v2
	fadd.4s	v5, v1, v3
	fsub.4s	v0, v0, v2
	fsub.4s	v1, v1, v3
	stp	q4, q5, [x13, #-16]
	add	x12, x12, #8
	add	x9, x9, #32
	stp	q0, q1, [x11, #-16]
	add	x10, x10, #32
	add	x11, x11, #32
	add	x13, x13, #32
	cmp	x12, x8
	b.ls	LBB2_3
; %bb.4:
	sub	x9, x12, #8
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
