#!/usr/bin/python3
#
# 86Box          A hypervisor and IBM PC system emulator that specializes in
#                running old operating systems and software designed for IBM
#                PC systems and compatibles from 1981 through fairly recent
#                system designs based on the PCI bus.
#
#                This file is part of the 86Box Probing Tools distribution.
#
#                Utility library for identifying PCI device/vendor IDs.
#
#
#
# Authors:       RichardG, <richardg867@gmail.com>
#
#                Copyright 2021 RichardG.
#
import re, urllib.request

clean_device_abbr = [
	('100Base-T', 'FE'),
	('100Base-TX', 'FE'),
	('1000Base-T', 'GbE'),
	('Acceleration', 'Accel.'),
	('Accelerator', 'Accel.'),
	('Alert on LAN', 'AoL'),
	('Chipset Family', 'Chipset'),
	('Chipset Graphics', 'iGPU'),
	('Connection', 'Conn.'),
	('DECchip', ''),
	('Dual Port', '2-port'),
	('Fast Ethernet', 'FE'),
	('Fibre Channel', 'FC'),
	('Function', 'Func.'),
	('([0-9]{1,3})G Ethernet', '\\2GbE'),
	('(?:([0-9]{1,3}) ?)?(?:G(?:bit|ig) Ethernet|GbE)', '\\2GbE'),
	('Graphics Processor', 'GPU'),
	('High Definition Audio', 'HDA'),
	('Host Adapter', 'HBA'),
	('Host Bus Adapter', 'HBA'),
	('Host Controller', 'HC'),
	('Input/Output', 'I/O'),
	('Integrated ([^\s]+) Graphics', '\\2 iGPU'), # VIA CLE266
	('Integrated Graphics', 'iGPU'),
	('([0-9]) lane', '\\2-lane'),
	('Local Area Network', 'LAN'),
	('Low Pin Count', 'LPC'),
	('Memory Controller Hub', 'MCH'),
	('Network Adapter', 'NIC'),
	('Network (?:Interface )?Card', 'NIC'),
	('Network (?:Interface )?Controller', 'NIC'),
	('NVM Express', 'NVMe'),
	('Parallel ATA', 'PATA'),
	('PCI-E', 'PCIe'),
	('PCI Express', 'PCIe'),
	('PCI[- ]to[- ]PCI', 'PCI-PCI'),
	('Platform Controller Hub', 'PCH'),
	('([0-9]) port', '\\2-port'),
	('Processor Graphics', 'iGPU'),
	('Quad Port', '4-port'),
	('Serial ATA', 'SATA'),
	('Serial Attached SCSI', 'SAS'),
	('Single Port', '1-port'),
	('USB ?([0-9])\\.0', 'USB\\2'),
	('USB ?([0-9])\\.[0-9] ?Gen([0-9x]+)', 'USB\\2.\\3'),
	('USB ?([0-9]\\.[0-9])', 'USB\\2'),
	('Virtual Machine', 'VM'),
	('Wake on LAN', 'WoL'),
	('Wireless LAN', 'WLAN'),
]
clean_device_bit_pattern = re.compile('''( |^|\(|\[|\{|/)(?:([0-9]{1,4}) )?(?:(K)(?:ilo)?|(M)(?:ega)?|(G)(?:iga)?)bit( |$|\)|\]|\})''', re.I)
clean_device_group_pattern = re.compile('''\\\\([0-9]+)''')
clean_device_suffix_pattern = re.compile(''' (?:Adapter|Card|Device|(?:Host )?Controller)( (?: [0-9#]+)?|$|\)|\]|\})''', re.I)
clean_vendor_abbr_pattern = re.compile(''' \[([^\]]+)\]''')
clean_vendor_suffix_pattern = re.compile(''' (?:Semiconductors?|(?:Micro)?electronics?|Interactive|Technolog(?:y|ies)|(?:Micro)?systems|Computer(?: works)?|Products|Group|and subsidiaries|of(?: America)?|Co(?:rp(?:oration)?|mpany)?|Inc|LLC|Ltd|GmbH|AB|AG|SA|(?:\(|\[|\{).*)$''', re.I)
clean_vendor_force = {
	'National Semiconductor Corporation': 'NSC',
}
clean_vendor_final = {
	'Chips and': 'C&T',
	'Digital Equipment': 'DEC',
	'Microchip Technology/SMSC': 'Microchip/SMSC',
	'NVidia/SGS Thomson': 'NVIDIA/ST',
	'S3 Graphics': 'S3',
	'Silicon Integrated': 'SiS',
	'Silicon Motion': 'SMI',
	'STMicroelectronics': 'ST',
	'Texas Instruments': 'TI',
	'VMWare': 'VMware',
}

