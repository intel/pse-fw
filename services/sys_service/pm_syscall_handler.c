#include <syscall_handler.h>
#include "sedi.h"
#include "pm_service.h"

int z_impl_power_trigger_pme(uint32_t pci_func)
{
	return sedi_pm_trigger_pme(pci_func);
}

#ifdef CONFIG_USERSPACE
int z_vrfy_power_trigger_pme(uint32_t pci_func)
{
	Z_OOPS(Z_SYSCALL_VERIFY((pci_func >= 0) && (pci_func < 36)));

	return z_impl_power_trigger_pme(pci_func);
}

#include <syscalls/power_trigger_pme_mrsh.c>
#endif /* CONFIG_USERSPACE */

