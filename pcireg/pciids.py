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
	vendor_db = device_db = subdevice_db = class_db = subclass_db = progif_db = string_db = b''
	vendor_devices_offset = {}
	string_db_lookup = {}
	vendor_has_termination = device_has_termination = class_has_termination = subclass_has_termination = progif_has_termination = False

	# Enumerate device IDs, while also going through subdevice IDs.
	print('Enumerating devices and subdevices...')
	current_vendor_id = None
	for pci_id in sorted(pciutil._pci_devices):
		# Check if the vendor ID has changed.
		if (pci_id >> 16) != current_vendor_id:
			# Add termination device entry if one isn't already present.
			if current_vendor_id != None and not device_has_termination:
				device_db += struct.pack('<HII', 0xffff, 0xffffffff, 0xffffffff)

			# Mark this as the current vendor ID.
			current_vendor_id = pci_id >> 16

			# Store the device entries offset for this vendor.
			vendor_devices_offset[current_vendor_id] = len(device_db)

		# Enumerate this device's subdevices.
		subdevice_db_pos = len(subdevice_db)
		subdevice_has_termination = False
		for pci_subid in sorted(pciutil._pci_subdevices.get(pci_id, {})):
			# Store a null device entries offset for this vendor if one isn't already present.
			subvendor_id = (pci_subid >> 16) & 0xffff
			if subvendor_id not in vendor_devices_offset:
				vendor_devices_offset[subvendor_id] = None

			# Look up subdevice ID.
			subdevice = pciutil.clean_device(pciutil._pci_subdevices[pci_id][pci_subid]).encode('cp437', 'ignore')[:256]

			# Add to string database if a valid result was found.
			if subdevice:
				string_db_pos = string_db_lookup.get(subdevice, None)
				if string_db_pos == None:
					string_db_pos = string_db_lookup[subdevice] = len(string_db)
					string_db += struct.pack('<B', len(subdevice))
					string_db += subdevice
			else:
				string_db_pos = 0xffffffff

			# Add to subdevice database.
			subdevice_db += struct.pack('<HHI', subvendor_id, pci_subid & 0xffff, string_db_pos)
			subdevice_has_termination = (pci_subid & 0xffff) == 0xffff

		# Mark subdevice entry if there is at least one subdevice entry.
		if len(subdevice_db) != subdevice_db_pos:
			# Add termination subdevice entry if one isn't already present.
			if not subdevice_has_termination:
				subdevice_db += struct.pack('<HHI', 0xffff, 0xffff, 0xffffffff)
		else:
			subdevice_db_pos = 0xffffffff

		# Look up device ID.
		device = pciutil.clean_device(pciutil._pci_devices[pci_id]).encode('cp437', 'ignore')[:256]

		# Add to string database if a valid result was found.
		if device:
			string_db_pos = string_db_lookup.get(device, None)
			if string_db_pos == None:
				string_db_pos = string_db_lookup[device] = len(string_db)
				string_db += struct.pack('<B', len(device))
				string_db += device
		else:
			string_db_pos = 0xffffffff

		# Add to device database.
		device_db += struct.pack('<HII', pci_id & 0xffff, subdevice_db_pos, string_db_pos)
		device_has_termination = (pci_id & 0xffff) == 0xffff

	# Enumerate vendor IDs.
	print('Enumerating vendors...')
	for vendor_id in sorted(pciutil._pci_vendors):
		# Look up vendor ID.
		vendor = pciutil.clean_vendor(pciutil._pci_vendors.get(vendor_id, '')).encode('cp437', 'ignore')[:256]

		# Add to string database if a valid result was found.
		if vendor:
			string_db_pos = string_db_lookup.get(vendor, None)
			if string_db_pos == None:
				string_db_pos = string_db_lookup[vendor] = len(string_db)
				string_db += struct.pack('<B', len(vendor))
				string_db += vendor
		else:
			string_db_pos = 0xffffffff

		# Add to vendor database.
		devices_offset = vendor_devices_offset.get(vendor_id, None)
		if devices_offset == None:
			devices_offset = 0xffffffff
		if string_db_pos != 0xffffffff or devices_offset != 0xffffffff:
			vendor_db += struct.pack('<HII', vendor_id, devices_offset, string_db_pos)
			vendor_has_termination = vendor_id == 0xffff

	# Enumerate class IDs.
	print('Enumerating classes...')
	for pci_class in sorted(pciutil._pci_classes):
		# Look up class ID.
		class_name = pciutil._pci_classes[pci_class].encode('cp437', 'ignore')[:256]

		# Add to string database if a valid result was found.
		if class_name:
			string_db_pos = string_db_lookup.get(class_name, None)
			if string_db_pos == None:
				string_db_pos = string_db_lookup[class_name] = len(string_db)
				string_db += struct.pack('<B', len(class_name))
				string_db += class_name
		else:
			string_db_pos = 0xffffffff

		# Add to class database.
		class_db += struct.pack('<BI', pci_class, string_db_pos)
		class_has_termination = pci_class == 0xff

	# Enumerate subclass IDs.
	print('Enumerating subclasses...')
	for pci_subclass in sorted(pciutil._pci_subclasses):
		# Look up class-subclass ID.
		subclass_name = pciutil._pci_subclasses[pci_subclass].encode('cp437', 'ignore')[:256]

		# Add to string database if a valid result was found.
		if subclass_name:
			string_db_pos = string_db_lookup.get(subclass_name, None)
			if string_db_pos == None:
				string_db_pos = string_db_lookup[subclass_name] = len(string_db)
				string_db += struct.pack('<B', len(subclass_name))
				string_db += subclass_name
		else:
			string_db_pos = 0xffffffff

		# Add to subclass database.
		subclass_db += struct.pack('<BBI', (pci_subclass >> 8) & 0xff, pci_subclass & 0xff, string_db_pos)
		subclass_has_termination = pci_subclass == 0xffff

	# Enumerate progif IDs.
	print('Enumerating progifs...')
	for pci_progif in sorted(pciutil._pci_progifs):
		# Look up class-subclass-progif ID.
		progif_name = pciutil._pci_progifs[pci_progif].encode('cp437', 'ignore')[:256]

		# Add to string database if a valid result was found.
		if progif_name:
			string_db_pos = string_db_lookup.get(progif_name, None)
			if string_db_pos == None:
				string_db_pos = string_db_lookup[progif_name] = len(string_db)
				string_db += struct.pack('<B', len(progif_name))
				string_db += progif_name
		else:
			string_db_pos = 0xffffffff

		# Add to progif database.
		progif_db += struct.pack('<BBBI', (pci_progif >> 16) & 0xff, (pci_progif >> 8) & 0xff, pci_progif & 0xff, string_db_pos)
		progif_has_termination = pci_progif == 0xffffff

	# Create binary file.
	print('Writing binary database...')
	f = open('PCIIDS.BIN', 'wb')

	# List all databases with their respective termination flags.
	dbs = [
		(vendor_db, 14, vendor_has_termination),
		(device_db, 10, device_has_termination),
		(subdevice_db, 8, True),
		(class_db, 5, class_has_termination),
		(subclass_db, 6, subclass_has_termination),
		(progif_db, 7, progif_has_termination),
		(string_db, None, True),
	]

	# Write header containing database offsets.
	db_len = 4 * (len(dbs) - 1)
	for db, entry_length, has_termination in dbs[:-1]:
		db_len += len(db)
		if not has_termination:
			db_len += entry_length
		f.write(struct.pack('<I', db_len))

	# Write the databases themselves, adding termination if required.
	for db, entry_length, has_termination in dbs:
		f.write(db)
		if not has_termination:
			f.write(b'\xff' * entry_length)

	# Finish file.
	f.close()

if __name__ == '__main__':
	main()
