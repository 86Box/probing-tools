;
; 86Box		A hypervisor and IBM PC system emulator that specializes in
;		running old operating systems and software designed for IBM
;		PC systems and compatibles from 1981 through fairly recent
;		system designs based on the PCI bus.
;
;		This file is part of the 86Box Probing Tools distribution.
;
;		AMIKEY keyboard controller probing tool.
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
	call	write_everything
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
	mov	byte [erl],17
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
	mov	byte [erl],18
write_file_ret:
	ret

close_file:
	mov	ah,3eh
	mov	bx,[fileh]
	int	21h
	jnc	short close_file_succ
	mov	byte [erl],19
	jmp	short close_file_ret
close_file_succ:
	mov	word [fileh],0
close_file_ret:
	ret

write_everything:
	pusha
	call	open_file
	mov	cx,2
	mov	dx,offset dmp_sz
	call	write_file
	mov	cx,[dmp_sz]
	mov	dx,offset dmp_arr
	call	write_file
	mov	cx,2
	mov	dx,offset copr_sz
	call	write_file
	mov	cx,[copr_sz]
	mov	dx,offset copr
	call	write_file
	mov	cx,1
	mov	dx,offset asciz
	call	write_file
	mov	cx,2
	mov	dx,offset ver_sz
	call	write_file
	mov	cx,1
	mov	dx,offset ver
	call	write_file
	mov	cx,2
	mov	dx,offset ver_sz
	call	write_file
	mov	cx,1
	mov	dx,offset stat
	call	write_file
	mov	cx,4
	mov	dx,offset cmd_ro
	call	write_file
	mov	cx,4
	mov	dx,offset cmd_ro2
	call	write_file
	mov	cx,8
	mov	dx,offset c1_sz
	call	write_file
	mov	cx,5
	mov	dx,offset ex_sz
	call	write_file
	call	close_file
	popa
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

amikey_copr:
	mov	al,0a0h
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jnz	short gcc_stuck
	mov	cx,512
	mov	si,offset copr
amikey_copr_loop:
	call	wait_for_obf
	jz	short not_amikey_copr
	in	al,60h
	cmp	al,00h
	jz	short out_of_loop
	mov	[si],al
	inc	si
	loop	amikey_copr_loop
out_of_loop:
	cmp	al,00h
	jnz	short string_too_big
	cmp	cx,512
	jz	short command_returns_nothing
	mov	byte [si],0dh
	cmp	cx,3
	jb	short string_too_big
	mov	byte [si+1],0ah
	cmp	cx,2
	jb	short string_too_big
	mov	byte [si+2],'$'
	cmp	cx,1
	jb	short string_too_big
	mov	[copr_sz],si
	sub	word [copr_sz],offset copr
	cmp	word [copr_sz],0
	jnz	not_zero
	; Zero copyright string - avoid a double new line.
	mov	byte [si],'$'
not_zero:
	mov	byte [erl],0
	jmp	short amikey_copr_ret
command_returns_nothing:
	mov	byte [si],'$'
	mov	byte [erl],5
	jmp	short amikey_copr_ret
string_too_big:
	mov	byte [erl],4
	jmp	short amikey_copr_ret
not_amikey_copr:
	mov	byte [erl],3
	jmp	short amikey_copr_ret
gcc_stuck:
	mov	byte [erl],2
amikey_copr_ret:
	popa
	call	store_erl
	ret

amikey_test:
	mov	al,0a1h
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jnz	short gcv_stuck
	call	wait_for_obf
	jz	short not_amikey
	in	al,60h
	mov	[ver],al
	mov	byte [erl],0
	jmp	short amikey_ret
not_amikey:
	mov	byte [erl],7
	jmp	short amikey_ret
gcv_stuck:
	mov	byte [erl],6
amikey_ret:
	popa
	call	store_erl
	ret

cmd_ca:
	mov	al,0cah
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jnz	short cmd_ca_stuck
	call	wait_for_obf
	jz	short cmd_ca_no_out
	in	al,60h
	mov	[si],al
	mov	byte [erl],0
	jmp	short cmd_ca_ret
cmd_ca_no_out:
	mov	byte [erl],21
	jmp	short cmd_ca_ret
cmd_ca_stuck:
	mov	byte [erl],20
cmd_ca_ret:
	popa
	call	store_erl
	ret

