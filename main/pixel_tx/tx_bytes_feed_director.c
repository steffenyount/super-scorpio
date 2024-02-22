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

#include "tx_bytes_feed_director.h"
#include "tx_pixels_buffer.h"

#include "hardware/dma.h"

const uint32_t tx_bytes_director_dma_channel_mask = DMA_TX_BYTES_FEED_DIRECTOR_CHAN_MASK;

uint8_t __not_in_flash("super_scorpio") * srcs_for_tx_bytes_director__stage_green_tx_bytes[2][33] = {{
        &tx_enabled[0][0],  &tx_pixels[0][0].green,
        &tx_enabled[0][1],  &tx_pixels[0][1].green,
        &tx_enabled[0][2],  &tx_pixels[0][2].green,
        &tx_enabled[0][3],  &tx_pixels[0][3].green,
        &tx_enabled[0][4],  &tx_pixels[0][4].green,
        &tx_enabled[0][5],  &tx_pixels[0][5].green,
        &tx_enabled[0][6],  &tx_pixels[0][6].green,
        &tx_enabled[0][7],  &tx_pixels[0][7].green,
        &tx_enabled[0][8],  &tx_pixels[0][8].green,
        &tx_enabled[0][9],  &tx_pixels[0][9].green,
        &tx_enabled[0][10], &tx_pixels[0][10].green,
        &tx_enabled[0][11], &tx_pixels[0][11].green,
        &tx_enabled[0][12], &tx_pixels[0][12].green,
        &tx_enabled[0][13], &tx_pixels[0][13].green,
        &tx_enabled[0][14], &tx_pixels[0][14].green,
        &tx_enabled[0][15], &tx_pixels[0][15].green,

        NULL,
    }, {
        &tx_enabled[1][0],  &tx_pixels[1][0].green,
        &tx_enabled[1][1],  &tx_pixels[1][1].green,
        &tx_enabled[1][2],  &tx_pixels[1][2].green,
        &tx_enabled[1][3],  &tx_pixels[1][3].green,
        &tx_enabled[1][4],  &tx_pixels[1][4].green,
        &tx_enabled[1][5],  &tx_pixels[1][5].green,
        &tx_enabled[1][6],  &tx_pixels[1][6].green,
        &tx_enabled[1][7],  &tx_pixels[1][7].green,
        &tx_enabled[1][8],  &tx_pixels[1][8].green,
        &tx_enabled[1][9],  &tx_pixels[1][9].green,
        &tx_enabled[1][10], &tx_pixels[1][10].green,
        &tx_enabled[1][11], &tx_pixels[1][11].green,
        &tx_enabled[1][12], &tx_pixels[1][12].green,
        &tx_enabled[1][13], &tx_pixels[1][13].green,
        &tx_enabled[1][14], &tx_pixels[1][14].green,
        &tx_enabled[1][15], &tx_pixels[1][15].green,

        NULL,
}};

uint8_t __not_in_flash("super_scorpio") * srcs_for_tx_bytes_director__stage_red_tx_bytes[2][33] = {{
        &tx_enabled[0][0],  &tx_pixels[0][0].red,
        &tx_enabled[0][1],  &tx_pixels[0][1].red,
        &tx_enabled[0][2],  &tx_pixels[0][2].red,
        &tx_enabled[0][3],  &tx_pixels[0][3].red,
        &tx_enabled[0][4],  &tx_pixels[0][4].red,
        &tx_enabled[0][5],  &tx_pixels[0][5].red,
        &tx_enabled[0][6],  &tx_pixels[0][6].red,
        &tx_enabled[0][7],  &tx_pixels[0][7].red,
        &tx_enabled[0][8],  &tx_pixels[0][8].red,
        &tx_enabled[0][9],  &tx_pixels[0][9].red,
        &tx_enabled[0][10], &tx_pixels[0][10].red,
        &tx_enabled[0][11], &tx_pixels[0][11].red,
        &tx_enabled[0][12], &tx_pixels[0][12].red,
        &tx_enabled[0][13], &tx_pixels[0][13].red,
        &tx_enabled[0][14], &tx_pixels[0][14].red,
        &tx_enabled[0][15], &tx_pixels[0][15].red,

        NULL,
    }, {
        &tx_enabled[1][0],  &tx_pixels[1][0].red,
        &tx_enabled[1][1],  &tx_pixels[1][1].red,
        &tx_enabled[1][2],  &tx_pixels[1][2].red,
        &tx_enabled[1][3],  &tx_pixels[1][3].red,
        &tx_enabled[1][4],  &tx_pixels[1][4].red,
        &tx_enabled[1][5],  &tx_pixels[1][5].red,
        &tx_enabled[1][6],  &tx_pixels[1][6].red,
        &tx_enabled[1][7],  &tx_pixels[1][7].red,
        &tx_enabled[1][8],  &tx_pixels[1][8].red,
        &tx_enabled[1][9],  &tx_pixels[1][9].red,
        &tx_enabled[1][10], &tx_pixels[1][10].red,
        &tx_enabled[1][11], &tx_pixels[1][11].red,
        &tx_enabled[1][12], &tx_pixels[1][12].red,
        &tx_enabled[1][13], &tx_pixels[1][13].red,
        &tx_enabled[1][14], &tx_pixels[1][14].red,
        &tx_enabled[1][15], &tx_pixels[1][15].red,

        NULL,
}};

