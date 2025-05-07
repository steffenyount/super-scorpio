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

#ifndef SUPER_SCORPIO_TICK_LOG_H
#define SUPER_SCORPIO_TICK_LOG_H

#include "pico/stdlib.h"

#include <stdio.h>
#include <hardware/sync.h>
#include "hardware/structs/systick.h"

#define TICK_LOG_BUFFER_SIZE_BITS (8u)
#define TICK_LOG_BUFFER_SIZE (1u << TICK_LOG_BUFFER_SIZE_BITS)
#define TICK_LOG_BUFFER_SIZE_BIT_MASK (TICK_LOG_BUFFER_SIZE - 1u)

#define TICK_LOG_MSG_LIMIT (250u)

typedef struct {
    uint32_t systick;
    char * msg;
    union {
        uint32_t value;
        char * string;
    };
} log_t;

extern volatile uint32_t core0_tick_logs_count;
extern volatile uint32_t core1_tick_logs_count;

#ifdef TICK_LOG_MSG_LIMIT
extern volatile uint32_t core0_tick_logs_msg_limit;
extern volatile uint32_t core1_tick_logs_msg_limit;
#endif

extern log_t core0_tick_logs[TICK_LOG_BUFFER_SIZE];
extern log_t core1_tick_logs[TICK_LOG_BUFFER_SIZE];

static inline void reset_core0_tick_logs_msg_limit() {
#ifdef TICK_LOG_MSG_LIMIT
    core0_tick_logs_msg_limit = (core0_tick_logs_count + TICK_LOG_MSG_LIMIT) & TICK_LOG_BUFFER_SIZE_BIT_MASK;
#endif
}

static inline void reset_core1_tick_logs_msg_limit() {
#ifdef TICK_LOG_MSG_LIMIT
    core1_tick_logs_msg_limit = (core1_tick_logs_count + TICK_LOG_MSG_LIMIT) & TICK_LOG_BUFFER_SIZE_BIT_MASK;
#endif
}

static inline uint32_t get_and_increment_core0_tick_logs_count() {
    uint32_t log_idx;

// make this operation atomic?
//    uint32_t saved_irqs = save_and_disable_interrupts();
    core0_tick_logs_count = ((log_idx = core0_tick_logs_count) + 1u) & TICK_LOG_BUFFER_SIZE_BIT_MASK;
//    restore_interrupts(saved_irqs);

    return log_idx;
}

static inline uint32_t get_and_increment_core1_tick_logs_count() {
    uint32_t log_idx;

// make this operation atomic?
//    uint32_t saved_irqs = save_and_disable_interrupts();
    core1_tick_logs_count = ((log_idx = core1_tick_logs_count) + 1u) & TICK_LOG_BUFFER_SIZE_BIT_MASK;
//    restore_interrupts(saved_irqs);

    return log_idx;
}

static inline void core0_log_tick_with_string(char * msg, char * string) {
#ifdef TICK_LOG_MSG_LIMIT
    if (core0_tick_logs_count == core0_tick_logs_msg_limit) return;
#endif

    uint32_t systick = systick_hw->cvr;

    core0_tick_logs[get_and_increment_core0_tick_logs_count()] = (log_t) {
            .systick = systick,
            .msg = msg,
            .string = string,
    };
}


static inline void core0_log_tick_with_value(char * msg, uint32_t value) {
#ifdef TICK_LOG_MSG_LIMIT
    if (core0_tick_logs_count == core0_tick_logs_msg_limit) return;
#endif

    uint32_t systick = systick_hw->cvr;

    core0_tick_logs[get_and_increment_core0_tick_logs_count()] = (log_t) {
            .systick = systick,
            .msg = msg,
            .value = value,
    };
}

static inline void core0_log_tick(char * msg) {
    core0_log_tick_with_string(msg, NULL);
}

static inline void core1_log_tick_with_string(char * msg, char * string) {
#ifdef TICK_LOG_MSG_LIMIT
    if (core1_tick_logs_count == core1_tick_logs_msg_limit) return;
#endif

    uint32_t systick = systick_hw->cvr;

    core1_tick_logs[get_and_increment_core1_tick_logs_count()] = (log_t) {
            .systick = systick,
            .msg = msg,
            .string = string,
    };
}

static inline void core1_log_tick_with_value(char * msg, uint32_t value) {
#ifdef TICK_LOG_MSG_LIMIT
    if (core1_tick_logs_count == core1_tick_logs_msg_limit) return;
#endif

    uint32_t systick = systick_hw->cvr;

    core1_tick_logs[get_and_increment_core1_tick_logs_count()] = (log_t) {
            .systick = systick,
            .msg = msg,
            .value = value,
    };
}

static inline void core1_log_tick(char * msg) {
    core1_log_tick_with_string(msg, NULL);
}


static inline void log_tick_with_string(char * msg, char * string) {
    if (sio_hw->cpuid) {
        core1_log_tick_with_string(msg, string);

    } else {
        core0_log_tick_with_string(msg, string);
    }
}

static inline void log_tick_with_value(char * msg, uint32_t uint32) {
    if (sio_hw->cpuid) {
        core1_log_tick_with_value(msg, uint32);

    } else {
        core0_log_tick_with_value(msg, uint32);
    }
}

static inline void log_tick(char * msg) {
    if (sio_hw->cpuid) {
        core1_log_tick(msg);

    } else {
        core0_log_tick(msg);
    }
}

void print_tick_logs();


#endif //SUPER_SCORPIO_TICK_LOG_H