cmd_cb:
	mov	al,0cbh
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jnz	short cmd_cb_stuck
	mov	al,[si]
	out	60h,al
	call	wait_for_nibf
	jnz	short cmd_cb_stuck
	mov	byte [erl],0
	jmp	short cmd_cb_ret
cmd_cb_stuck:
	mov	byte [erl],22
cmd_cb_ret:
	popa
	call	store_erl
	ret

cmd_d0:
	mov	al,0d0h
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jnz	short cmd_d0_stuck
	call	wait_for_obf
	jz	short cmd_d0_no_out
	in	al,60h
	mov	[si],al
	mov	byte [erl],0
	jmp	short cmd_d0_ret
cmd_d0_no_out:
	mov	byte [erl],24
	jmp	short cmd_d0_ret
cmd_d0_stuck:
	mov	byte [erl],23
cmd_d0_ret:
	popa
	call	store_erl
	ret

cmd_d1:
	mov	al,0d1h
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jnz	short cmd_d1_stuck
	mov	al,[si]
	out	60h,al
	call	wait_for_nibf
	jnz	short cmd_d1_stuck
	mov	byte [erl],0
	jmp	short cmd_d1_ret
cmd_d1_stuck:
	mov	byte [erl],25
cmd_d1_ret:
	popa
	call	store_erl
	ret

cmd_c0:
	mov	al,0c0h
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jnz	short cmd_c0_stuck
	call	wait_for_obf
	jz	short cmd_c0_no_out
	in	al,60h
	mov	[si],al
	mov	byte [erl],0
	jmp	short cmd_c0_ret
cmd_c0_no_out:
	mov	byte [erl],29
	jmp	short cmd_c0_ret
cmd_c0_stuck:
	mov	byte [erl],28
cmd_c0_ret:
	popa
	call	store_erl
	ret

cmd_c1:
	mov	al,0c1h
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jnz	short cmd_c1_stuck
	mov	al,[si]
	out	60h,al
	call	wait_for_nibf
	jnz	short cmd_c1_stuck
	mov	byte [erl],0
	jmp	short cmd_c1_ret
cmd_c1_stuck:
	mov	byte [erl],30
cmd_c1_ret:
	popa
	call	store_erl
	ret

cmd_b2:
	mov	al,0b2h
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jnz	short cmd_b2_stuck
	call	wait_for_obf
	jz	short cmd_b2_no_out
	in	al,60h
	mov	[si],al
	mov	byte [erl],0
	jmp	short cmd_b2_ret
cmd_b2_no_out:
	mov	byte [erl],32
	jmp	short cmd_b2_ret
cmd_b2_stuck:
	mov	byte [erl],31
cmd_b2_ret:
	popa
	call	store_erl
	ret

cmd_e1:
	mov	al,0e1h
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jnz	short cmd_e1_stuck
	mov	byte [erl],0
	jmp	short cmd_e1_ret
cmd_e1_stuck:
	mov	byte [erl],26
cmd_e1_ret:
	popa
	call	store_erl
	ret

cmd_ef:
	mov	al,0efh
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jnz	short cmd_ef_stuck
	mov	byte [erl],0
	jmp	short cmd_ef_ret
cmd_ef_stuck:
	mov	byte [erl],27
cmd_ef_ret:
	popa
	call	store_erl
	ret

cmd_60:
	mov	al,60h
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jnz	short cmd_60_stuck
	mov	al,[si]
	out	60h,al
	call	wait_for_nibf
	jnz	short cmd_60_stuck
	mov	byte [erl],0
	jmp	short cmd_60_ret
cmd_60_stuck:
	mov	byte [erl],13
cmd_60_ret:
	popa
	call	store_erl
	ret

cmd_20:
	mov	al,20h
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jnz	short cmd_20_stuck
	call	wait_for_obf
	jz	short cmd_20_no_out
	in	al,60h
	mov	[si],al
	mov	byte [erl],0
	jmp	short cmd_20_ret
cmd_20_no_out:
	mov	byte [erl],10
	jmp	short cmd_20_ret
cmd_20_stuck:
	mov	byte [erl],9
cmd_20_ret:
	popa
	call	store_erl
	ret

cmd_00:
	mov	al,00h
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jnz	short cmd_00_stuck
	call	wait_for_obf
	jz	short cmd_00_no_out
	in	al,60h
	mov	[si],al
	mov	byte [erl],0
	jmp	short cmd_00_ret
cmd_00_no_out:
	mov	byte [erl],12
	jmp	short cmd_00_ret
