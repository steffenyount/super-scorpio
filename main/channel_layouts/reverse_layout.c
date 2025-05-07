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

#include "reverse_layout.h"
#include "pixel_channels/tx_channels.h"


static void reverse_layout__set_chain_index(const uint8_t tx_channel_num) {
    tx_channels[tx_channel_num].chain_index = tx_channels[tx_channel_num].chain_offset +
            ((tx_channels[tx_channel_num].pixel_count - 1) - tx_channels[tx_channel_num].tx_status.pixels_fed);
}

channel_layout_t reverse_layout = {
        .set_chain_index = reverse_layout__set_chain_index,
};

void set_reverse_layout(uint8_t tx_gpio_num) {
    gpio_tx_channels[tx_gpio_num].layout = reverse_layout;
}
