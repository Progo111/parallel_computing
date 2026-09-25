	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 5
	.globl	_mat_add_e1_ffm                 ; -- Begin function mat_add_e1_ffm
	.p2align	2
_mat_add_e1_ffm:                        ; @mat_add_e1_ffm
	.cfi_startproc
; %bb.0:
	cbz	x3, LBB0_23
; %bb.1:
	cmp	x3, #15
	b.hi	LBB0_6
; %bb.2:
	mov	x8, #0                          ; =0x0
	lsl	x9, x3, #2
LBB0_3:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_4 Depth 2
	mov	x10, #0                         ; =0x0
LBB0_4:                                 ;   Parent Loop BB0_3 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s0, [x0, x10, lsl #2]
	ldr	s1, [x1, x10, lsl #2]
	fadd	s0, s1, s0
	str	s0, [x2, x10, lsl #2]
	add	x10, x10, #1
	cmp	x3, x10
	b.ne	LBB0_4
; %bb.5:                                ;   in Loop: Header=BB0_3 Depth=1
	add	x8, x8, #1
	add	x2, x2, x9
	add	x1, x1, x9
	add	x0, x0, x9
	cmp	x8, x3
	b.ne	LBB0_3
	b	LBB0_23
LBB0_6:
	sub	x8, x2, x0
	cmp	x8, #64
	b.lo	LBB0_13
; %bb.7:
	sub	x8, x2, x1
	cmp	x8, #63
	b.ls	LBB0_13
; %bb.8:
	and	x8, x3, #0xfffffffffffffff0
	cmp	x8, x3
	b.ne	LBB0_17
; %bb.9:
	mov	x9, #0                          ; =0x0
	add	x10, x0, #32
	lsl	x11, x3, #2
	add	x12, x2, #32
	add	x13, x1, #32
LBB0_10:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_11 Depth 2
	mov	x14, x13
	mov	x15, x12
	mov	x16, x10
	mov	x17, x8
LBB0_11:                                ;   Parent Loop BB0_10 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	q0, q1, [x16, #-32]
	ldp	q2, q3, [x16], #64
	ldp	q4, q5, [x14, #-32]
	ldp	q6, q7, [x14], #64
	fadd.4s	v0, v4, v0
	fadd.4s	v1, v5, v1
	fadd.4s	v2, v6, v2
	fadd.4s	v3, v7, v3
	stp	q0, q1, [x15, #-32]
	stp	q2, q3, [x15], #64
	subs	x17, x17, #16
	b.ne	LBB0_11
; %bb.12:                               ;   in Loop: Header=BB0_10 Depth=1
	add	x9, x9, #1
	add	x10, x10, x11
	add	x12, x12, x11
	add	x13, x13, x11
	cmp	x9, x3
	b.ne	LBB0_10
	b	LBB0_23
LBB0_13:
	mov	x8, #0                          ; =0x0
	lsl	x9, x3, #2
LBB0_14:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_15 Depth 2
	mov	x10, #0                         ; =0x0
LBB0_15:                                ;   Parent Loop BB0_14 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s0, [x0, x10, lsl #2]
	ldr	s1, [x1, x10, lsl #2]
	fadd	s0, s1, s0
	str	s0, [x2, x10, lsl #2]
	add	x10, x10, #1
	cmp	x3, x10
	b.ne	LBB0_15
; %bb.16:                               ;   in Loop: Header=BB0_14 Depth=1
	add	x8, x8, #1
	add	x2, x2, x9
	add	x1, x1, x9
	add	x0, x0, x9
	cmp	x8, x3
	b.ne	LBB0_14
	b	LBB0_23
LBB0_17:
	mov	x9, #0                          ; =0x0
	add	x10, x0, #32
	lsl	x11, x3, #2
	add	x12, x2, #32
	add	x13, x1, #32
LBB0_18:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_19 Depth 2
                                        ;     Child Loop BB0_21 Depth 2
	mov	x14, x13
	mov	x15, x12
	mov	x16, x10
	mov	x17, x8
LBB0_19:                                ;   Parent Loop BB0_18 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	q0, q1, [x16, #-32]
	ldp	q2, q3, [x16], #64
	ldp	q4, q5, [x14, #-32]
	ldp	q6, q7, [x14], #64
	fadd.4s	v0, v4, v0
	fadd.4s	v1, v5, v1
	fadd.4s	v2, v6, v2
	fadd.4s	v3, v7, v3
	stp	q0, q1, [x15, #-32]
	stp	q2, q3, [x15], #64
	subs	x17, x17, #16
	b.ne	LBB0_19
; %bb.20:                               ;   in Loop: Header=BB0_18 Depth=1
	mov	x14, x8
LBB0_21:                                ;   Parent Loop BB0_18 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s0, [x0, x14, lsl #2]
	ldr	s1, [x1, x14, lsl #2]
	fadd	s0, s1, s0
	str	s0, [x2, x14, lsl #2]
	add	x14, x14, #1
	cmp	x3, x14
	b.ne	LBB0_21
; %bb.22:                               ;   in Loop: Header=BB0_18 Depth=1
	add	x9, x9, #1
	add	x10, x10, x11
	add	x12, x12, x11
	add	x13, x13, x11
	add	x2, x2, x11
	add	x1, x1, x11
	add	x0, x0, x11
	cmp	x9, x3
	b.ne	LBB0_18
LBB0_23:
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	_mat_sub_e1_ffm                 ; -- Begin function mat_sub_e1_ffm
	.p2align	2
_mat_sub_e1_ffm:                        ; @mat_sub_e1_ffm
	.cfi_startproc
; %bb.0:
	cbz	x3, LBB1_23
; %bb.1:
	cmp	x3, #15
	b.hi	LBB1_6
; %bb.2:
	mov	x8, #0                          ; =0x0
	lsl	x9, x3, #2
LBB1_3:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB1_4 Depth 2
	mov	x10, #0                         ; =0x0
LBB1_4:                                 ;   Parent Loop BB1_3 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s0, [x0, x10, lsl #2]
	ldr	s1, [x1, x10, lsl #2]
	fsub	s0, s0, s1
	str	s0, [x2, x10, lsl #2]
	add	x10, x10, #1
	cmp	x3, x10
	b.ne	LBB1_4
; %bb.5:                                ;   in Loop: Header=BB1_3 Depth=1
	add	x8, x8, #1
	add	x2, x2, x9
	add	x1, x1, x9
	add	x0, x0, x9
	cmp	x8, x3
	b.ne	LBB1_3
	b	LBB1_23
LBB1_6:
	sub	x8, x2, x0
	cmp	x8, #64
	b.lo	LBB1_13
; %bb.7:
	sub	x8, x2, x1
	cmp	x8, #63
	b.ls	LBB1_13
; %bb.8:
	and	x8, x3, #0xfffffffffffffff0
	cmp	x8, x3
	b.ne	LBB1_17
; %bb.9:
	mov	x9, #0                          ; =0x0
	add	x10, x0, #32
	lsl	x11, x3, #2
	add	x12, x2, #32
	add	x13, x1, #32
LBB1_10:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB1_11 Depth 2
	mov	x14, x13
	mov	x15, x12
	mov	x16, x10
	mov	x17, x8
LBB1_11:                                ;   Parent Loop BB1_10 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	q0, q1, [x16, #-32]
	ldp	q2, q3, [x16], #64
	ldp	q4, q5, [x14, #-32]
	ldp	q6, q7, [x14], #64
	fsub.4s	v0, v0, v4
	fsub.4s	v1, v1, v5
	fsub.4s	v2, v2, v6
	fsub.4s	v3, v3, v7
	stp	q0, q1, [x15, #-32]
	stp	q2, q3, [x15], #64
	subs	x17, x17, #16
	b.ne	LBB1_11
; %bb.12:                               ;   in Loop: Header=BB1_10 Depth=1
	add	x9, x9, #1
	add	x10, x10, x11
	add	x12, x12, x11
	add	x13, x13, x11
	cmp	x9, x3
	b.ne	LBB1_10
	b	LBB1_23
LBB1_13:
	mov	x8, #0                          ; =0x0
	lsl	x9, x3, #2
LBB1_14:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB1_15 Depth 2
	mov	x10, #0                         ; =0x0
LBB1_15:                                ;   Parent Loop BB1_14 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s0, [x0, x10, lsl #2]
	ldr	s1, [x1, x10, lsl #2]
	fsub	s0, s0, s1
	str	s0, [x2, x10, lsl #2]
	add	x10, x10, #1
	cmp	x3, x10
	b.ne	LBB1_15
; %bb.16:                               ;   in Loop: Header=BB1_14 Depth=1
	add	x8, x8, #1
	add	x2, x2, x9
	add	x1, x1, x9
	add	x0, x0, x9
	cmp	x8, x3
	b.ne	LBB1_14
	b	LBB1_23
LBB1_17:
	mov	x9, #0                          ; =0x0
	add	x10, x0, #32
	lsl	x11, x3, #2
	add	x12, x2, #32
	add	x13, x1, #32
LBB1_18:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB1_19 Depth 2
                                        ;     Child Loop BB1_21 Depth 2
	mov	x14, x13
	mov	x15, x12
	mov	x16, x10
	mov	x17, x8
LBB1_19:                                ;   Parent Loop BB1_18 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	q0, q1, [x16, #-32]
	ldp	q2, q3, [x16], #64
	ldp	q4, q5, [x14, #-32]
	ldp	q6, q7, [x14], #64
	fsub.4s	v0, v0, v4
	fsub.4s	v1, v1, v5
	fsub.4s	v2, v2, v6
	fsub.4s	v3, v3, v7
	stp	q0, q1, [x15, #-32]
	stp	q2, q3, [x15], #64
	subs	x17, x17, #16
	b.ne	LBB1_19
; %bb.20:                               ;   in Loop: Header=BB1_18 Depth=1
	mov	x14, x8
LBB1_21:                                ;   Parent Loop BB1_18 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s0, [x0, x14, lsl #2]
	ldr	s1, [x1, x14, lsl #2]
	fsub	s0, s0, s1
	str	s0, [x2, x14, lsl #2]
	add	x14, x14, #1
	cmp	x3, x14
	b.ne	LBB1_21
; %bb.22:                               ;   in Loop: Header=BB1_18 Depth=1
	add	x9, x9, #1
	add	x10, x10, x11
	add	x12, x12, x11
	add	x13, x13, x11
	add	x2, x2, x11
	add	x1, x1, x11
	add	x0, x0, x11
	cmp	x9, x3
	b.ne	LBB1_18
LBB1_23:
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	_mat_addsub_e1_ffm              ; -- Begin function mat_addsub_e1_ffm
	.p2align	2
_mat_addsub_e1_ffm:                     ; @mat_addsub_e1_ffm
	.cfi_startproc
; %bb.0:
	cbz	x4, LBB2_31
; %bb.1:
	cmp	x4, #7
	b.hi	LBB2_11
; %bb.2:
	mov	x8, #0                          ; =0x0
	add	x9, x0, #24
	lsl	x10, x4, #2
	add	x11, x1, #24
	add	x12, x2, #24
	add	x13, x3, #12
	mov	x14, x4
	b	LBB2_4
LBB2_3:                                 ;   in Loop: Header=BB2_4 Depth=1
	add	x8, x8, x10
	subs	x14, x14, #1
	b.eq	LBB2_31
LBB2_4:                                 ; =>This Inner Loop Header: Depth=1
	add	x15, x9, x8
	ldur	s0, [x15, #-24]
	add	x16, x11, x8
	ldur	s1, [x16, #-24]
	fadd	s2, s1, s0
	add	x17, x12, x8
	stur	s2, [x17, #-24]
	fsub	s0, s0, s1
	add	x0, x13, x8
	stur	s0, [x0, #-12]
	cmp	x4, #1
	b.eq	LBB2_3
; %bb.5:                                ;   in Loop: Header=BB2_4 Depth=1
	ldur	s0, [x15, #-20]
	ldur	s1, [x16, #-20]
	fadd	s2, s1, s0
	stur	s2, [x17, #-20]
	fsub	s0, s0, s1
	stur	s0, [x0, #-8]
	cmp	x4, #2
	b.eq	LBB2_3
; %bb.6:                                ;   in Loop: Header=BB2_4 Depth=1
	add	x15, x9, x8
	ldur	s0, [x15, #-16]
	add	x16, x11, x8
	ldur	s1, [x16, #-16]
	fadd	s2, s1, s0
	add	x17, x12, x8
	stur	s2, [x17, #-16]
	fsub	s0, s0, s1
	add	x0, x13, x8
	stur	s0, [x0, #-4]
	cmp	x4, #3
	b.eq	LBB2_3
; %bb.7:                                ;   in Loop: Header=BB2_4 Depth=1
	ldur	s0, [x15, #-12]
	ldur	s1, [x16, #-12]
	fadd	s2, s1, s0
	stur	s2, [x17, #-12]
	fsub	s0, s0, s1
	str	s0, [x13, x8]
	cmp	x4, #4
	b.eq	LBB2_3
; %bb.8:                                ;   in Loop: Header=BB2_4 Depth=1
	add	x15, x9, x8
	ldur	s0, [x15, #-8]
	add	x16, x11, x8
	ldur	s1, [x16, #-8]
	fadd	s2, s1, s0
	add	x17, x12, x8
	stur	s2, [x17, #-8]
	fsub	s0, s0, s1
	add	x0, x13, x8
	str	s0, [x0, #4]
	cmp	x4, #5
	b.eq	LBB2_3
; %bb.9:                                ;   in Loop: Header=BB2_4 Depth=1
	ldur	s0, [x15, #-4]
	ldur	s1, [x16, #-4]
	fadd	s2, s1, s0
	stur	s2, [x17, #-4]
	fsub	s0, s0, s1
	str	s0, [x0, #8]
	cmp	x4, #6
	b.eq	LBB2_3
; %bb.10:                               ;   in Loop: Header=BB2_4 Depth=1
	ldr	s0, [x9, x8]
	ldr	s1, [x11, x8]
	fadd	s2, s1, s0
	str	s2, [x12, x8]
	fsub	s0, s0, s1
	add	x15, x13, x8
	str	s0, [x15, #12]
	b	LBB2_3
LBB2_11:
	sub	x8, x3, x2
	cmp	x8, #32
	b.lo	LBB2_21
; %bb.12:
	sub	x8, x2, x0
	cmp	x8, #32
	b.lo	LBB2_21
; %bb.13:
	sub	x8, x2, x1
	cmp	x8, #32
	b.lo	LBB2_21
; %bb.14:
	sub	x8, x3, x0
	cmp	x8, #32
	b.lo	LBB2_21
; %bb.15:
	sub	x8, x3, x1
	cmp	x8, #31
	b.ls	LBB2_21
; %bb.16:
	and	x8, x4, #0xfffffffffffffff8
	subs	x9, x4, x8
	b.ne	LBB2_25
; %bb.17:
	add	x10, x3, #16
	lsl	x11, x4, #2
	add	x12, x0, #16
	add	x13, x2, #16
	add	x14, x1, #16
LBB2_18:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB2_19 Depth 2
	mov	x15, x14
	mov	x16, x13
	mov	x17, x12
	mov	x0, x10
	mov	x1, x8
LBB2_19:                                ;   Parent Loop BB2_18 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	q0, q1, [x17, #-16]
	ldp	q2, q3, [x15, #-16]
	fadd.4s	v4, v2, v0
	fadd.4s	v5, v3, v1
	stp	q4, q5, [x16, #-16]
	fsub.4s	v0, v0, v2
	fsub.4s	v1, v1, v3
	stp	q0, q1, [x0, #-16]
	add	x0, x0, #32
	add	x17, x17, #32
	add	x16, x16, #32
	add	x15, x15, #32
	subs	x1, x1, #8
	b.ne	LBB2_19
; %bb.20:                               ;   in Loop: Header=BB2_18 Depth=1
	add	x9, x9, #1
	add	x10, x10, x11
	add	x12, x12, x11
	add	x13, x13, x11
	add	x14, x14, x11
	cmp	x9, x4
	b.ne	LBB2_18
	b	LBB2_31
LBB2_21:
	mov	x8, #0                          ; =0x0
	lsl	x9, x4, #2
LBB2_22:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB2_23 Depth 2
	mov	x10, #0                         ; =0x0
LBB2_23:                                ;   Parent Loop BB2_22 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s0, [x0, x10, lsl #2]
	ldr	s1, [x1, x10, lsl #2]
	fadd	s2, s1, s0
	str	s2, [x2, x10, lsl #2]
	fsub	s0, s0, s1
	str	s0, [x3, x10, lsl #2]
	add	x10, x10, #1
	cmp	x4, x10
	b.ne	LBB2_23
; %bb.24:                               ;   in Loop: Header=BB2_22 Depth=1
	add	x8, x8, #1
	add	x3, x3, x9
	add	x2, x2, x9
	add	x1, x1, x9
	add	x0, x0, x9
	cmp	x8, x4
	b.ne	LBB2_22
	b	LBB2_31
LBB2_25:
	mov	x10, #0                         ; =0x0
	add	x11, x3, #16
	lsl	x12, x4, #2
	add	x13, x2, #16
	add	x14, x0, #16
	add	x15, x1, #16
	and	x5, x12, #0xffffffffffffffe0
	add	x16, x3, x5
	add	x17, x2, x5
	add	x1, x1, x5
	add	x0, x0, x5
LBB2_26:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB2_27 Depth 2
                                        ;     Child Loop BB2_29 Depth 2
	mov	x2, x15
	mov	x3, x14
	mov	x5, x13
	mov	x6, x8
	mov	x7, x11
LBB2_27:                                ;   Parent Loop BB2_26 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	q0, q1, [x3, #-16]
	ldp	q2, q3, [x2, #-16]
	fadd.4s	v4, v2, v0
	fadd.4s	v5, v3, v1
	stp	q4, q5, [x5, #-16]
	fsub.4s	v0, v0, v2
	fsub.4s	v1, v1, v3
	stp	q0, q1, [x7, #-16]
	add	x7, x7, #32
	add	x5, x5, #32
	add	x3, x3, #32
	add	x2, x2, #32
	subs	x6, x6, #8
	b.ne	LBB2_27
; %bb.28:                               ;   in Loop: Header=BB2_26 Depth=1
	mov	x2, #0                          ; =0x0
LBB2_29:                                ;   Parent Loop BB2_26 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s0, [x0, x2, lsl #2]
	ldr	s1, [x1, x2, lsl #2]
	fadd	s2, s1, s0
	str	s2, [x17, x2, lsl #2]
	fsub	s0, s0, s1
	str	s0, [x16, x2, lsl #2]
	add	x2, x2, #1
	cmp	x9, x2
	b.ne	LBB2_29
; %bb.30:                               ;   in Loop: Header=BB2_26 Depth=1
	add	x10, x10, #1
	add	x11, x11, x12
	add	x13, x13, x12
	add	x14, x14, x12
	add	x15, x15, x12
	add	x16, x16, x12
	add	x17, x17, x12
	add	x1, x1, x12
	add	x0, x0, x12
	cmp	x10, x4
	b.ne	LBB2_26
LBB2_31:
	ret
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols
