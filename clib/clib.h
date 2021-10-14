/*
 * 86Box	A hypervisor and IBM PC system emulator that specializes in
 *		running old operating systems and software designed for IBM
 *		PC systems and compatibles from 1981 through fairly recent
 *		system designs based on the PCI bus.
 *
 *		This file is part of the 86Box Probing Tools distribution.
 *
 *		Definitions for the common library for C-based tools.
 *
 *
 *
 * Authors:	RichardG, <richardg867@gmail.com>
 *
 *		Copyright 2021 RichardG.
 *
 */
#ifndef WLIB_H
# define WLIB_H

/* Common macros. */
#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))
#define ABS(x)		((x) > 0 ? (x) : -(x))

/* Compiler compatibility macros. */
#ifdef __GNUC__
# define PACKED __attribute__((packed))
#else
# define PACKED
#endif


#pragma pack(push, 0)
/* Convenience type for breaking a dword value down into words and bytes. */
typedef union {
    uint8_t u8[4];
    uint16_t u16[2];
    uint32_t u32;
} multi_t;
#pragma pack(pop)


/* String functions. */
extern int	parse_hex_u8(char *val, uint8_t *dest);
extern int	parse_hex_u16(char *val, uint16_t *dest);
extern int	parse_hex_u32(char *val, uint32_t *dest);

/* Comparator functions. */
extern int	comp_ui8(const void *elem1, const void *elem2);

/* System functions. */
#ifdef __WATCOMC__
void		cli();
# pragma aux cli = "cli";
void		sti();
# pragma aux sti = "sti";
#else
extern void	cli();
extern void	sti();
#endif

/* Terminal functions. */
extern int	term_get_size_x();
extern int	term_get_size_y();
extern int	term_get_cursor_pos(uint8_t *x, uint8_t *y);
extern int	term_set_cursor_pos(uint8_t x, uint8_t y);
extern void	term_unbuffer_stdout();
extern void	term_final_linebreak();

/* Port I/O functions. */
#ifdef __WATCOMC__
uint8_t		inb(uint16_t port);
# pragma aux inb = "in al, dx" parm [dx] value [al];
void		outb(uint16_t port, uint8_t data);
# pragma aux outb = "out dx, al" parm [dx] [al];
uint16_t 	inw(uint16_t port);
# pragma aux inw = "in ax, dx" parm [dx] value [ax];
void		outw(uint16_t port, uint16_t data);
# pragma aux outw = "out dx, ax" parm [dx] [ax];
# ifdef M_I386
uint32_t	inl(uint16_t port);
#  pragma aux inl = "in eax, dx" parm [dx] value [eax];
void		outl(uint16_t port, uint32_t data);
#  pragma aux outl = "out dx, eax" parm [dx] [eax];
# else
/* Some manual prefixing trickery to perform 32-bit I/O and access
   the extended part of EAX in real mode. Exchanging is necessary
   due to Watcom ignoring the order registers are specified in... */
uint32_t	inl(uint16_t port);
#  pragma aux inl =	"db 0x66" "in ax, dx" /* in eax, dx */ \
			"mov cx, ax" \
			"db 0x66, 0xc1, 0xe8, 0x10" /* shr eax, 16 */ \
			"xchg ax, cx" \
			parm [dx] \
			value [ax cx];
void		outl(uint16_t port, uint32_t data);
#  pragma aux outl =	"xchg ax, cx" \
			"db 0x66, 0xc1, 0xe0, 0x10" /* shl eax, 16 */ \
			"mov ax, cx" \
			"db 0x66" "out dx, ax" /* out dx, eax */ \
			parm [dx] [ax cx] \
			modify [ax cx];
# endif
#else
extern uint8_t	inb(uint16_t port);
extern void	outb(uint16_t port, uint8_t data);
extern uint16_t	inw(uint16_t port);
extern void	outw(uint16_t port, uint16_t data);
extern uint32_t	inl(uint16_t port);
extern void	outl(uint16_t port, uint32_t data);
#endif

/* PCI I/O functions. */
extern uint32_t	pci_cf8(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
extern uint8_t	pci_readb(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
extern uint16_t	pci_readw(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
extern uint32_t	pci_readl(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
extern void	pci_writeb(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint8_t val);
extern void	pci_writew(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint16_t val);
extern void	pci_writel(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t val);

/* File I/O functions. */
extern void	fseek_to(FILE *f, long offset);

#endif
