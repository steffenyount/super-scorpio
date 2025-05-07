/**
 * Copyright (C) 2024 Steffen Yount
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "tick_log.h"

volatile uint32_t core0_tick_logs_count = 0u;
volatile uint32_t core1_tick_logs_count = 0u;

#ifdef TICK_LOG_MSG_LIMIT
volatile uint32_t core0_tick_logs_msg_limit = TICK_LOG_MSG_LIMIT;
volatile uint32_t core1_tick_logs_msg_limit = TICK_LOG_MSG_LIMIT;
#endif

log_t core0_tick_logs[TICK_LOG_BUFFER_SIZE];
log_t core1_tick_logs[TICK_LOG_BUFFER_SIZE];

static uint32_t core0_tick_logs_print_idx = 0u;
static uint32_t core1_tick_logs_print_idx = 0u;

static uint32_t prev_core0_systick = 0u;
static uint32_t prev_core1_systick = 0u;

static inline void print_next_core0_tick_log_entry(uint32_t core0_systick) {
    printf("C0[%4d] %8d %8d: ", core0_tick_logs_print_idx, core0_systick,
            (prev_core0_systick > core0_systick) ?
                    prev_core0_systick - core0_systick : (prev_core0_systick + (1u << 24)) - core0_systick);
    printf(core0_tick_logs[core0_tick_logs_print_idx].msg, core0_tick_logs[core0_tick_logs_print_idx].value);
    printf("\n");
    prev_core0_systick = core0_systick;
}

static inline void print_next_core1_tick_log_entry(uint32_t core1_systick) {
    printf("C1[%4d] %8d %8d: ", core1_tick_logs_print_idx, core1_systick,
            (prev_core1_systick > core1_systick) ?
                    prev_core1_systick - core1_systick : (prev_core1_systick + (1u << 24)) - core1_systick);
    printf(core1_tick_logs[core1_tick_logs_print_idx].msg, core1_tick_logs[core1_tick_logs_print_idx].value);
    printf("\n");
    prev_core1_systick = core1_systick;
}

void print_tick_logs() {
    uint32_t next_core1_tick_logs_idx = core1_tick_logs_count;
    uint32_t next_core0_tick_logs_idx = core0_tick_logs_count;

    while (next_core0_tick_logs_idx != core0_tick_logs_print_idx &&
           next_core1_tick_logs_idx != core1_tick_logs_print_idx) {
        uint32_t core0_systick = core0_tick_logs[core0_tick_logs_print_idx].systick;
        uint32_t core1_systick = core1_tick_logs[core1_tick_logs_print_idx].systick;

        // if core0_systick is older than core1_systick by less than 1/2 a full cycle of ticks then print it next
        if (((core0_systick >= core1_systick) ?
                core0_systick - core1_systick : (core0_systick + (1u << 24)) - core1_systick) < (1u << 23) ) {
            print_next_core0_tick_log_entry(core0_systick);
            // increment the print index
            core0_tick_logs_print_idx = (core0_tick_logs_print_idx + 1u) & TICK_LOG_BUFFER_SIZE_BIT_MASK;

        } else {
            print_next_core1_tick_log_entry(core1_systick);
            // increment the print index
            core1_tick_logs_print_idx = (core1_tick_logs_print_idx + 1u) & TICK_LOG_BUFFER_SIZE_BIT_MASK;
        }
    }
    while (next_core0_tick_logs_idx != core0_tick_logs_print_idx) {
        print_next_core0_tick_log_entry(core0_tick_logs[core0_tick_logs_print_idx].systick);
        // increment the print index
        core0_tick_logs_print_idx = (core0_tick_logs_print_idx + 1u) & TICK_LOG_BUFFER_SIZE_BIT_MASK;
    }
    while (next_core1_tick_logs_idx != core1_tick_logs_print_idx) {
        print_next_core1_tick_log_entry(core1_tick_logs[core1_tick_logs_print_idx].systick);
        // increment the print index
        core1_tick_logs_print_idx = (core1_tick_logs_print_idx + 1u) & TICK_LOG_BUFFER_SIZE_BIT_MASK;
    }
}
