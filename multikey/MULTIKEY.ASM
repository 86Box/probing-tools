;
; 86Box		A hypervisor and IBM PC system emulator that specializes in
;		running old operating systems and software designed for IBM
;		PC systems and compatibles from 1981 through fairly recent
;		system designs based on the PCI bus.
;
;		This file is part of the 86Box Probing Tools distribution.
;
;		Phoenix MultiKey keyboard controller probing tool.
;
;
;
; Authors:	Miran Grca, <mgrca8@gmail.com>
;
;		Copyright 2021 Miran Grca.
;

.model tiny
.286
.code

start:
	jmp	real_start

store_erl:
	push	ax
	mov	al,[erl]
	mov	[di],al
	inc	di
	inc	word [dmp_sz]
	pop	ax
	ret

store_cmd:
	mov	[di],al
	inc	di
	inc	word [dmp_sz]
	ret

open_file:
	mov	ah,3ch
	mov	cx,0020h
	mov	dx,offset filename
	int	21h
	jnc	short open_file_succ
	mov	byte [erl],9
	jmp	short open_file_ret
open_file_succ:
	mov	[fileh],ax
open_file_ret:
	ret

write_file:
	mov	ah,40h
	mov	bx,[fileh]
	int	21h
	jnc	short write_file_ret
	mov	byte [erl],10
write_file_ret:
	ret

close_file:
	mov	ah,3eh
	mov	bx,[fileh]
	int	21h
	jnc	short close_file_succ
	mov	byte [erl],11
	jmp	short close_file_ret
close_file_succ:
	mov	word [fileh],0
close_file_ret:
	ret

wait_for_nibf:
	xor	cx,cx
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
	or	al,al
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

cmd_d5:
	mov	al,0d5h
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jnz	short cmd_d5_stuck
	call	wait_for_obf
	jz	short cmd_d5_no_out
	in	al,60h
	mov	[si],al
	inc	si
	call	wait_for_obf
	jz	short cmd_d5_no_out
	in	al,60h
	mov	[si],al
	mov	byte [erl],0
	jmp	short cmd_d5_ret
cmd_d5_no_out:
	mov	byte [erl],3
	jmp	short cmd_d5_ret
cmd_d5_stuck:
	mov	byte [erl],2
cmd_d5_ret:
	popa
	call	store_erl
	ret

cmd_d6:
	mov	al,0d6h
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jnz	short cmd_d6_stuck
	call	wait_for_obf
	jz	short cmd_d6_no_out
	in	al,60h
	mov	[si],al
	inc	si
	call	wait_for_obf
	jz	short cmd_d6_no_out
	in	al,60h
	mov	[si],al
	mov	byte [erl],0
	jmp	short cmd_d6_ret
cmd_d6_no_out:
	mov	byte [erl],5
	jmp	short cmd_d6_ret
cmd_d6_stuck:
	mov	byte [erl],4
cmd_d6_ret:
	popa
	call	store_erl
	ret

cmd_d7:
	mov	al,0d7h
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jnz	short cmd_d7_stuck
	call	wait_for_obf
	jz	short cmd_d7_no_out
	in	al,60h
	mov	[si],al
	inc	si
	call	wait_for_obf
	jz	short cmd_d7_no_out
	in	al,60h
	mov	[si],al
	inc	si
	call	wait_for_obf
	jz	short cmd_d7_no_out
	in	al,60h
	mov	[si],al
	mov	byte [erl],0
	jmp	short cmd_d7_ret
cmd_d7_no_out:
	mov	byte [erl],7
	jmp	short cmd_d7_ret
cmd_d7_stuck:
	mov	byte [erl],6
cmd_d7_ret:
	popa
	call	store_erl
	ret

disable_kbd:
	mov	al,0adh
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jz	short dkbd_ret
	mov	byte [erl],1
dkbd_ret:
	popa
	call	store_erl
	ret

enable_kbd:
	mov	al,0aeh
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jz	short ekbd_ret
	mov	byte [erl],8
ekbd_ret:
	popa
	call	store_erl
	ret

print_out:
	mov	si,[erl]
	shl	si,1
	add	si,offset strs
	mov	dx,[si]
	call	print_string
	ret

dmp_sz	dw	0
dmp_arr	dup	10, 0
d5_sz	dw	2
d5_data	dup	2, 0
d6_sz	dw	2
d6_data	dup	2, 0
d7_sz	dw	3
d7_data	dup	3, 0
fileh	dw	0000h
erl	dw	0
; String table organized by error level
strs	dw	offset err_00,offset err_01,offset err_02,offset err_03
	dw	offset err_04,offset err_05,offset err_06,offset err_07
	dw	offset err_08,offset err_09,offset err_0a,offset err_0b

err_00	db	"Phoenix MultiKey/42 series detected",0dh,0ah,'$'
err_01	db	"Disable Keyboard command stuck",0dh,0ah,'$'
err_02	db	"Read MultiKey Code Revision Level command stuck",0dh,0ah,'$'
err_03	db	"Read MultiKey Code Revision Level command not returning bytes",0dh,0ah,'$'
err_04	db	"Read Version Information command stuck",0dh,0ah,'$'
err_05	db	"Read Version Information command not returning bytes",0dh,0ah,'$'
err_06	db	"Read MultiKey Model Numbers command stuck",0dh,0ah,'$'
err_07	db	"Read MultiKey Model Numbers command not returning bytes",0dh,0ah,'$'
err_08	db	"Enable Keyboard command stuck",0dh,0ah,'$'
err_09	db	"Error opening file "
filename
	db	"MULTIKEY.BIN",0,0dh,0ah,'$'
err_0a	db	"Error writing file",0dh,0ah,'$'
err_0b	db	"Error closing file",0dh,0ah,'$'

;лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
;
;                       External Entry Point
;
;лллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

real_start:
	; First round of tests
	cli
	mov	di,offset dmp_arr
	call	disable_kbd
	mov	si,offset d5_data
	call	cmd_d5
	mov	si,offset d6_data
	call	cmd_d6
	mov	si,offset d7_data
	call	cmd_d7
	call	enable_kbd
	; 06 - Enable Keyboard
	sti
	call	print_out
	call	open_file
	mov	cx,2
	mov	dx,offset dmp_sz
	call	write_file
	mov	cx,[dmp_sz]
	mov	dx,offset dmp_arr
	call	write_file
	mov	cx,4
	mov	dx,offset d5_sz
	call	write_file
	mov	cx,4
	mov	dx,offset d6_sz
	call	write_file
	mov	cx,5
	mov	dx,offset d7_sz
	call	write_file
	call	close_file
	mov	al,[erl]
errorlevel_not_0:
	mov	ah,4ch
	int	21h

.end start

end
