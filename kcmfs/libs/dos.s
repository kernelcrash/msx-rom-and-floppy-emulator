	.area _DATA


_last_error::
	.ds 1


	.area _CODE


_open::
	push ix
	ld ix,#0
	add ix,sp

	; FCB pointer
	ld e,4(ix)
	ld d,5(ix)
	; open file
	ld c,#0x0F
	call 5
	ld (_last_error),a
	ld l,a

	pop ix
	ret


_creat::
	push ix
	ld ix,#0
	add ix,sp

	; FCB pointer
	ld e,4(ix)
	ld d,5(ix)
	; create file
	ld c,#0x16
	call 5
	ld (_last_error),a
	ld l,a

	pop ix
	ret


_close::
	push ix
	ld ix,#0
	add ix,sp

	; FCB pointer
	ld e,4(ix)
	ld d,5(ix)
	; close file
	ld c,#0x10
	call 5
	ld (_last_error),a
	ld l,a

	pop ix
	ret



_block_read::
	push ix
	ld ix,#0
	add ix,sp

	; sequential read
	; FCB pointer
	ld e,4(ix)
	ld d,5(ix)
	ld c,#0x14
	call 5
	ld (_last_error),a
	ld l,a

	pop ix
	ret


_block_set_data_ptr::
	push ix
	ld ix,#0
	add ix,sp

	ld e,4(ix)
	ld d,5(ix)
	ld c,#0x1A
	call 5
	ld (_last_error),a
	ld l,a

	pop ix
	ret


_block_write::
	push ix
	ld ix,#0
	add ix,sp

	; sequential write
	; FCB pointer
	ld e,4(ix)
	ld d,5(ix)
	ld c,#0x15
	call 5
	ld (_last_error),a
	ld l,a

	pop ix
	ret


_exit::
	push ix
	ld ix,#0
	add ix,sp

	ld b,4(ix)
	ld c,#0x62
	call 5

	pop ix
	ret


