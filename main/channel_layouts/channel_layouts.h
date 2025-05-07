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

#ifndef SUPER_SCORPIO_CHANNEL_LAYOUTS_H
#define SUPER_SCORPIO_CHANNEL_LAYOUTS_H

#include "common.h"

typedef struct tx_channel tx_channel_t;

typedef struct channel_layout {
    void (*set_chain_index)(const uint8_t tx_channel_num);
} channel_layout_t;



#endif //SUPER_SCORPIO_CHANNEL_LAYOUTS_H
