;
; 86Box		A hypervisor and IBM PC system emulator that specializes in
;		running old operating systems and software designed for IBM
;		PC systems and compatibles from 1981 through fairly recent
;		system designs based on the PCI bus.
;
;		This file is part of the 86Box Probing Tools distribution.
;
;		VIA VT82C42N keyboard controller probing tool.
;
;
;
; Authors:	Miran Grca, <mgrca8@gmail.com>
;
;		Copyright 2021 Miran Grca.
;

.model tiny
.386
.code

start:
		jmp	real_start

wait_for_nibf:
	push	ax
	mov	cx,0010h
nibf_push_cx:
	push	cx
	xor	cx,cx
nibf_in:
	in	al,64h
	test	al,2
	loopne	nibf_in
	pop	cx
	loopne	nibf_push_cx
	pop	ax
	ret

wait_for_obf:
	push	ax
	mov	cx,0010h
obf_push_cx:
	push	cx
	xor	cx,cx
obf_in:
	in	al,64h
	test	al,1
	loope	obf_in
	pop	cx
	loope	obf_push_cx
	pop	ax
	ret

print_string:
	mov	ah,9
	int	21h
	ret

convert_digit:
	and	al,0fh
	add	al,30h
	cmp	al,39h
	ja	short digit_is_letter
	jmp	short output_digit
digit_is_letter:
	add	al,7
output_digit:
	mov	[si],al
	inc	si
	ret

convert_to_hex:
	push	ax
	push	ax
	shr	al,4
	call	convert_digit
	pop	ax
	call	convert_digit
	pop	ax
	ret

viakey_test:
	xor	dx,dx
	mov	al,0afh
	out	64h,al
	call	wait_for_nibf
	jnz	gcv_stuck
	call	wait_for_obf
	jz	not_viakey
	in	al,60h
	mov	si,offset ver
	call	convert_to_hex
	mov	byte [erl],0
	jmp	short viakey_ret
not_viakey:
	mov	byte [erl],3
	jmp	short viakey_ret
gcv_stuck:
	mov	byte [erl],2
viakey_ret:
	ret

disable_kbd:
	mov	al,0adh
	out	64h,al
	call	wait_for_nibf
	jz	dkbd_ret
	mov	byte [erl],1
dkbd_ret:
	ret

enable_kbd:
	mov	al,0aeh
	out	64h,al
	call	wait_for_nibf
	jz	ekbd_ret
	mov	byte [erl],4
ekbd_ret:
	ret

print_out:
	mov	si,[erl]
	shl	si,1
	add	si,offset strs
	mov	dx,[si]
	call	print_string
	ret

new_cmd	db	57h
saved_cmd
	db	0
erl	dw	0
strs	dw	offset succ,offset err_1,offset err_2,offset err_3,offset err_4

succ	db	"VIA VT82C42N detected: "
ver	db	"0",0dh,0ah
copr	dup	512, 0
	db	0dh,0ah,'$'
err_1	db	"Disable Keyboard command stuck",0dh,0ah,'$'
err_2	db	"Get Controller Version command stuck",0dh,0ah,'$'
err_3	db	"VIA VT82C42N not detected",0dh,0ah,'$'
err_4	db	"Enable Keyboard command stuck",0dh,0ah,'$'


;лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
;
;                       External Entry Point
;
;лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

real_start:
	; First round of tests
	cli
	call	disable_kbd
	cmp	byte [erl],0
	jnz	short stop_tests
	call	viakey_test
	cmp	byte [erl],0
	jnz	short stop_tests
stop_tests:
	call	enable_kbd
	sti
	call	print_out
	mov	al,[erl]
	cmp	al,0
	mov	ah,4ch
	int	21h

.end start

end
