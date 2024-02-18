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
#include "hardware/structs/systick.h"

typedef struct {
    uint32_t systick;
    char * msg;
} log_t;

extern uint32_t systick_start;
extern volatile uint32_t tick_logs_count;
extern log_t tick_logs[4096];

static inline void log_tick(char * msg) {
    if (tick_logs_count > 300) return;

    log_t entry = {
            .systick = systick_hw->cvr,
            .msg = msg,
    };
    tick_logs[tick_logs_count++] = entry;
}

static inline void start_tick_logs() {
    tick_logs_count = 0u;
    systick_hw->cvr = 0u;
    systick_start = systick_hw->cvr;
}

static inline void print_tick_logs() {
    printf("logs[%d]\n", tick_logs_count);
    uint32_t systick_last = systick_start;
    for (int ii = 0; ii < tick_logs_count; ii++) {
        printf("[%4d] %6d %6d: %s\n", ii, systick_start - tick_logs[ii].systick, systick_last - tick_logs[ii].systick, tick_logs[ii].msg);
        systick_last = tick_logs[ii].systick;
    }
}

#endif //SUPER_SCORPIO_TICK_LOG_H
