; /opt/amiga/bin/vasmm68k_mot -Fbin -L out.txt -o bootrom.bin bootrom.s -no-opt -cnop=0 -I /opt/amiga/m68k-amigaos/ndk-include

	include "exec/resident.i"
	include "exec/nodes.i"
	include "exec/libraries.i"

	include "lvo/exec_lib.i"

	org	$f10000

JUMP_CUT

.tag:	dc.w	RTC_MATCHWORD
		dc.l	.tag
		dc.l	.end
		dc.b	RTF_COLDSTART
		dc.b	1				; version
		dc.b	NT_UNKNOWN
		dc.b	-40				; prio
		dc.l	.name
		dc.l	.id
		dc.l	.init
.name:	dc.b	"jump cut",0
.id:	dc.b	"jump cut 1.0 (1.1.2024)",$d,$a,0
		even
		cnop 	0,4
.init:
		jsr		$f0ff90
		move.l	d0,d7
		beq.b	.nopayload

		suba.l	a0,a0
		asl.l	#2,d7
		move.l	d7,a3
		moveq.l #0,d0
		jmp		4(a3)

.nopayload:
		rts

.end:

		cnop 	0,16

DOS:

.tag:	dc.w	RTC_MATCHWORD
		dc.l	.tag
		dc.l	.end
		dc.b	RTF_COLDSTART
		dc.b	99	; version
		dc.b	9	; nt_library
		dc.b	0	; prio
		dc.l	.name
		dc.l	.id
		dc.l	.init

.name:	dc.b	"dos.library",0
.id:	dc.b	"dos 99 (1.1.2099)",$d,$a,0
		even

		cnop 	0,4

.init:
		movem.l	d2-d7/a2-a6,-(sp)
		move.l	$4.w,a6
		lea		.func(pc),a0
		suba.l	a1,a1
		suba.l	a2,a2
		move.l	#LIB_SIZE,d0
		jsr		_LVOMakeLibrary(a6)
		move.l	d0,d2
		beq.b	.fail
		movea.l	d0,a1

		move.b	#NT_LIBRARY,LN_TYPE(a1)
		move.l	#.name,LN_NAME(a1)
		move.b	#LIBF_SUMUSED|LIBF_CHANGED,LIB_FLAGS(a1)
		move.w	#99,LIB_VERSION(a1)
		move.w	#99,LIB_REVISION(a1)
		move.l	#.id,LIB_IDSTRING(a1)

		jsr		_LVOAddLibrary(a6)

.fail	move.l	d2,d0
		movem.l	(sp)+,d2-d7/a2-a6
		rts

.libopen
		addq.w	#1,LIB_OPENCNT(a6)
		move.l	a6,d0
		rts

.libclose
		subq.w	#1,LIB_OPENCNT(a6)
		moveq	#0,d0
		rts

.dummy	move.l	#0,d0
		rts

.shortfunc	macro
			dc.w	(\1-.func)
			endm

.func	
		dc.w  -1
		.shortfunc	.libopen
		.shortfunc	.libclose
		.shortfunc	.dummy		; expunge
		.shortfunc	.dummy		; null

		rept 100
		.shortfunc	.dummy
		endr
		dc.w	-1

.end:
