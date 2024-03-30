; vasmm68k_mot -Fbin -L out.txt -o bootrom.bin bootrom.s

	org	$f00000

	jmp	(a5)

tag:	dc.w	$4afc
	dc.l	tag
	dc.l	end
	dc.b	1
	dc.b	1	; version
	dc.b	0
	dc.b	-35	; prio
	dc.l	name
	dc.l	name
	dc.l	init
name:	dc.b	"jump cut",0
	even
init:
	suba.l	a0,a0
	move.l	$0.w,d7
	asl.l	#2,d7
	move.l	d7,a3
	moveq.l #0,d0
	jmp	4(a3)

end:
