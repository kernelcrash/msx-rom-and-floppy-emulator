; ./asmz80 -e -w -l -o -s37 -- msx-cart.asm
;  srec_cat msx-cart.asm.s37 -offset -16384 -o msx-cart.bin -binary

initxt	equ	$006c
chsns	equ	$009c
chget	equ	$009f
chput	equ	$00a2
cls	equ	$00c3
gtstck	equ	$00d5
gttrig	equ	$00d8

; menu interface
cmd_reg		equ	$8000
fname_count	equ	$8002
fname_to_load	equ	$8080
fname_list	equ	$8100



ITEMS_PER_PAGE	equ	20
MAX_PAGES	equ	9

; on stack
frame_size	equ	32
page_size	equ	frame_size-3	; byte
page_left	equ	page_size-1	; byte
page_number	equ	page_left-1	; byte
redraw		equ	page_number-1	; byte
saved_fname_count equ	redraw-1
page_top	equ	saved_fname_count-2 ; 16 bit word


; ROM (pages 1 and 2)
	org	$4000

; MSX cartridge (ROM) header
	db	"AB"		; ROM Catridge ID ("AB")
	dw	CARTRIDGE_INIT	; INIT
	dw	$0000		; STATEMENT
	dw	$0000		; DEVICE
	dw	$0000		; TEXT
	ds	$4010 - $, $00	; Reserved

; 
header1:	db	"== Kernelcrash MSX File Selector ==\r\n",0
header2:	db	"===================================\r\n",0
footer_part1:	db	"<<<<< PAGE ",0
footer_part2:	db	" >>>>>   (? - help) ",0

help_screen	db	" ?       - this help page\r\n\r\n"
		db	" A - Z   - select a rom or dsk\r\n"
        	db	"           and reboot on to it\r\n\r\n"
        	db	" 1 - 9   - select a page of files\r\n\r\n",0

safe_chget:
	push	bc
	push	de
	push	hl
	push	ix
	xor	a
	call	chget
	pop	ix
	pop	hl
	pop	de
	pop	bc
	ret

safe_chput:
	push	ix
	call	chput
	pop	ix
	ret

safe_chput_and_wait:
	call	safe_chput
	push	af
	push	de
	ld	de,30000
@1:	dec	de
	ld	a,d
	or	e
	jr	nz,@1
	pop	de
	pop	af
	ret

; it will never return, but anyway
trigger_file_load_and_reset:
        ld a, $40
        ld hl, cmd_reg
        ld (hl),a
        rst 00
        ret


; hl points to the null ending string
print_string:
	ld	a,(hl)
	or 	a
	jr	z,print_string_done
	call	safe_chput
	inc	hl
	jr	print_string
print_string_done:
	ret


clear:
	push	ix
	xor	a
	call	cls
	pop	ix
	ret

print_help:
	call	clear
	ld	hl,header1
	call	print_string
	ld	hl,header2
	call	print_string
	ld	hl,help_screen
	call	print_string
	call	safe_chget
	ret

	

;------------------------------------------------
; find_rom_page_2
; original name     : LOCALIZAR_SEGUNDA_PAGINA
; Original author   : Eduardo Robsy Petrus
; Snippet taken from: http://karoshi.auic.es/index.php?topic=117.msg1465
;
; Rutina que localiza la segunda pagina de 16 KB
; de una ROM de 32 KB ubicada en 4000h
; Basada en la rutina de Konami-
; Compatible con carga en RAM
; Compatible con expansores de slots
;------------------------------------------------
; Comprobacion de RAM/ROM

find_rom_page_2:
        ld hl, $4000
        ld b, (hl)
        xor a
        ld (hl), a
        ld a, (hl)
        or a
        jr nz,@5 ; jr nz,@@ROM
        ; El programa esta en RAM - no buscar
        ld (hl),b
        ret
@5: 
        di
        ; Slot primario
        call $0138 ; call RSLREG
        rrca
        rrca
        and $03
        ; Slot secundario
        ld c, a
        ld hl, $fcc1
        add a, l
        ld l, a
        ld a, (hl)
        and $80
        or c
        ld c, a
        inc l
        inc l
        inc l
        inc l
        ld a, (hl)
        ; Definir el identificador de slot
        and $0c
        or c
        ld h, $80
        ; Habilitar permanentemente
        call $0024 ; call ENASLT
        ei
	ret

; Cartridge entry point
CARTRIDGE_INIT:
	;
	; YOUR CODE (ROM) GOES HERE
	;
