/***************************************************************************
 *   Generic ESP xtensa target implementation for OpenOCD                  *
 *   Copyright (C) 2019 Espressif Systems Ltd.                             *
 *   Author: Alexey Gerenkov <alexey@espressif.com>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#ifndef ESP_XTENSA_H
#define ESP_XTENSA_H

#include "target.h"
#include "command.h"
#include "xtensa.h"
#include "esp_xtensa_semihosting.h"


/* must be in sync with ESP-IDF version */
/** Size of the pre-compiled target buffer for stub trampoline.
 * @note Must be in sync with ESP-IDF version */
#define ESP_DBG_STUBS_CODE_BUF_SIZE         32	/* TODO: move this info to esp_dbg_stubs_desc */
/** Size of the pre-compiled target buffer for stack.
 * @note Must be in sync with ESP-IDF version */
#define ESP_DBG_STUBS_STACK_MIN_SIZE        2048/* TODO: move this info to esp_dbg_stubs_desc */


/**
 * Debug stubs table entries IDs
 *
 * @note Must be in sync with ESP-IDF version
 */
enum esp_dbg_stub_id {
	ESP_DBG_STUB_TABLE_START,
	ESP_DBG_STUB_DESC = ESP_DBG_STUB_TABLE_START,	/*< Stubs descriptor ID */
	ESP_DBG_STUB_ENTRY_FIRST,
	ESP_DBG_STUB_ENTRY_GCOV = ESP_DBG_STUB_ENTRY_FIRST,	/*< GCOV stub ID */
	/* add new stub entries here */
	ESP_DBG_STUB_ENTRY_MAX,
};

/**
 * Debug stubs descriptor. ID: ESP_DBG_STUB_DESC
 *
 * @note Must be in sync with ESP-IDF version
 */
struct esp_dbg_stubs_desc {
	/** Address of pre-compiled target buffer for stub trampoline. The size of buffer the is
	 * ESP_DBG_STUBS_CODE_BUF_SIZE. */
	uint32_t tramp_addr;
	/** Pre-compiled target buffer's addr for stack. The size of the buffer is ESP_DBG_STUBS_STACK_MIN_SIZE.
	    Target has the buffer which is used for the stack of onboard algorithms.
	If stack size required by algorithm exceeds ESP_DBG_STUBS_STACK_MIN_SIZE,
	it should be allocated using onboard function pointed by 'data_alloc' and
	freed by 'data_free'. They fit to the minimal stack. See below. */
	uint32_t min_stack_addr;
	/** Address of malloc-like function to allocate buffer on target. */
	uint32_t data_alloc;
	/** Address of free-like function to free buffer allocated with data_alloc. */
	uint32_t data_free;
};

/**
 * Debug stubs info.
 */
struct esp_dbg_stubs {
	/** Address. */
	uint32_t base;
	/** Table contents. */
	uint32_t entries[ESP_DBG_STUB_ENTRY_MAX];
	/** Number of table entries. */
	uint32_t entries_count;
	/** Debug stubs decsriptor. */
	struct esp_dbg_stubs_desc desc;
};

#define ESP_XTENSA_FLASH_BREAKPOINTS_MAX_NUM  32

struct esp_xtensa_flash_breakpoint {
	struct xtensa_sw_breakpoint data;
	void *priv;
};

struct esp_xtensa_flash_breakpoint_ops {
	int (*breakpoint_add)(struct target *target, struct breakpoint *breakpoint,
		struct esp_xtensa_flash_breakpoint *spec_bp);
	int (*breakpoint_remove)(struct target *target,
		struct esp_xtensa_flash_breakpoint *spec_bp);
};

struct esp_xtensa_semihost_data {
	char *basedir;
	uint32_t version;		/* sending with drvinfo syscall */
	bool need_resume;
};

struct esp_xtensa_common {
	struct xtensa xtensa;
	struct esp_dbg_stubs dbg_stubs;
	const struct esp_xtensa_flash_breakpoint_ops *flash_brps_ops;
	struct esp_xtensa_flash_breakpoint *flash_brps;
	struct esp_xtensa_semihost_data semihost;
};

static inline struct esp_xtensa_common *target_to_esp_xtensa(struct target *target)
{
	return container_of(target->arch_info, struct esp_xtensa_common, xtensa);
}

int esp_xtensa_init_arch_info(struct target *target,
	struct esp_xtensa_common *esp_xtensa,
	const struct xtensa_config *xtensa_cfg,
	struct xtensa_debug_module_config *dm_cfg,
	const struct esp_xtensa_flash_breakpoint_ops *spec_brps_ops);
int esp_xtensa_target_init(struct command_context *cmd_ctx, struct target *target);
void esp_xtensa_target_deinit(struct target *target);
int esp_xtensa_arch_state(struct target *target);
void esp_xtensa_queue_tdi_idle(struct target *target);
int esp_xtensa_breakpoint_add(struct target *target, struct breakpoint *breakpoint);
int esp_xtensa_breakpoint_remove(struct target *target, struct breakpoint *breakpoint);
int esp_xtensa_poll(struct target *target);
int esp_xtensa_handle_target_event(struct target *target, enum target_event event,
	void *priv);

static inline int esp_xtensa_set_peri_reg_mask(struct target *target,
	target_addr_t addr,
	uint32_t mask,
	uint32_t val)
{
	uint32_t reg_val;
	int res = target_read_u32(target, addr, &reg_val);
	if (res != ERROR_OK)
		return res;
	reg_val = (reg_val & (~mask)) | val;
	res = target_write_u32(target, addr, reg_val);
	if (res != ERROR_OK)
		return res;

	return ERROR_OK;
}

COMMAND_HELPER(esp_xtensa_cmd_semihost_basedir_do, struct esp_xtensa_common *esp_xtensa);

extern const struct command_registration esp_command_handlers[];

#endif	/* ESP_XTENSA_H */
