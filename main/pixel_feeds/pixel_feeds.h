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

#ifndef SUPER_SCORPIO_PIXEL_FEEDS_H
#define SUPER_SCORPIO_PIXEL_FEEDS_H

#include "pixel_feed_base.h"
#include "empty_feed.h"
#include "on_off_feed.h"
#include "rx_channel_feed.h"

#include "hardware/sync.h"

typedef union pixel_feed {
    pixel_feed_base_t;
    empty_feed_t empty;
    on_off_feed_t on_off;
    rx_channel_feed_t rx_channel;
} pixel_feed_t;

void init_pixel_feeds();


#endif //SUPER_SCORPIO_PIXEL_FEEDS_H