_clean_device_abbr_cache = []
_pci_vendors = {}
_pci_devices = {}
_pci_classes = {}
_pci_subclasses = {}
_pci_progifs = {}

def clean_device(device, vendor=None):
	"""Make a device name more compact if possible."""

	# Generate pattern cache if required.
	if not _clean_device_abbr_cache:
		for pattern, replace in clean_device_abbr:
			_clean_device_abbr_cache.append((
				re.compile('''( |^|\(|\[|\{|/)''' + pattern + '''( |$|\)|\]|\})''', re.I),
				'\\g<1>' + replace + '\\g<' + str(1 + len(clean_device_group_pattern.findall(pattern))) + '>',
			))

	# Apply patterns.
	device = clean_device_bit_pattern.sub('\\1\\2\\3\\4\\5bit\\6', device)
	for pattern, replace in _clean_device_abbr_cache:
		device = pattern.sub(replace, device)
	device = clean_device_suffix_pattern.sub('\\1', device)

	# Remove duplicate vendor ID.
	if vendor and device[:len(vendor)] == vendor:
		device = device[len(vendor):]

	# Remove duplicate spaces.
	return ' '.join(device.split())

def clean_vendor(vendor):
	"""Make a vendor name more compact if possible."""

	# Apply force table.
	vendor_force = clean_vendor_force.get(vendor, None)
	if vendor_force:
		return vendor_force

	# Use an abbreviation if the name already includes it.
	vendor = vendor.replace(' / ', '/')
	match = clean_vendor_abbr_pattern.search(vendor)
	if match:
		return match.group(1)

	# Apply patterns.
	match = True
	while match:
		vendor = vendor.rstrip(' ,.')
		match = clean_vendor_suffix_pattern.search(vendor)
		if match:
			vendor = vendor[:match.start()]

	# Apply final cleanup table.
	vendor = clean_vendor_final.get(vendor, vendor)

	# Remove duplicate spaces.
	return ' '.join(vendor.split())

def get_pci_id(vendor_id, device_id):
	"""Get the PCI device vendor and name for vendor_id and device_id."""

	# Load PCI ID database if required.
	if not _pci_vendors:
		load_pci_db()

	# Get identification.
	vendor = _pci_vendors.get(vendor_id, '').strip()
	return vendor or '[Unknown]', _pci_devices.get((vendor_id << 16) | device_id, vendor and '[Unknown]' or '').strip()

def load_pci_db():
	"""Loads PCI ID database from disk or the website."""

	# Try loading from disk or the website.
	try:
		f = open('/usr/share/misc/pci.ids', 'rb')
	except:
		try:
			f = urllib.request.urlopen('https://pci-ids.ucw.cz/v2.2/pci.ids', timeout=30)
		except:
			# No sources available.
			return

	vendor = 0
	class_num = subclass_num = None
	for line in f:
		if len(line) < 2 or line[0] == 35:
			continue
		elif line[0] == 67: # class
			class_num = int(line[2:4], 16)
			_pci_classes[class_num] = line[6:-1].decode('utf8', 'ignore')
		elif class_num != None: # subclass/progif
			if line[1] != 9: # subclass
				subclass_num = (class_num << 8) | int(line[1:3], 16)
				_pci_subclasses[subclass_num] = line[5:-1].decode('utf8', 'ignore')
			else: # progif
				progif_num = (subclass_num << 8) | int(line[2:4], 16)
				_pci_progifs[progif_num] = line[6:-1].decode('utf8', 'ignore')
		elif line[0] != 9: # vendor
			vendor = int(line[:4], 16)
			_pci_vendors[vendor] = line[6:-1].decode('utf8', 'ignore')
		elif line[1] != 9: # device
			device = (vendor << 16) | int(line[1:5], 16)
			_pci_devices[device] = line[7:-1].decode('utf8', 'ignore')

	f.close()

# Debugging feature.
if __name__ == '__main__':
	s = input()
	if len(s) == 8 and ' ' not in s:
		vendor, device = get_pci_id(int(s[:4], 16), int(s[4:], 16))
		vendor = clean_vendor(vendor)
		print(vendor)
		print(clean_device(device, vendor))
	else:
		print(clean_device(s))
