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

#ifndef SUPER_SCORPIO_TX_BYTE_FEEDS_H
#define SUPER_SCORPIO_TX_BYTE_FEEDS_H

#include "common.h"
#include "tick_log/tick_log.h"
#include "pixel_feeds/pixel_feeds.h"

//  for the reset period we should pause (280us) equiv to 28 bytes sent
#define NUM_WS2812_RESET_PERIOD_BYTES (28)

typedef struct {
    uint16_t pixels_fed;
    uint16_t reserved;
    uint32_t frames_fed;
} tx_status_t;

extern volatile uint32_t pixel_feeds_ready;


void init_tx_byte_feeds();

void advance_tx_bytes();

uint32_t get_tx_pixels_enabled();

#endif //SUPER_SCORPIO_TX_BYTE_FEEDS_H
