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

#include "tx_data_feeds.h"
#include "tx_pixels.h"
#include "pixel_channels/tx_channels.h"
#include "tx_bytes_to_gpio_pins.pio.h"

#include "hardware/dma.h"
#include "hardware/pio.h"

// byte srcs pattern repeats every: 2 buffers x 3 GRB_3_BYTEs x 4 RGBW_4_BYTEs => 24 bytes fed
#define NUM_CTRLS_AND_SRCS_FOR_TX_BYTES_FEEDS (NUM_TX_PIXEL_BUFFERS * 3u * 4u)

//static inline dma_channel_config tx_bytes_feed_ctrl(uint chain_to) {
//    dma_channel_config dma_channel_ctrl = {0};
//    channel_config_set_read_increment(&dma_channel_ctrl, true);
//    channel_config_set_write_increment(&dma_channel_ctrl, true);
//    channel_config_set_dreq(&dma_channel_ctrl, DREQ_FORCE);
//    channel_config_set_chain_to(&dma_channel_ctrl, chain_to); // self == disable
//    channel_config_set_transfer_data_size(&dma_channel_ctrl, DMA_SIZE_8);
//    channel_config_set_ring(&dma_channel_ctrl, true, 4u); // 16 x 1 byte
//    channel_config_set_bswap(&dma_channel_ctrl, false);
//    channel_config_set_irq_quiet(&dma_channel_ctrl, false);
//    channel_config_set_enable(&dma_channel_ctrl, true);
//    channel_config_set_sniff_enable(&dma_channel_ctrl, false);
//    channel_config_set_high_priority(&dma_channel_ctrl, true);
//
//    return dma_channel_ctrl;
//}
#define TX_BYTES_FEED_BASE_CTRL ( \
        DMA_CH0_CTRL_TRIG_INCR_READ_BITS | \
        DMA_CH0_CTRL_TRIG_INCR_WRITE_BITS | \
        (DREQ_FORCE << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB) | \
        (((uint)DMA_SIZE_8) << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB) | \
        (4u << DMA_CH0_CTRL_TRIG_RING_SIZE_LSB) | DMA_CH0_CTRL_TRIG_RING_SEL_BITS | \
        DMA_CH0_CTRL_TRIG_EN_BITS | \
        DMA_CH0_CTRL_TRIG_HIGH_PRIORITY_BITS)
#define TX_BYTES_FEED_CHAINED_TO_TX_BYTES_FEED_DIRECTOR_CTRL ( \
        TX_BYTES_FEED_BASE_CTRL | \
        (DMA_TX_DATA_FEED_CHAN << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB))
#define TX_BYTES_FEED_CHAINED_TO_TX_DATA_FEED_DIRECTOR_CTRL ( \
        TX_BYTES_FEED_BASE_CTRL | \
        (DMA_TX_DATA_FEED_DIRECTOR_CHAN << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB))

//static dma_channel_config tx_bytes_feed_director_ctrl() {
//    dma_channel_config dma_channel_ctrl = {0};
//    channel_config_set_read_increment(&dma_channel_ctrl, true);
//    channel_config_set_write_increment(&dma_channel_ctrl, true);
//    channel_config_set_dreq(&dma_channel_ctrl, DREQ_FORCE);
//    channel_config_set_chain_to(&dma_channel_ctrl, DMA_TX_BYTES_FEED_CHAN); // self == disable
//    channel_config_set_transfer_data_size(&dma_channel_ctrl, DMA_SIZE_32);
//    channel_config_set_ring(&dma_channel_ctrl, true, 3u); // 2 x 4 byte
//    channel_config_set_bswap(&dma_channel_ctrl, false);
//    channel_config_set_irq_quiet(&dma_channel_ctrl, false);
//    channel_config_set_enable(&dma_channel_ctrl, true);
//    channel_config_set_sniff_enable(&dma_channel_ctrl, false);
//    channel_config_set_high_priority(&dma_channel_ctrl, true);
//
//    return dma_channel_ctrl;
//}
#define TX_BYTES_FEED_DIRECTOR_CTRL ( \
        DMA_CH0_CTRL_TRIG_INCR_READ_BITS | \
        DMA_CH0_CTRL_TRIG_INCR_WRITE_BITS | \
        (DREQ_FORCE << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB) | \
        (DMA_TX_BYTES_FEED_CHAN << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) | \
        (((uint)DMA_SIZE_32) << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB) | \
        (3u << DMA_CH0_CTRL_TRIG_RING_SIZE_LSB) | DMA_CH0_CTRL_TRIG_RING_SEL_BITS | \
        DMA_CH0_CTRL_TRIG_EN_BITS | \
        DMA_CH0_CTRL_TRIG_HIGH_PRIORITY_BITS)