cmd_00_stuck:
	mov	byte [erl],11
cmd_00_ret:
	popa
	call	store_erl
	ret

cmd_af:
	mov	al,0afh
	call	store_cmd
	pusha
	out	64h,al
	call	wait_for_nibf
	jnz	short cmd_af_stuck
	mov	al,[addr]
	out	60h,al
	call	wait_for_nibf
	jnz	short cmd_af_stuck
	mov	al,[si]
	out	60h,al
	call	wait_for_nibf
	jnz	short cmd_af_stuck
	mov	byte [erl],0
	jmp	short cmd_af_ret
cmd_af_stuck:
	mov	byte [erl],14
cmd_af_ret:
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

b8_stat	db	0
p1	db	0
p2	db	0
stat	db	0
nstat	db	0
new_mem	db	0
flags	db	0
nflags	db	0
dmp_sz	dw	0
dmp_arr	dup	80, 0
cmd_ro	dw	2
cmd_rd	dup	2, 0
cmd_ro2	dw	2
cmd_rd2	dup	2, 0
c1_sz	dw	6
c1_data	dup	6, 0
ex_sz	dw	3
ex_data	dup	3, 0
ver_sz	dw	1
fileh	dw	0000h
str_ptr	dw	0000h
copr_sz	dw	0000h
addr	db	20h
new_cmd	db	77h
asciz	db	0
saved_cmd
	db	0
erl	dw	0
; String table organized by error level
strs	dw	offset err_00,offset err_01,offset err_02,offset err_03
	dw	offset err_04,offset err_05,offset err_06,offset err_07
	dw	offset err_08,offset err_09,offset err_0a,offset err_0b
	dw	offset err_0c,offset err_0d,offset err_0e,offset err_0f
	dw	offset err_10,offset err_11,offset err_12,offset err_13
	dw	offset err_14,offset err_15,offset err_16,offset err_17
	dw	offset err_18,offset err_19,offset err_1a,offset err_1b
	dw	offset err_1c,offset err_1d,offset err_1e,offset err_1f
	dw	offset err_20
bytes	db	04h,08h,14h,18h,24h,28h,34h,38h,44h,48h

err_00	db	"AMIKey detected: "
ver	db	"0",0dh,0ah
copr	dup	512, 0
	db	0dh,0ah,'$'
err_01	db	"Disable Keyboard command stuck",0dh,0ah,'$'
err_02	db	"Get Copyright String command stuck",0dh,0ah,'$'
err_03	db	"Get Copyright String command not returning bytes",0dh,0ah,'$'
err_04	db	"Get Copyright String command output too long",0dh,0ah,'$'
err_05	db	"Get Copyright String command returning empty string",0dh,0ah,'$'
err_06	db	"Get Controller Version command stuck",0dh,0ah,'$'
err_07	db	"AMIKey not detected",0dh,0ah,'$'
err_08	db	"Enable Keyboard command stuck",0dh,0ah,'$'
err_09	db	"Read Controller Command Byte command stuck",0dh,0ah,'$'
err_0a	db	"Read Controller Command Byte not returning a byte",0dh,0ah,'$'
err_0b	db	"Read Controller Command Byte Alt. command stuck",0dh,0ah,'$'
err_0c	db	"Read Controller Command Byte Alt. not returning a byte",0dh,0ah,'$'
err_0d	db	"Write Controller Command Byte command stuck",0dh,0ah,'$'
err_0e	db	"Set Extended Controller RAM command stuck",0dh,0ah,'$'
err_0f	db	"Write To Keyboard Output Buffer command stuck",0dh,0ah,'$'
err_10	db	"Write To Keyboard Output Buffer command not returning a byte",0dh,0ah,'$'
err_11	db	"Error opening file "
filename
	db	"JETKEY.BIN",0,0dh,0ah,'$'
