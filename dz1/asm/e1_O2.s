	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 5
	.globl	_mat_add_e1_O2                  ; -- Begin function mat_add_e1_O2
	.p2align	2
_mat_add_e1_O2:                         ; @mat_add_e1_O2
	.cfi_startproc
; %bb.0:
	cbz	x3, LBB0_10
; %bb.1:
	mov	x8, #0                          ; =0x0
	mov	x9, #0                          ; =0x0
	sub	x10, x2, x0
	sub	x11, x2, x1
	cmp	x10, #64
	mov	w10, #64                        ; =0x40
	ccmp	x11, x10, #0, hs
	cset	w10, lo
	and	x11, x3, #0xfffffffffffffff0
	add	x12, x0, #32
	lsl	x13, x3, #2
	add	x14, x1, #32
	add	x15, x2, #32
	b	LBB0_3
LBB0_2:                                 ;   in Loop: Header=BB0_3 Depth=1
	add	x9, x9, #1
	add	x12, x12, x13
	add	x14, x14, x13
	add	x15, x15, x13
	add	x8, x8, x3
	cmp	x9, x3
	b.eq	LBB0_10
LBB0_3:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_6 Depth 2
                                        ;     Child Loop BB0_9 Depth 2
	cmp	x3, #16
	csinc	w16, w10, wzr, hs
	tbz	w16, #0, LBB0_5
; %bb.4:                                ;   in Loop: Header=BB0_3 Depth=1
	mov	x17, #0                         ; =0x0
	b	LBB0_8
LBB0_5:                                 ;   in Loop: Header=BB0_3 Depth=1
	mov	x16, x15
	mov	x17, x14
	mov	x4, x12
	mov	x5, x11
