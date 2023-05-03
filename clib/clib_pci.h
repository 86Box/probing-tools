/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          This file is part of the 86Box Probing Tools distribution.
 *
 *          Definitions for the common library for C-based tools.
 *
 *
 *
 * Authors: RichardG, <richardg867@gmail.com>
 *
 *          Copyright 2021 RichardG.
 *
 */
#ifndef CLIB_PCI_H
#define CLIB_PCI_H
#include "clib.h"

#if defined(__GNUC__) && !defined(__POSIX_UEFI__)
#include <pci/pci.h>
static inline void libpci_init(struct pci_access *pacc) { pci_init(pacc); }
static inline void libpci_scan_bus(struct pci_access *pacc) { pci_scan_bus(pacc); }
#define pci_init pci_init_
#define pci_scan_bus pci_scan_bus_
#endif

/* Global variables. */
extern uint8_t pci_mechanism, pci_device_count;

/* Configuration functions. */
extern uint32_t pci_cf8(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
extern uint16_t pci_get_io_bar(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint16_t size, const char *name);
#ifdef IS_32BIT
extern uint32_t pci_get_mem_bar(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t size, const char *name);
#endif
extern int      pci_init();
extern uint8_t  pci_readb(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
extern uint16_t pci_readw(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
extern uint32_t pci_readl(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
extern void     pci_writeb(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint8_t val);
extern void     pci_writew(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint16_t val);
extern void     pci_writel(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t val);
extern void     pci_scan_bus(uint8_t bus,
                             void (*callback)(uint8_t bus, uint8_t dev, uint8_t func,
                                          uint16_t ven_id, uint16_t dev_id));

#endif