uint8_t __not_in_flash("super_scorpio") * srcs_for_tx_bytes_director__stage_blue_tx_bytes[2][33] = {{
        &tx_enabled[0][0],  &tx_pixels[0][0].blue,
        &tx_enabled[0][1],  &tx_pixels[0][1].blue,
        &tx_enabled[0][2],  &tx_pixels[0][2].blue,
        &tx_enabled[0][3],  &tx_pixels[0][3].blue,
        &tx_enabled[0][4],  &tx_pixels[0][4].blue,
        &tx_enabled[0][5],  &tx_pixels[0][5].blue,
        &tx_enabled[0][6],  &tx_pixels[0][6].blue,
        &tx_enabled[0][7],  &tx_pixels[0][7].blue,
        &tx_enabled[0][8],  &tx_pixels[0][8].blue,
        &tx_enabled[0][9],  &tx_pixels[0][9].blue,
        &tx_enabled[0][10], &tx_pixels[0][10].blue,
        &tx_enabled[0][11], &tx_pixels[0][11].blue,
        &tx_enabled[0][12], &tx_pixels[0][12].blue,
        &tx_enabled[0][13], &tx_pixels[0][13].blue,
        &tx_enabled[0][14], &tx_pixels[0][14].blue,
        &tx_enabled[0][15], &tx_pixels[0][15].blue,

        NULL,
    },  {
        &tx_enabled[1][0],  &tx_pixels[1][0].blue,
        &tx_enabled[1][1],  &tx_pixels[1][1].blue,
        &tx_enabled[1][2],  &tx_pixels[1][2].blue,
        &tx_enabled[1][3],  &tx_pixels[1][3].blue,
        &tx_enabled[1][4],  &tx_pixels[1][4].blue,
        &tx_enabled[1][5],  &tx_pixels[1][5].blue,
        &tx_enabled[1][6],  &tx_pixels[1][6].blue,
        &tx_enabled[1][7],  &tx_pixels[1][7].blue,
        &tx_enabled[1][8],  &tx_pixels[1][8].blue,
        &tx_enabled[1][9],  &tx_pixels[1][9].blue,
        &tx_enabled[1][10], &tx_pixels[1][10].blue,
        &tx_enabled[1][11], &tx_pixels[1][11].blue,
        &tx_enabled[1][12], &tx_pixels[1][12].blue,
        &tx_enabled[1][13], &tx_pixels[1][13].blue,
        &tx_enabled[1][14], &tx_pixels[1][14].blue,
        &tx_enabled[1][15], &tx_pixels[1][15].blue,

        NULL,
}};

void __not_in_flash_func(init_dma_gpio_tx_bytes_feed_director)() {
    dma_channel_claim(DMA_TX_BYTES_FEED_DIRECTOR_CHAN);

    dma_channel_config dma_channel_ctrl = {0};
    channel_config_set_read_increment(&dma_channel_ctrl, true);
    channel_config_set_write_increment(&dma_channel_ctrl, false);
    channel_config_set_dreq(&dma_channel_ctrl, DREQ_FORCE);
    channel_config_set_chain_to(&dma_channel_ctrl, DMA_TX_BYTES_FEED_DIRECTOR_CHAN); // self == disable
    channel_config_set_transfer_data_size(&dma_channel_ctrl, DMA_SIZE_32);
    channel_config_set_ring(&dma_channel_ctrl, true, 0);
    channel_config_set_bswap(&dma_channel_ctrl, false);
    channel_config_set_irq_quiet(&dma_channel_ctrl, false);
    channel_config_set_enable(&dma_channel_ctrl, true);
    channel_config_set_sniff_enable(&dma_channel_ctrl, false);
    channel_config_set_high_priority(&dma_channel_ctrl, false);
    dma_channel_set_config(DMA_TX_BYTES_FEED_DIRECTOR_CHAN, &dma_channel_ctrl, false);

//    dma_channel_set_read_addr(DMA_TX_BYTES_FEED_DIRECTOR_CHAN, even_buff_green_srcs_for_gpio_tx_bytes_feed, false);
    dma_channel_set_write_addr(DMA_TX_BYTES_FEED_DIRECTOR_CHAN, &dma_hw->ch[DMA_TX_BYTES_FEED_CHAN].al3_read_addr_trig, false);
    dma_channel_set_trans_count(DMA_TX_BYTES_FEED_DIRECTOR_CHAN, 1, false);
}