//static dma_channel_config tx_data_feed_ctrl() {
//    dma_channel_config dma_channel_ctrl = {0};
//    channel_config_set_read_increment(&dma_channel_ctrl, true);
//    channel_config_set_write_increment(&dma_channel_ctrl, true);
//    channel_config_set_dreq(&dma_channel_ctrl, DREQ_FORCE);
//    channel_config_set_chain_to(&dma_channel_ctrl, DMA_TX_DATA_FEED_DIRECTOR_CHAN); // self == disable
//    channel_config_set_transfer_data_size(&dma_channel_ctrl, DMA_SIZE_32);
//    channel_config_set_ring(&dma_channel_ctrl, true, 4u); // 4 x 4 byte
//    channel_config_set_bswap(&dma_channel_ctrl, false);
//    channel_config_set_irq_quiet(&dma_channel_ctrl, false);
//    channel_config_set_enable(&dma_channel_ctrl, true);
//    channel_config_set_sniff_enable(&dma_channel_ctrl, false);
//    channel_config_set_high_priority(&dma_channel_ctrl, true);
//
//    return dma_channel_ctrl;
//}
#define TX_DATA_FEED_CTRL ( \
        DMA_CH0_CTRL_TRIG_INCR_READ_BITS | \
        DMA_CH0_CTRL_TRIG_INCR_WRITE_BITS | \
        (DREQ_FORCE << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB) | \
        (DMA_TX_DATA_FEED_DIRECTOR_CHAN << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) | \
        (((uint)DMA_SIZE_32) << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB) | \
        (4u << DMA_CH0_CTRL_TRIG_RING_SIZE_LSB) | DMA_CH0_CTRL_TRIG_RING_SEL_BITS | \
        DMA_CH0_CTRL_TRIG_EN_BITS | \
        DMA_CH0_CTRL_TRIG_HIGH_PRIORITY_BITS)

//static dma_channel_config tx_feed_ctrl() {
//    dma_channel_config dma_channel_ctrl = {0};
//    channel_config_set_read_increment(&dma_channel_ctrl, false);
//    channel_config_set_write_increment(&dma_channel_ctrl, false);
//    channel_config_set_dreq(&dma_channel_ctrl, DREQ_FORCE);
//    channel_config_set_chain_to(&dma_channel_ctrl, DMA_TX_DATA_FEED_DIRECTOR_CHAN); // self == disable
//    channel_config_set_transfer_data_size(&dma_channel_ctrl, DMA_SIZE_32);
//    channel_config_set_ring(&dma_channel_ctrl, false, 0u);
//    channel_config_set_bswap(&dma_channel_ctrl, false);
//    channel_config_set_irq_quiet(&dma_channel_ctrl, false);
//    channel_config_set_enable(&dma_channel_ctrl, true);
//    channel_config_set_sniff_enable(&dma_channel_ctrl, false);
//    channel_config_set_high_priority(&dma_channel_ctrl, true);
//
//    return dma_channel_ctrl;
//}
#define TX_FEED_CTRL ( \
        (DREQ_FORCE << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB) | \
        (DMA_TX_DATA_FEED_DIRECTOR_CHAN << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) | \
        (((uint)DMA_SIZE_32) << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB) | \
        DMA_CH0_CTRL_TRIG_EN_BITS | \
        DMA_CH0_CTRL_TRIG_HIGH_PRIORITY_BITS)