err_12	db	"Error writing file",0dh,0ah,'$'
err_13	db	"Error closing file",0dh,0ah,'$'
err_14	db	"Read Mode command stuck",0dh,0ah,'$'
err_15	db	"Read Mode command not returning a byte",0dh,0ah,'$'
err_16	db	"Write Mode command stuck",0dh,0ah,'$'
err_17	db	"Read Output Port command stuck",0dh,0ah,'$'
err_18	db	"Read Output Port command not returning a byte",0dh,0ah,'$'
err_19	db	"Write Output Port command stuck",0dh,0ah,'$'
err_1a	db	"Reset P11-P13 command stuck",0dh,0ah,'$'
err_1b	db	"Set P11-P13 command stuck",0dh,0ah,'$'
err_1c	db	"Read Input Port command stuck",0dh,0ah,'$'
err_1d	db	"Read Input Port command not returning a byte",0dh,0ah,'$'
err_1e	db	"Write Input Port command stuck",0dh,0ah,'$'
err_1f	db	"Reset P12 command stuck",0dh,0ah,'$'
err_20	db	"Reset P12 command not returning a byte",0dh,0ah,'$'

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
	; 00 - Disable Keyboard
	call	amikey_copr
	; 02 - Get Copyright String
	call	amikey_test
	; 04 - Get Controller Version
	call	enable_kbd
	; 06 - Enable Keyboard
	sti
	call	print_out
	mov	word [erl],0
	; Second round of tests
	; Patch the offset table to change the ERRORLEVEL 0 string
	mov	byte [err_00],'$'
	cli
	mov	si,offset saved_cmd
	call	cmd_20
	; 08 - Read Command Byte [SAVE]
	call	disable_kbd
	; 0A - Disable Keyboard
	mov	si,offset new_cmd
	call	cmd_af
	; 0C - Set Extended Controller RAM
	mov	si,offset cmd_rd
	call	cmd_20
	; 0E - Read Command Byte
	inc	si
	call	cmd_00
	; 10 - Read Command Byte Alt.
	call	enable_kbd
	; 12 - Enable Keyboard
	mov	si,offset saved_cmd
	call	cmd_60
	; 14 - Write Command Byte [RESTORE]
	sti
	call	print_out
	mov	word [erl],0
	; Third round of tests
	mov	byte [new_cmd],0bbh
	cli
	mov	si,offset saved_cmd
	call	cmd_20
	; 16 - Read Command Byte [SAVE]
	call	disable_kbd
	; 18 - Disable Keyboard
	mov	si,offset new_cmd
	call	cmd_af
	; 1A - Set Extended Controller RAM
	mov	si,offset cmd_rd2
	call	cmd_20
	; 1C - Read Command Byte
	inc	si
	call	cmd_00
	; 1E - Read Command Byte Alt.
	call	enable_kbd
	; 20 - Enable Keyboard
	mov	si,offset saved_cmd
	call	cmd_60
	; 22 - Write Command Byte [RESTORE]
	sti
	call	print_out
	mov	word [erl],0
	; Third round of tests
	cli
	mov	si,offset saved_cmd
	call	cmd_20
	; 24 - Read Command Byte [SAVE]
	call	disable_kbd
	; 26 - Disable Keyboard
	mov	al,[saved_cmd]
	and	al,40h		; Enable translation
	or	al,0dfh		; Disable XT mode if applicable
	mov	[new_cmd],al
	mov	si,offset new_cmd
	call	cmd_60
	; 28 - Write Command Byte

	mov	si,offset stat
	call	cmd_ca
	mov	al,[si]
	or	al,10h
	mov	[nstat],al
	mov	si,offset nstat
	call	cmd_cb

	mov	si,offset p1
	call	cmd_c0
	mov	si,offset c1_data
	mov	al,[p1]
	mov	[si],al
	inc	si
	in	al,60h
	mov	[si],al
	inc	si
	call	cmd_b2
	inc	si
	call	cmd_c0
	inc	si
	mov	al,[p1]
	mov	[si],al
	and	byte [si],0f3h
	call	cmd_c1
	call	cmd_c0
	inc	si
	mov	al,[p1]
	mov	[si],al
	or	byte [si],0ch
	call	cmd_c1
	call	cmd_c0
	mov	si,offset p1
	call	cmd_c1

	mov	si,offset p2
	call	cmd_d0
	mov	si,offset ex_data
	mov	al,[p2]
	mov	[si],al
	inc	si
	call	cmd_e1
	call	cmd_d0
	inc	si
	call	cmd_ef
	call	cmd_d0
	mov	si,offset p2
	call	cmd_d1

	mov	si,offset stat
	call	cmd_cb
	call	enable_kbd
	; AD0 - Enable Keyboard
	mov	si,offset saved_cmd
	call	cmd_60
	; AD2 - Write Command Byte [RESTORE]
	sti
	call	print_out
	call	write_everything
	mov	al,[erl]
	; db	0cch
errorlevel_not_0:
	mov	ah,4ch
	int	21h

.end start

end
