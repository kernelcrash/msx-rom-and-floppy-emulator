	.area _CODE


OLD_ISR  = 0xF886  ; (3 bytes) 0xF87F fkeys 160 bytes
HOOK     = 0xFD9F

_install_isr::
	push ix
	ld ix,#0
	add ix,sp

	; Save old ISR vector
	ld A,(#HOOK)
	ld (#OLD_ISR),A
	ld HL,(#HOOK+1)
	ld (#OLD_ISR+1),HL
	; Set new ISR vector
	di
	ld A,#0xC3 ; JP
	ld (#HOOK),A
	ld L,4(ix)
	ld H,5(ix)
	ld (#HOOK+1),HL
	ei
	
	pop ix
	ret


_uninstall_isr::
	; Restore old ISR vector
	di
	ld A,(#OLD_ISR)
	ld (#HOOK),A
	ld HL,(#OLD_ISR+1)
	ld (#HOOK+1),HL
	ei
	ret