; bit of a stack frame
	call	find_rom_page_2
	call	initxt

	ld	hl,-frame_size
	add	hl,sp
	ld	sp,hl
	ld	ix,0
	add	ix,sp
	ld	a,ITEMS_PER_PAGE
	ld	(ix+page_size),a
	ld	a,$1
	ld	(ix+page_number),a
	ld	(ix+redraw),a

; uncomment this to simplify debugging in an emulator
	;jp	SKIP

; Talk to the stm32f4
	ld	hl,cmd_reg
	ld	a,$80		; trigger directory load
	ld	(hl),a

; delay before checking that the directory is ready
	ld	de,5000
@10:	dec	de
	ld	a,d
	or	e
	jr	nz,@10

@3:	ld	a,(hl)
	rlca
	jr	nc,@1
; delay
	ld	de,1000
@2:	dec	de
	ld	a,d
	or	e
	jr	nz,@2
	jr	@3

@1:	ld	hl,fname_count
	ld	a,(hl)
	ld	(ix+saved_fname_count),a
SKIP:
loop1:	ld	a,(ix+redraw)
	or	a
	jr	nz,draw
	jp	post_redraw
draw:
	call	clear

	ld	hl,header1
	call	print_string
	ld	hl,header2
	call	print_string
	ld	b,(ix+page_size)
	ld	(ix+page_left),b
	ld	hl,0
	ld	e,(ix+page_size)	; ie 20
	ld	d,0
	ld	a,(ix+page_number)
@2:	dec	a
	jr	z,@1
	add	hl,de
	jr	@2
@1:
; hl will be the offset into the pages (0, 20 , 40 etc)
; multiply by 128
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl
	add	hl,hl

	ld	de,$8100
	add	hl,de		; hl should point to the first filename at the top of a page
	ld	(ix+page_top),l
	ld	(ix+page_top+1),h
	
	ld	c,0
@5:
	ld	a,c
	add	a,'a'
	call	safe_chput
	ld	a,'-'
	call	safe_chput

	ld	d,h
	ld	e,l		;ld	de,hl		; backup the start of line ptr
	push	bc
	ld	b,34
@4:	ld	a,(hl)
	or	a
	jr	z,@3
	dec	b
	jr	z,@3
	call	safe_chput
	inc	hl
	jr	@4
; end of the line
@3:	pop	bc
	ld	a,'\r'
	call	safe_chput
	ld	a,'\n'
	call	safe_chput
	ld	hl,$0080
	add	hl,de		; hl should now be the next line
	inc	c		; 0 is 'a', 1 is 'b' etc
	ld	a,(ix+page_left)
	dec	a
	ld	(ix+page_left),a
	jr	nz,@5

; footer
	ld	hl,footer_part1
	call	print_string
	ld	a,(ix+page_number)
	add	a,'0'
	call	safe_chput
	ld	hl,footer_part2
	call	print_string


post_redraw:
	call	safe_chget
	cp	'a'
	jr	c,@1
	cp	't'+1
	jr	nc,@1
	call	safe_chput
; Must be a letter
	sub	'a'
; keep adding 128 to the top of page pointer until we get to our file
	ld	l,(ix+page_top)
	ld	h,(ix+page_top+1)
	ld	de,128

@3:	or	a
	jr	z,@2
	add	hl,de
	dec	a
	jr	@3
; now copy the filename
@2:	ld	de,fname_to_load

	push	bc
	ld	b,127
@5:	ld	a,(hl)
	ld	(de),a
	or	a
	jr	z,@4
	inc	hl
	inc	de
	dec	b
	jr	nz,@5
	ld	a,0
	ld	(de),a		; force null end

@4:	pop	bc
	;ld	a,'z'
	;call	safe_chput
	jp	trigger_file_load_and_reset



@1:	; check some other chars
	cp	'0'
	jr	c,@6
	cp	'9'+1
	jr	nc,@6
	sub	'0'
	ld	(ix+page_number),a
	jp	loop1
@6:	
	cp	$1d
	jr	nz,@7
	ld	a,(ix+page_number)
	sub	$1
	jr	nz,@8
	ld	a,$1
@8:	ld	(ix+page_number),a
	jp	loop1
@7:	cp	$1c
	jr	nz,@9
	ld	a,(ix+page_number)
	add	a,1
	cp	10
	jr	nz,@10
	ld	a,9
@10:	ld	(ix+page_number),a
	jp	loop1

@9	
	cp	'?'
	jr	nz,@11
	call	print_help

@11:	cp	'/'
	jr	nz,@12
	call	print_help
@12:	jp	loop1	

	; never gets here
	call	chget
	ret


	org	$8000

; this is just to make sure that $8000 to the Z80 is non zero
always_false	db	$ff

	org	$bfff

; this is just so we dont have to pad the file
the_end		db	$ff

