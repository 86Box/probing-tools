#!/usr/bin/python3
#
# 86Box          A hypervisor and IBM PC system emulator that specializes in
#                running old operating systems and software designed for IBM
#                PC systems and compatibles from 1981 through fairly recent
#                system designs based on the PCI bus.
#
#                This file is part of the 86Box Probing Tools distribution.
#
#                Conversion tool for pcireg's binary PCI ID database file.
#
#
#
# Authors:       RichardG, <richardg867@gmail.com>
#
#                Copyright 2021 RichardG.
#
import pciutil, struct, sys

def main():
	# Load PCI ID database.
	print('Loading database...')
	pciutil.load_pci_db()

	# Start databases.
	vendor_db = b''
	device_db = b''
	class_db = b''
	string_db = struct.pack('<B', 0)
	string_db_lookup = {b'': 0}

	end_entry = struct.pack('<H', 0xffff)
	vendor_has_ffff = device_has_ffff = class_has_ffff = False

	# Enumerate device IDs, while also going through vendor IDs.
	print('Enumerating vendors and devices...')
	last_vendor = None
	for pci_id in sorted(pciutil._pci_devices):
		vendor = pciutil.clean_vendor(pciutil._pci_vendors.get(pci_id >> 16, '')).encode('cp437', 'ignore')[:256]
		if not vendor:
			continue
		elif last_vendor != vendor:
			if last_vendor != None and not device_has_ffff:
				device_db += end_entry

			string_db_pos = string_db_lookup.get(vendor, None)
			if string_db_pos == None:
				string_db_pos = string_db_lookup[vendor] = len(string_db)
				string_db += struct.pack('<B', len(vendor))
				string_db += vendor

			vendor_db += struct.pack('<HII', pci_id >> 16, len(device_db), string_db_pos)
			vendor_has_ffff = (pci_id >> 16) == 0xffff

			last_vendor = vendor

		device = pciutil.clean_device(pciutil._pci_devices[pci_id]).encode('cp437', 'ignore')[:256]

		string_db_pos = string_db_lookup.get(device, None)
		if string_db_pos == None:
			string_db_pos = string_db_lookup[device] = len(string_db)
			string_db += struct.pack('<B', len(device))
			string_db += device

		device_db += struct.pack('<HI', pci_id & 0xffff, string_db_pos)
		device_has_ffff = (pci_id & 0xffff) == 0xffff

	# Enumerate class-subclass IDs.
	print('Enumerating classes...')
	for pci_subclass in sorted(pciutil._pci_subclasses):
		class_name = pciutil._pci_subclasses[pci_subclass].encode('cp437', 'ignore')[:256]

		string_db_pos = string_db_lookup.get(class_name, None)
		if string_db_pos == None:
			string_db_pos = string_db_lookup[class_name] = len(string_db)
			string_db += struct.pack('<B', len(class_name))
			string_db += class_name

		class_db += struct.pack('<HI', pci_subclass, string_db_pos)
		class_has_ffff = pci_subclass == 0xffff

	# Add ffff end entry to the databases if required.
	if not device_has_ffff:
		device_db += end_entry
	if not vendor_has_ffff:
		vendor_db += end_entry
	if not class_has_ffff:
		class_db += end_entry

	# Write binary file.
	print('Writing binary database...')
	f = open('PCIIDS.BIN', 'wb')
	f.write(struct.pack('<III',
		12 + len(vendor_db), # device DB offset
		12 + len(vendor_db) + len(device_db), # class DB offset
		12 + len(vendor_db) + len(device_db) + len(class_db) # string DB offset
	))
	f.write(vendor_db)
	f.write(device_db)
	f.write(class_db)
	f.write(string_db)
	f.close()

if __name__ == '__main__':
	main()