//static dma_channel_config tx_feed_done_ctrl() {
//    dma_channel_config dma_channel_ctrl = {0};
//    channel_config_set_read_increment(&dma_channel_ctrl, false);
//    channel_config_set_write_increment(&dma_channel_ctrl, false);
//    channel_config_set_dreq(&dma_channel_ctrl, DREQ_FORCE);
//    channel_config_set_chain_to(&dma_channel_ctrl, DMA_TX_DATA_FEED_CHAN); // self == disable
//    channel_config_set_transfer_data_size(&dma_channel_ctrl, DMA_SIZE_32);
//    channel_config_set_ring(&dma_channel_ctrl, false, 0u);
//    channel_config_set_bswap(&dma_channel_ctrl, false);
//    channel_config_set_irq_quiet(&dma_channel_ctrl, false);
//    channel_config_set_enable(&dma_channel_ctrl, true);
//    channel_config_set_sniff_enable(&dma_channel_ctrl, false);
//    channel_config_set_high_priority(&dma_channel_ctrl, true);
//
//    return dma_channel_ctrl;
//}
#define TX_FEED_DONE_CTRL ( \
        (DREQ_FORCE << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB) | \
        (DMA_TX_DATA_FEED_CHAN << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) | \
        (((uint)DMA_SIZE_32) << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB) | \
        DMA_CH0_CTRL_TRIG_EN_BITS | \
        DMA_CH0_CTRL_TRIG_HIGH_PRIORITY_BITS)


tx_data_t __alignment(TX_DATA_ALIGN_SIZE_BITS) tx_data = {
        .sm_enabled_bits = {0u},
        .sm_bytes = {0u},
        };

// usage: gpio_tx_bytes[gpio_num]
uint8_t * const gpio_tx_bytes = &tx_data.bytes[0] - GPIO_TX_PINS_BEGIN;

volatile uint32_t tx_data_pending_index = 0u;
volatile uint32_t tx_data_fed_index = 0u;

volatile uint32_t prev_pio1_fdebug = 0u;
static uint32_t pio_sm_all_fdebug_txstall_mask = PIO_FDEBUG_TXSTALL_BITS;

typedef struct {
    uint32_t ctrl;
    volatile void * read_addr;
} ctrl_and_src_t;

static ctrl_and_src_t ctrls_and_srcs_for_tx_bytes_feed[NUM_CTRLS_AND_SRCS_FOR_TX_BYTES_FEEDS][NUM_TX_PINS];

typedef struct {
    volatile void * read_addr;
    volatile void * write_addr;
    uint32_t transfer_count;
    uint32_t ctrl_trig;
} src_dest_count_and_ctrl_t;

static src_dest_count_and_ctrl_t srcs_dests_counts_and_ctrls_for_tx_data_feed[5] = {
        { // launch the tx_bytes staging sequence, sets the TX_DATA_FEED_CHAN to act as the TX_BYTES_FEED_DIRECTOR
            .read_addr = &ctrls_and_srcs_for_tx_bytes_feed[0][0],
            .write_addr = &dma_hw->ch[DMA_TX_BYTES_FEED_CHAN].al1_ctrl,
            .transfer_count = 2,
            .ctrl_trig = TX_BYTES_FEED_DIRECTOR_CTRL,
        },
        { // feed all 8 words of the tx_data to the 4 pio state machines
            .read_addr = &tx_data.sm_enabled_bits[0],
            .write_addr = &pio1->txf[0],
            .transfer_count = 8,
            .ctrl_trig = TX_DATA_FEED_CTRL,
        },
        { // capture the pio state machine's FDEBUG/TXSTALL flags
                .read_addr = &pio1->fdebug,
                .write_addr = &prev_pio1_fdebug,
                .transfer_count = 1,
                .ctrl_trig = TX_FEED_CTRL,
        },
        { // clear the pio state machine's TXSTALL flags
            .read_addr = &pio_sm_all_fdebug_txstall_mask,
            .write_addr = &pio1->fdebug,
            .transfer_count = 1,
            .ctrl_trig = TX_FEED_CTRL,
        },
        { // update the tx_data_fed_index to mark the tx_data transfer completed, and stop processing
            .read_addr = &tx_data_pending_index,
            .write_addr = &tx_data_fed_index,
            .transfer_count = 1,
            .ctrl_trig = TX_FEED_DONE_CTRL,
        },
};

static inline void increment_tx_data_pending_index() {
    if (tx_data_pending_index >= (NUM_CTRLS_AND_SRCS_FOR_TX_BYTES_FEEDS - 1u)) {
        tx_data_pending_index = 0u;
    } else {
        tx_data_pending_index++;
    }
}

