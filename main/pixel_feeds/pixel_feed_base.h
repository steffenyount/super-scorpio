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

#ifndef SUPER_SCORPIO_PIXEL_FEED_BASE_H
#define SUPER_SCORPIO_PIXEL_FEED_BASE_H

#include "common.h"

typedef struct tx_channel tx_channel_t;
typedef union pixel_feed pixel_feed_t;

typedef struct pixel_feed_base {
    void (*open_frame)(pixel_feed_t * const this_feed);
    void (*feed_pixel)(pixel_feed_t * const this_feed, rgbw_pixel_t * const rgbw_dest);
    void (*close_frame)(pixel_feed_t * const this_feed);
    tx_channel_t * tx_chan;
    tx_channel_t * chain_root;
} pixel_feed_base_t;


#endif //SUPER_SCORPIO_PIXEL_FEED_BASE_H
