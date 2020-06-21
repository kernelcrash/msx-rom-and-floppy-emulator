

	.area _DATA


_old_isr::
	.ds 2


	.area _CODE


_install_isr::
	push ix
	ld ix,#0
	add ix,sp

	; Save old ISR vector
	ld hl,(#0x0039)
	ld (_old_isr),hl
	; Set new ISR vector
	di
	ld l,4(ix)
	ld h,5(ix)
	ld (#0x0039),hl
	ei
	
	pop ix
	ret


_uninstall_isr::
	; Restore old ISR vector
	di
	ld hl,(_old_isr)
	ld (#0x0039),hl
	ei
	ret

