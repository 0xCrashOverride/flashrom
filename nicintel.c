/*
 * This file is part of the flashrom project.
 *
 * Copyright (C) 2011 Carl-Daniel Hailfinger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

/* Datasheet: http://download.intel.com/design/network/datashts/82559_Fast_Ethernet_Multifunction_PCI_Cardbus_Controller_Datasheet.pdf */

#include <stdlib.h>
#include "flash.h"
#include "programmer.h"

uint8_t *nicintel_bar;
uint8_t *nicintel_control_bar;

const struct pcidev_status nics_intel[] = {
	{PCI_VENDOR_ID_INTEL, 0x1209, NT, "Intel", "8255xER/82551IT Fast Ethernet Controller"},
	{PCI_VENDOR_ID_INTEL, 0x1229, NT, "Intel", "82557/8/9/0/1 Ethernet Pro 100"},

	{},
};

/* Arbitrary limit, taken from the datasheet I just had lying around.
 * 128 kByte on the 82559 device. Or not. Depends on whom you ask.
 */
#define NICINTEL_MEMMAP_SIZE (128 * 1024)
#define NICINTEL_MEMMAP_MASK (NICINTEL_MEMMAP_SIZE - 1)

#define NICINTEL_CONTROL_MEMMAP_SIZE	0x10 

#define CSR_FCR 0x0c

static int nicintel_shutdown(void *data)
{
	physunmap(nicintel_control_bar, NICINTEL_CONTROL_MEMMAP_SIZE);
	physunmap(nicintel_bar, NICINTEL_MEMMAP_SIZE);
	pci_cleanup(pacc);
	release_io_perms();
	return 0;
}

int nicintel_init(void)
{
	uintptr_t addr;

	/* Needed only for PCI accesses on some platforms.
	 * FIXME: Refactor that into get_mem_perms/get_io_perms/get_pci_perms?
	 */
	get_io_perms();

	/* No need to check for errors, pcidev_init() will not return in case
	 * of errors.
	 * FIXME: BAR2 is not available if the device uses the CardBus function.
	 */
	addr = pcidev_init(PCI_BASE_ADDRESS_2, nics_intel);

	nicintel_bar = physmap("Intel NIC flash", addr, NICINTEL_MEMMAP_SIZE);
	if (nicintel_bar == ERROR_PTR)
		goto error_out_unmap;

	/* FIXME: Using pcidev_dev _will_ cause pretty explosions in the future. */
	addr = pcidev_validate(pcidev_dev, PCI_BASE_ADDRESS_0, nics_intel);
	/* FIXME: This is not an aligned mapping. Use 4k? */
	nicintel_control_bar = physmap("Intel NIC control/status reg",
	                               addr, NICINTEL_CONTROL_MEMMAP_SIZE);
	if (nicintel_control_bar == ERROR_PTR)
		goto error_out;

	if (register_shutdown(nicintel_shutdown, NULL))
		return 1;

	/* FIXME: This register is pretty undocumented in all publicly available
	 * documentation from Intel. Let me quote the complete info we have:
	 * "Flash Control Register: The Flash Control register allows the CPU to
	 *  enable writes to an external Flash. The Flash Control Register is a
	 *  32-bit field that allows access to an external Flash device."
	 * Ah yes, we also know where it is, but we have absolutely _no_ idea
	 * what we should do with it. Write 0x0001 because we have nothing
	 * better to do with our time.
	 */
	pci_rmmio_writew(0x0001, nicintel_control_bar + CSR_FCR);

	buses_supported = CHIP_BUSTYPE_PARALLEL;

	max_rom_decode.parallel = NICINTEL_MEMMAP_SIZE;

	return 0;

error_out_unmap:
	physunmap(nicintel_bar, NICINTEL_MEMMAP_SIZE);
error_out:
	pci_cleanup(pacc);
	release_io_perms();
	return 1;
}

void nicintel_chip_writeb(uint8_t val, chipaddr addr)
{
	pci_mmio_writeb(val, nicintel_bar + (addr & NICINTEL_MEMMAP_MASK));
}

uint8_t nicintel_chip_readb(const chipaddr addr)
{
	return pci_mmio_readb(nicintel_bar + (addr & NICINTEL_MEMMAP_MASK));
}