static void init_counts_and_srcs_for_tx_bytes_feed() {
    for (uint8_t tx_channel_num = 0u; tx_channel_num < NUM_TX_PINS; tx_channel_num++) {
        const uint32_t ctrl = (tx_channel_num < (NUM_TX_PINS - 1u)) ?
                TX_BYTES_FEED_CHAINED_TO_TX_BYTES_FEED_DIRECTOR_CTRL :
                TX_BYTES_FEED_CHAINED_TO_TX_DATA_FEED_DIRECTOR_CTRL;

        if (GRB_3_BYTE == tx_channels[tx_channel_num].pixel_type) {
            uint32_t buffer_idx = 0;
            uint32_t tx_bytes_feed_idx = 0;

//            printf("init_counts_and_srcs_for_tx_bytes_feed() - GRB_3_BYTE == tx_channels[%d].pixel_type \n", tx_channel_num);

            while (tx_bytes_feed_idx < NUM_CTRLS_AND_SRCS_FOR_TX_BYTES_FEEDS) {
//                printf("ctrls_and_srcs_for_tx_bytes_feed[%d][%d] = tx_pixels[%d][%d].green\n", tx_bytes_feed_idx, tx_channel_num, buffer_idx, tx_channel_num);
                ctrls_and_srcs_for_tx_bytes_feed[tx_bytes_feed_idx++][tx_channel_num] = (ctrl_and_src_t) {
                        .ctrl = ctrl,
                        .read_addr = &tx_pixels[buffer_idx][tx_channel_num].green,
                };
//                printf("ctrls_and_srcs_for_tx_bytes_feed[%d][%d] = tx_pixels[%d][%d].red\n", tx_bytes_feed_idx, tx_channel_num, buffer_idx, tx_channel_num);
                ctrls_and_srcs_for_tx_bytes_feed[tx_bytes_feed_idx++][tx_channel_num] = (ctrl_and_src_t) {
                        .ctrl = ctrl,
                        .read_addr = &tx_pixels[buffer_idx][tx_channel_num].red,
                };
//                printf("ctrls_and_srcs_for_tx_bytes_feed[%d][%d] = tx_pixels[%d][%d].blue\n", tx_bytes_feed_idx, tx_channel_num, buffer_idx, tx_channel_num);
                ctrls_and_srcs_for_tx_bytes_feed[tx_bytes_feed_idx++][tx_channel_num] = (ctrl_and_src_t) {
                        .ctrl = ctrl,
                        .read_addr = &tx_pixels[buffer_idx][tx_channel_num].blue,
                };
                buffer_idx ^= 0x00000001;
            }
        }
        if (RGBW_4_BYTE == tx_channels[tx_channel_num].pixel_type) {
            uint32_t buffer_idx = 0;
            uint32_t tx_bytes_feed_idx = 0;

//            printf("init_counts_and_srcs_for_tx_bytes_feed() - RGBW_4_BYTE == tx_channels[%d].pixel_type \n", tx_channel_num);

            while (tx_bytes_feed_idx < NUM_CTRLS_AND_SRCS_FOR_TX_BYTES_FEEDS) {
//                printf("ctrls_and_srcs_for_tx_bytes_feed[%d][%d] = tx_pixels[%d][%d].red\n", tx_bytes_feed_idx, tx_channel_num, buffer_idx, tx_channel_num);
                ctrls_and_srcs_for_tx_bytes_feed[tx_bytes_feed_idx++][tx_channel_num] = (ctrl_and_src_t) {
                        .ctrl = ctrl,
                        .read_addr = &tx_pixels[buffer_idx][tx_channel_num].red,
                };
//                printf("ctrls_and_srcs_for_tx_bytes_feed[%d][%d] = tx_pixels[%d][%d].green\n", tx_bytes_feed_idx, tx_channel_num, buffer_idx, tx_channel_num);
                ctrls_and_srcs_for_tx_bytes_feed[tx_bytes_feed_idx++][tx_channel_num] = (ctrl_and_src_t) {
                        .ctrl = ctrl,
                        .read_addr = &tx_pixels[buffer_idx][tx_channel_num].green,
                };
//                printf("ctrls_and_srcs_for_tx_bytes_feed[%d][%d] = tx_pixels[%d][%d].blue\n", tx_bytes_feed_idx, tx_channel_num, buffer_idx, tx_channel_num);
                ctrls_and_srcs_for_tx_bytes_feed[tx_bytes_feed_idx++][tx_channel_num] = (ctrl_and_src_t) {
                        .ctrl = ctrl,
                        .read_addr = &tx_pixels[buffer_idx][tx_channel_num].blue,
                };
//                printf("ctrls_and_srcs_for_tx_bytes_feed[%d][%d] = tx_pixels[%d][%d].white\n", tx_bytes_feed_idx, tx_channel_num, buffer_idx, tx_channel_num);
                ctrls_and_srcs_for_tx_bytes_feed[tx_bytes_feed_idx++][tx_channel_num] = (ctrl_and_src_t) {
                        .ctrl = ctrl,
                        .read_addr = &tx_pixels[buffer_idx][tx_channel_num].white,
                };
                buffer_idx ^= 0x00000001;
            }
        }
    }
}

