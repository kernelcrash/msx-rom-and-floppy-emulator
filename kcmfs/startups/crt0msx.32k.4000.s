; crt0 for MSX ROM of 32KB, starting at 0x4000
; includes detection and set of ROM page 2 (0x8000 - 0xbfff)
; suggested options: --code-loc 0x4020 --data-loc 0xc000

.globl	_main
.globl	_putchar
.globl	_getchar
.globl	_cls
.globl  _initxt
.globl  _trigger_file_load_and_reset

.area _HEADER (ABS)
; Reset vector
	.org 0x4000
	.db  0x41
	.db  0x42
	.dw  init
	.dw  0x0000
	.dw  0x0000
	.dw  0x0000
	.dw  0x0000
	.dw  0x0000
	.dw  0x0000
;
;	.ascii "END ROMHEADER"
;

init:
	ld      sp,(0xfc4a) ; Stack at the top of memory.
	call    find_rom_page_2
	call    _main ; Initialise global variables
	call    #0x0000; call CHKRAM

; Ordering of segments for the linker.
	.area	_CODE

;char.c:3: void putchar(char chr) {
;	---------------------------------
; Function putchar
; ---------------------------------
_putchar_start::
_putchar:
	push ix
	ld   ix, #0
	add  ix, sp
;char.c:7: __endasm;
	ld   a, 4(ix)
	cp   a, #0x0a
	jr   nz, 2$
1$:
	ld   a, #0x0d
	call 0x00A2 ; call CHPUT
	ld   a, #0x0a
2$:
	call 0x00A2 ; call CHPUT
	pop  ix
	ret
_putchar_end::

;char.c:10: char getchar(void) {
;	---------------------------------
; Function getchar
; ---------------------------------
_getchar_start::
_getchar:
;char.c:15: __endasm;
	call 0x009F
	ld h,#0x00
	ld l,a
	ret
_getchar_end::

;char.c:10: char cls(void) {
;	---------------------------------
; Function cls
; ---------------------------------
_cls_start::
_cls:
;char.c:15: __endasm;
	call 0x00C3
	xor a
	ret
_cls_end::

;char.c:10: char initxt(void) {
;	---------------------------------
; Function initxt
; ---------------------------------
_initxt_start::
_initxt:
;char.c:15: __endasm;
	call 0x006C
	ret
_initxt_end::

_trigger_file_load_and_reset_start::
_trigger_file_load_and_reset:
;char.c:15: __endasm;
	ld a, #0x40
	ld hl, #0x8000
	ld (hl),a
	rst 00
	ret
_trigger_file_load_and_reset_end::

	.area   _GSINIT
	.area   _GSFINAL

	.area   _DATA
	.area   _BSS
	.area   _CODE

; ------------------------------------------
; Special RLE decoder used for initing global data

__initrleblock::
	; Pull the destination address out
	ld      c,l
	ld      b,h
	; Pop the return address
	pop     hl
1$:
	; Fetch the run
	ld      e,(hl)
	inc     hl
	; Negative means a run
	bit     7,e
	jp      z,2$
	; Code for expanding a run
	ld      a,(hl)
	inc     hl
3$:
	ld      (bc),a
	inc     bc
	inc     e
	jp      nz,3$
	jp      1$
2$:
	; Zero means end of a block
	xor     a
	or      e
	jp      z,4$
	; Code for expanding a block
5$:
	ld      a,(hl)
	inc     hl
	ld      (bc),a
	inc     bc
	dec     e
	jp      nz,5$
	jp      1$
4$:
	; Push the return address back onto the stack
	push    hl
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

find_rom_page_2::
	ld hl, #0x4000
	ld b, (hl)
	xor a
	ld (hl), a
	ld a, (hl)
	or a
	jr nz,5$ ; jr nz,@@ROM
	; El programa esta en RAM - no buscar
	ld (hl),b
	ret
5$: ; ----------- @@ROM:
	di
	; Slot primario
	call #0x0138 ; call RSLREG
	rrca
	rrca
	and #0x03
	; Slot secundario
	ld c, a
	ld hl, #0xfcc1
	add a, l
	ld l, a
	ld a, (hl)
	and #0x80
	or c
	ld c, a
	inc l
	inc l
	inc l
	inc l
	ld a, (hl)
	; Definir el identificador de slot
	and #0x0c
	or c
	ld h, #0x80
	; Habilitar permanentemente
	call #0x0024 ; call ENASLT
	ei
	ret
;------------------------------------------------

	.area   _GSINIT

gsinit::
	.area   _GSFINAL
	ret
