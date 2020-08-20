// SPDX-License-Identifier: GPL-2.0
/*
 * aarch64 PSCI 1.1 implementation
 *
 * Copyright (c) 2020 Michael Walle <michael@walle.cc>
 */

#include <config.h>
#include <asm/macro.h>
#include <asm/psci.h>
#include <asm/secure.h>

struct psci_cpu_state {
	u8 affinity_level;
	u8 flags;
	u64 entry_point_address;
	u64 context_id;
};

static struct psci_cpu_state psci_state[CONFIG_ARMV8_PSCI_NR_CPUS] __secure_data = { 0 };

static void __secure psci_set_flags(int cpu, u8 flags)
{
	psci_state[cpu].flags |= flags;
	dsb();
}

static u8 __secure psci_get_flags(int cpu)
{
	return psci_state[cpu].flags;
}

void __secure psci_save_64(int cpu, u64 pc, u64 context_id)
{
	psci_state[cpu].entry_point_address = pc;
	psci_state[cpu].context_id = context_id;
	dsb();
}

void __secure psci_set_affinity_level(int cpu, u8 level)
{
	psci_state[cpu].affinity_level = level;
	dsb();
}

u8 __secure psci_get_affinity_level(int cpu)
{
	return psci_state[cpu].affinity_level;
}

void __secure psci_arch_init(void)
{
	int cpu;

	psci_state[0].affinity_level = PSCI_AFFINITY_LEVEL_ON;
	psci_state[0].flags = PSCI_FLAGS_CORE_POWERED_UP;
	for (cpu = 1; cpu < CONFIG_ARMV8_PSCI_NR_CPUS; cpu++)
		psci_state[cpu].affinity_level = PSCI_AFFINITY_LEVEL_OFF;
	dsb();
}

u64 __secure psci_get_target_pc_64(int cpu)
{
	if (cpu > CONFIG_ARMV8_PSCI_NR_CPUS)
		return 0;

	return psci_state[cpu].entry_point_address;
}

u64 __secure psci_get_context_id_64(int cpu)
{
	if (cpu > CONFIG_ARMV8_PSCI_NR_CPUS)
		return 0;

	return psci_state[cpu].context_id;
}

u32 __secure psci_version(u32 __always_unused func_id)
{
	return ARM_PSCI_VER_1_0;
}

s32 __secure psci_features(u32 __always_unused func_id, u32 psci_func_id)
{
	switch (psci_func_id) {
	case ARM_PSCI_0_2_FN_AFFINITY_INFO:
	case ARM_PSCI_0_2_FN_CPU_OFF:
	case ARM_PSCI_0_2_FN_CPU_ON:
	case ARM_PSCI_0_2_FN_PSCI_VERSION:
	case ARM_PSCI_0_2_FN_SYSTEM_OFF:
	case ARM_PSCI_0_2_FN_SYSTEM_RESET:
	case ARM_PSCI_0_2_FN64_AFFINITY_INFO:
	case ARM_PSCI_0_2_FN64_CPU_ON:
	case ARM_PSCI_1_0_FN_PSCI_FEATURES:
	case ARM_PSCI_1_0_FN_SYSTEM_SUSPEND:
	case ARM_PSCI_1_0_FN64_SYSTEM_SUSPEND:
		return 0;

	case ARM_PSCI_0_2_FN_CPU_SUSPEND:
	case ARM_PSCI_0_2_FN_MIGRATE:
	case ARM_PSCI_0_2_FN_MIGRATE_INFO_TYPE:
	case ARM_PSCI_0_2_FN_MIGRATE_INFO_UP_CPU:
	case ARM_PSCI_0_2_FN64_CPU_SUSPEND:
	case ARM_PSCI_0_2_FN64_MIGRATE:
	case ARM_PSCI_0_2_FN64_MIGRATE_INFO_UP_CPU:
	case ARM_PSCI_0_2_FN64_SYSTEM_RESET2:
	case ARM_PSCI_1_0_FN_CPU_FREEZE:
	case ARM_PSCI_1_0_FN_CPU_DEFAULT_SUSPEND:
	case ARM_PSCI_1_0_FN_NODE_HW_STATE:
	case ARM_PSCI_1_0_FN_SET_SUSPEND_MODE:
	case ARM_PSCI_1_0_FN_STAT_RESIDENCY:
	case ARM_PSCI_1_0_FN_STAT_COUNT:
	case ARM_PSCI_1_0_FN64_CPU_DEFAULT_SUSPEND:
	case ARM_PSCI_1_0_FN64_NODE_HW_STATE:
	case ARM_PSCI_1_0_FN64_STAT_RESIDENCY:
	case ARM_PSCI_1_0_FN64_STAT_COUNT:
		break;
	}

	return ARM_PSCI_RET_NI;
}