static void init_dma_tx_bytes_feed() {
    dma_channel_claim(DMA_TX_BYTES_FEED_CHAN);

//    dma_channel_set_config(DMA_TX_BYTES_FEED_CHAN, &tx_bytes_feed_chained_to_tx_data_feed_ctrl, false);

//    dma_channel_set_read_addr(DMA_TX_BYTES_FEED_CHAN, &tx_pixels[0u][0u].red, false);
    dma_channel_set_write_addr(DMA_TX_BYTES_FEED_CHAN, tx_data.bytes, false);
    dma_channel_set_trans_count(DMA_TX_BYTES_FEED_CHAN, 1, false);
}

static void init_dma_tx_data_feed() {
    dma_channel_claim(DMA_TX_DATA_FEED_CHAN);
}

static void init_dma_tx_data_feed_director() {
    dma_channel_claim(DMA_TX_DATA_FEED_DIRECTOR_CHAN);

    dma_channel_config dma_channel_ctrl = {0};
    channel_config_set_read_increment(&dma_channel_ctrl, true);
    channel_config_set_write_increment(&dma_channel_ctrl, true);
    channel_config_set_dreq(&dma_channel_ctrl, DREQ_FORCE);
    channel_config_set_chain_to(&dma_channel_ctrl, DMA_TX_DATA_FEED_DIRECTOR_CHAN); // self == disable
    channel_config_set_transfer_data_size(&dma_channel_ctrl, DMA_SIZE_32);
    channel_config_set_ring(&dma_channel_ctrl, true, 4u); // 4 x 4 byte
    channel_config_set_bswap(&dma_channel_ctrl, false);
    channel_config_set_irq_quiet(&dma_channel_ctrl, false);
    channel_config_set_enable(&dma_channel_ctrl, true);
    channel_config_set_sniff_enable(&dma_channel_ctrl, false);
    channel_config_set_high_priority(&dma_channel_ctrl, true);

    dma_channel_set_config(DMA_TX_DATA_FEED_DIRECTOR_CHAN, &dma_channel_ctrl, false);

//    dma_channel_set_read_addr(DMA_TX_DATA_FEED_DIRECTOR_CHAN, &srcs_dests_counts_and_ctrls_for_tx_data_feed[0], false);
    dma_channel_set_write_addr(DMA_TX_DATA_FEED_DIRECTOR_CHAN, &dma_hw->ch[DMA_TX_DATA_FEED_CHAN].read_addr, false);
    dma_channel_set_trans_count(DMA_TX_DATA_FEED_DIRECTOR_CHAN, 4, false);
}

void init_tx_data_feeds() {
    init_counts_and_srcs_for_tx_bytes_feed();
    init_dma_tx_bytes_feed();
    init_dma_tx_data_feed();
    init_dma_tx_data_feed_director();
}

void trigger_next_tx_data_feed() {
    // queue up the next tx_bytes staging sequence
    srcs_dests_counts_and_ctrls_for_tx_data_feed[0].read_addr = &ctrls_and_srcs_for_tx_bytes_feed[tx_data_pending_index][0];
    // advance index for the next pass (also used for the completion status markers)
    increment_tx_data_pending_index();
    // reset the tx_data feeding sequence and trigger one pass of the tx_data feed
    dma_hw->ch[DMA_TX_DATA_FEED_DIRECTOR_CHAN].al3_read_addr_trig =
            (uintptr_t) &srcs_dests_counts_and_ctrls_for_tx_data_feed[0];
}
