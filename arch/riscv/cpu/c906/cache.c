// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <cpu_func.h>
#include <asm/cache.h>
#include <cache.h>
#include <asm/csr.h>

#define CSR_MHCR 0x7c1

#define MHCR_IE		BIT(0)
#define MHCR_DE		BIT(1)
#define MHCR_WA		BIT(2)
#define MHCR_WB		BIT(3)
#define MHCR_RS		BIT(4)
#define MHCR_BPE	BIT(5)
#define MHCR_L0BTB	BIT(6)

#define THEAD_dcache_ciall ".long 0x0030000b"

void flush_dcache_all(void)
{
	asm volatile (THEAD_dcache_ciall);
}

void flush_dcache_range(unsigned long start, unsigned long end)
{
	flush_dcache_all();
}

void invalidate_dcache_range(unsigned long start, unsigned long end)
{
	flush_dcache_all();
}

void icache_enable(void)
{
	unsigned long val;

	val = csr_read(CSR_MHCR);
	val |= MHCR_IE;
	csr_write(CSR_MHCR, val);
}

void icache_disable(void)
{
	unsigned long val;

	asm volatile ("fence.i" ::: "memory");
	val = csr_read(CSR_MHCR);
	val &= ~MHCR_IE;
	csr_write(CSR_MHCR, val);
}

void dcache_enable(void)
{
	unsigned long val;

	val = csr_read(CSR_MHCR);
	val |= MHCR_DE;
	csr_write(CSR_MHCR, val);
}

void dcache_disable(void)
{
	unsigned long val;

	flush_dcache_all();
	val = csr_read(CSR_MHCR);
	val &= ~MHCR_DE;
	csr_write(CSR_MHCR, val);
}

int icache_status(void)
{
	return csr_read(CSR_MHCR) & MHCR_IE;
}

int dcache_status(void)
{
	return csr_read(CSR_MHCR) & MHCR_DE;
}