s32 __secure psci_affinity_info_64(u32 __always_unused func_id,
				   u64 target_affinity,
				   u64 lowest_affinity_level)
{
	int cpu = __psci_get_cpu_id(target_affinity);

	if (lowest_affinity_level > 0)
		return ARM_PSCI_RET_INVAL;

	if (target_affinity & ~(MPIDR_EL1_AFF0 | MPIDR_EL1_AFF1))
		return ARM_PSCI_RET_INVAL;

	if (cpu >= CONFIG_ARMV8_PSCI_NR_CPUS)
		return ARM_PSCI_RET_INVAL;

	return psci_get_affinity_level(cpu);
}

s32 __secure psci_affinity_info(u32 __always_unused func_id,
				u32 target_affinity,
				u32 lowest_affinity_level)
{
	return psci_affinity_info_64(ARM_PSCI_0_2_FN64_AFFINITY_INFO,
				     target_affinity, lowest_affinity_level);
}

/*
 * For the generic implementation, we just disable all interrupts except for
 * SGI15 and we enter a wfi loop until the core is turned on again. SGI15 is
 * sent by another core to wake us up.
 */
void __secure __weak psci_release_cpu_64(u64 mpidr)
{
	int cpu = __psci_get_cpu_id(mpidr);
	u8 flags = psci_get_flags(cpu);

	/* CPU core wasn't powered up yet, call the arch code to boot it up */
	if (!(flags & PSCI_FLAGS_CORE_POWERED_UP)) {
		psci_set_flags(cpu, PSCI_FLAGS_CORE_POWERED_UP);
		psci_arch_release_cpu_64(cpu);
		return;
	}

	psci_gic_kick_cpu_core(mpidr);
}

void __secure __weak psci_power_down_cpu_64(u64 mpidr)
{
	int cpu = __psci_get_cpu_id(mpidr);
	unsigned long scr_el3;

	/* Disable interrupts except SGI15 */
	psci_gic_pre_off(mpidr);

	/*
	 * SCR_EL3.IRQ need to be enabled otherwise wfi won't return when this
	 * core is waken up by SGI15. When the core wakes up again,
	 * psci_cpu_entry_64() will overwrite this setting again.
	 */
	scr_el3 = get_scr_el3();
	scr_el3 |= SCR_EL3_IRQ_EN;
	set_scr_el3(scr_el3);

	/* enter power-down mode */
	while (psci_get_affinity_level(cpu) == PSCI_AFFINITY_LEVEL_OFF)
		wfi();

	/* Ack SGI15 */
	psci_gic_post_off(mpidr);

	/* Just call the entry point, doesn't return */
	psci_cpu_entry_64();
}

s32 __secure psci_cpu_on_64(u32 __always_unused func_id, u64 mpidr, u64 pc,
			    u64 context_id)
{
	int cpu = __psci_get_cpu_id(mpidr);

	if (mpidr & ~(MPIDR_EL1_AFF0 | MPIDR_EL1_AFF1))
		return ARM_PSCI_RET_INVAL;

	if (cpu >= CONFIG_ARMV8_PSCI_NR_CPUS)
		return ARM_PSCI_RET_INVAL;

	if (psci_get_affinity_level(cpu) == PSCI_AFFINITY_LEVEL_ON)
		return ARM_PSCI_RET_ALREADY_ON;

	if (psci_get_affinity_level(cpu) == PSCI_AFFINITY_LEVEL_ON_PENDING)
		return ARM_PSCI_RET_ON_PENDING;

	psci_save_64(cpu, pc, context_id);
	psci_set_affinity_level(cpu, PSCI_AFFINITY_LEVEL_ON_PENDING);
	psci_release_cpu_64(cpu);

	return ARM_PSCI_RET_SUCCESS;
}

s32 __secure psci_cpu_on(u32 __always_unused func_id, u32 mpidr, u32 pc,
			 u32 context_id)
{
	return psci_cpu_on_64(ARM_PSCI_0_2_FN64_CPU_ON, mpidr, pc, context_id);
}

s32 __secure psci_cpu_off_64(void)
{
	unsigned long mpidr = read_mpidr();
	int cpu = __psci_get_cpu_id(mpidr);

	psci_set_affinity_level(cpu, PSCI_AFFINITY_LEVEL_OFF);
	psci_power_down_cpu_64(mpidr);

	/* Just in case psci_power_down_64() returns */
	while (1)
		wfi();
}

s32 __secure psci_cpu_off(void)
{
	return psci_cpu_off_64();
}