LBB0_6:                                 ;   Parent Loop BB0_3 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	q0, q1, [x4, #-32]
	ldp	q2, q3, [x4], #64
	ldp	q4, q5, [x17, #-32]
	ldp	q6, q7, [x17], #64
	fadd.4s	v0, v0, v4
	fadd.4s	v1, v1, v5
	fadd.4s	v2, v2, v6
	fadd.4s	v3, v3, v7
	stp	q0, q1, [x16, #-32]
	stp	q2, q3, [x16], #64
	subs	x5, x5, #16
	b.ne	LBB0_6
; %bb.7:                                ;   in Loop: Header=BB0_3 Depth=1
	mov	x17, x11
	cmp	x11, x3
	b.eq	LBB0_2
LBB0_8:                                 ;   in Loop: Header=BB0_3 Depth=1
	sub	x16, x3, x17
	add	x17, x17, x8
	lsl	x5, x17, #2
	add	x17, x2, x5
	add	x4, x1, x5
	add	x5, x0, x5
LBB0_9:                                 ;   Parent Loop BB0_3 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s0, [x5], #4
	ldr	s1, [x4], #4
	fadd	s0, s0, s1
	str	s0, [x17], #4
	subs	x16, x16, #1
	b.ne	LBB0_9
	b	LBB0_2
LBB0_10:
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	_mat_sub_e1_O2                  ; -- Begin function mat_sub_e1_O2
	.p2align	2
_mat_sub_e1_O2:                         ; @mat_sub_e1_O2
	.cfi_startproc
; %bb.0:
	cbz	x3, LBB1_10
; %bb.1:
	mov	x8, #0                          ; =0x0
	mov	x9, #0                          ; =0x0
	sub	x10, x2, x0
	sub	x11, x2, x1
	cmp	x10, #64
	mov	w10, #64                        ; =0x40
	ccmp	x11, x10, #0, hs
	cset	w10, lo
	and	x11, x3, #0xfffffffffffffff0
	add	x12, x0, #32
	lsl	x13, x3, #2
	add	x14, x1, #32
	add	x15, x2, #32
	b	LBB1_3
LBB1_2:                                 ;   in Loop: Header=BB1_3 Depth=1
	add	x9, x9, #1
	add	x12, x12, x13
	add	x14, x14, x13
	add	x15, x15, x13
	add	x8, x8, x3
	cmp	x9, x3
	b.eq	LBB1_10
LBB1_3:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB1_6 Depth 2
                                        ;     Child Loop BB1_9 Depth 2
	cmp	x3, #16
	csinc	w16, w10, wzr, hs
	tbz	w16, #0, LBB1_5
; %bb.4:                                ;   in Loop: Header=BB1_3 Depth=1
	mov	x17, #0                         ; =0x0
	b	LBB1_8
LBB1_5:                                 ;   in Loop: Header=BB1_3 Depth=1
	mov	x16, x15
	mov	x17, x14
	mov	x4, x12
	mov	x5, x11
LBB1_6:                                 ;   Parent Loop BB1_3 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	q0, q1, [x4, #-32]
	ldp	q2, q3, [x4], #64
	ldp	q4, q5, [x17, #-32]
	ldp	q6, q7, [x17], #64
	fsub.4s	v0, v0, v4
	fsub.4s	v1, v1, v5
	fsub.4s	v2, v2, v6
	fsub.4s	v3, v3, v7
	stp	q0, q1, [x16, #-32]
	stp	q2, q3, [x16], #64
	subs	x5, x5, #16
	b.ne	LBB1_6
; %bb.7:                                ;   in Loop: Header=BB1_3 Depth=1
	mov	x17, x11
	cmp	x11, x3
	b.eq	LBB1_2
LBB1_8:                                 ;   in Loop: Header=BB1_3 Depth=1
	sub	x16, x3, x17
	add	x17, x17, x8
	lsl	x5, x17, #2
	add	x17, x2, x5
	add	x4, x1, x5
	add	x5, x0, x5
LBB1_9:                                 ;   Parent Loop BB1_3 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s0, [x5], #4
	ldr	s1, [x4], #4
	fsub	s0, s0, s1
	str	s0, [x17], #4
	subs	x16, x16, #1
	b.ne	LBB1_9
	b	LBB1_2
LBB1_10:
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	_mat_addsub_e1_O2               ; -- Begin function mat_addsub_e1_O2
	.p2align	2
_mat_addsub_e1_O2:                      ; @mat_addsub_e1_O2
	.cfi_startproc
; %bb.0:
	cbz	x4, LBB2_11
; %bb.1:
	stp	x20, x19, [sp, #-16]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	.cfi_offset w19, -8
	.cfi_offset w20, -16
	mov	x8, #0                          ; =0x0
	mov	x9, #0                          ; =0x0
	sub	x10, x3, x2
	sub	x11, x2, x0
	sub	x12, x2, x1
	sub	x13, x3, x0
	sub	x14, x3, x1
	cmp	x10, #32
	mov	w10, #32                        ; =0x20
	ccmp	x11, x10, #0, hs
	ccmp	x12, x10, #0, hs
	ccmp	x13, x10, #0, hs
	ccmp	x14, x10, #0, hs
	cset	w10, lo
	and	x11, x4, #0xfffffffffffffff8
	add	x12, x3, #16
	lsl	x13, x4, #2
	add	x14, x0, #16
	add	x15, x2, #16
	add	x16, x1, #16
	b	LBB2_3
LBB2_2:                                 ;   in Loop: Header=BB2_3 Depth=1
	add	x9, x9, #1
	add	x12, x12, x13
	add	x14, x14, x13
	add	x15, x15, x13
	add	x16, x16, x13
	add	x8, x8, x4
	cmp	x9, x4
	b.eq	LBB2_10
LBB2_3:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB2_6 Depth 2
                                        ;     Child Loop BB2_9 Depth 2
	cmp	x4, #8
	csinc	w17, w10, wzr, hs
	tbz	w17, #0, LBB2_5
; %bb.4:                                ;   in Loop: Header=BB2_3 Depth=1
	mov	x5, #0                          ; =0x0
	b	LBB2_8
LBB2_5:                                 ;   in Loop: Header=BB2_3 Depth=1
	mov	x17, x16
	mov	x5, x15
	mov	x6, x14
	mov	x7, x12
	mov	x19, x11
LBB2_6:                                 ;   Parent Loop BB2_3 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldp	q0, q1, [x6, #-16]
	ldp	q2, q3, [x17, #-16]
	fadd.4s	v4, v0, v2
	fadd.4s	v5, v1, v3
	stp	q4, q5, [x5, #-16]
	fsub.4s	v0, v0, v2
	fsub.4s	v1, v1, v3
	stp	q0, q1, [x7, #-16]
	add	x7, x7, #32
	add	x6, x6, #32
	add	x5, x5, #32
	add	x17, x17, #32
	subs	x19, x19, #8
	b.ne	LBB2_6
; %bb.7:                                ;   in Loop: Header=BB2_3 Depth=1
	mov	x5, x11
	cmp	x11, x4
	b.eq	LBB2_2
LBB2_8:                                 ;   in Loop: Header=BB2_3 Depth=1
	sub	x17, x4, x5
	add	x5, x5, x8
	lsl	x19, x5, #2
	add	x5, x3, x19
	add	x6, x2, x19
	add	x7, x1, x19
	add	x19, x0, x19
LBB2_9:                                 ;   Parent Loop BB2_3 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	s0, [x19], #4
	ldr	s1, [x7], #4
	fadd	s2, s0, s1
	str	s2, [x6], #4
	fsub	s0, s0, s1
	str	s0, [x5], #4
	subs	x17, x17, #1
	b.ne	LBB2_9
	b	LBB2_2
LBB2_10:
	ldp	x20, x19, [sp], #16             ; 16-byte Folded Reload
LBB2_11:
	ret
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols
