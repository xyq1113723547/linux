/*
 * fixup-loongson3.c
 *
 * Copyright (C) 2012 Lemote, Inc.
 * Author: Xiang Yu, xiangy@lemote.com
 *         Chen Huacai, chenhc@lemote.com
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 * WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 * USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/pci.h>
#include <irq.h>
#include <boot_param.h>
#include <workarounds.h>
#include <loongson.h>
#include <loongson-pch.h>
#include <linux/vgaarb.h>

static void print_fixup_info(const struct pci_dev *pdev)
{
	dev_info(&pdev->dev, "Device %x:%x, irq %d\n",
			pdev->vendor, pdev->device, pdev->irq);
}

int pcibios_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	if (loongson_pch->pcibios_map_irq)
		return loongson_pch->pcibios_map_irq(dev, slot, pin);

	print_fixup_info(dev);
	return dev->irq;
}

static void pci_fixup_radeon(struct pci_dev *pdev)
{
	struct resource *res = &pdev->resource[PCI_ROM_RESOURCE];

	if (res->start)
		return;

	if (!loongson_sysconf.vgabios_addr)
		return;

	pci_disable_rom(pdev);
	if (res->parent)
		release_resource(res);

	res->start = virt_to_phys((void *) loongson_sysconf.vgabios_addr);
	res->end   = res->start + 256*1024 - 1;
	res->flags = IORESOURCE_MEM | IORESOURCE_ROM_SHADOW |
		     IORESOURCE_PCI_FIXED;

	dev_info(&pdev->dev, "BAR %d: assigned %pR for Radeon ROM\n",
		 PCI_ROM_RESOURCE, res);
}

DECLARE_PCI_FIXUP_CLASS_FINAL(PCI_VENDOR_ID_ATI, 0x9615,
				PCI_CLASS_DISPLAY_VGA, 8, pci_fixup_radeon);


static void pci_fixup_ls7a1000(struct pci_dev *pdev)
{
	struct pci_dev *pDefVga = NULL;

	while ((pDefVga = pci_get_class(PCI_CLASS_DISPLAY_VGA << 8, pDefVga))) {
		if (pDefVga->vendor != PCI_VENDOR_ID_LOONGSON) {
			vga_set_default_device(pDefVga);
			dev_info(&pdev->dev,
				"Overriding boot device as %X:%X\n",
				pDefVga->vendor, pDefVga->device);
		}
	}
}
DECLARE_PCI_FIXUP_CLASS_FINAL(PCI_VENDOR_ID_LOONGSON, PCI_DEVICE_ID_LOONGSON_DC,
		PCI_CLASS_DISPLAY_VGA, 8, pci_fixup_ls7a1000);

DECLARE_PCI_FIXUP_CLASS_FINAL(PCI_VENDOR_ID_LOONGSON, PCI_DEVICE_ID_LOONGSON_DC,
		PCI_CLASS_DISPLAY_OTHER, 8, pci_fixup_ls7a1000);


/* Do platform specific device initialization at pci_enable_device() time */
int pcibios_plat_dev_init(struct pci_dev *dev)
{
	int ret;
	dev->dev.archdata.dma_attrs = 0;
	if (loongson_sysconf.workarounds & WORKAROUND_PCIE_DMA)
		dev->dev.archdata.dma_attrs = DMA_ATTR_FORCE_SWIOTLB;

	if (loongson_pch->pcibios_dev_init)
		ret = loongson_pch->pcibios_dev_init(dev);
	else
		ret = 0;

	return ret;
}
