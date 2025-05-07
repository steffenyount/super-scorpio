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

#include "pixel_rx_loop.h"
#include "pixel_channels/rx_channels.h"
#include "rx_bytes_feeds.h"
#include "gpio_pins_to_rx_bytes.pio.h"
#include "tick_log/tick_log.h"
#include "pixel_tx_loop/tx_bytes_feed.h"

#include "hardware/irq.h"
#include "hardware/pio.h"


#define PIO_IRQ0_SRC_MASK (PIO_INTR_SM0_BITS | PIO_INTR_SM1_BITS | PIO_INTR_SM2_BITS | PIO_INTR_SM3_BITS)
#define PIO_SM0_IRQ_FLAG (1u << 0)
#define PIO_SM1_IRQ_FLAG (1u << 1)
#define PIO_SM2_IRQ_FLAG (1u << 2)
#define PIO_SM3_IRQ_FLAG (1u << 3)
#define PIO_SM_IRQ_FLAGS (PIO_SM0_IRQ_FLAG | PIO_SM1_IRQ_FLAG | PIO_SM2_IRQ_FLAG | PIO_SM3_IRQ_FLAG)
#define PIO_SM0_FSTAT_RXEMPTY_FLAG (1u << (PIO_FSTAT_RXEMPTY_LSB + 0))
#define PIO_SM1_FSTAT_RXEMPTY_FLAG (1u << (PIO_FSTAT_RXEMPTY_LSB + 1))
#define PIO_SM2_FSTAT_RXEMPTY_FLAG (1u << (PIO_FSTAT_RXEMPTY_LSB + 2))
#define PIO_SM3_FSTAT_RXEMPTY_FLAG (1u << (PIO_FSTAT_RXEMPTY_LSB + 3))

static volatile uint32_t dma_channel_aborting = 0u;

static inline void update_rx_pixel_buffer_stats(rx_channel_t * const rx_chan) {
    // record byte_count and reset bytes_fed
    rx_chan->byte_count = rx_chan->bytes_fed;
    rx_chan->bytes_fed = RX_BYTES_BUFFER_SIZE;

    uint16_t pixel_count = fast_16_divide_by_3(rx_chan->byte_count);
    if (0u == (rx_chan->byte_count - (pixel_count * 3u))) { // 0 mod 3
        rx_chan->pixel_type = GRB_3_BYTE;
        rx_chan->pixel_count = pixel_count;

    } else if (0u == (rx_chan->byte_count & 3u)) { // 0 mod 4
        rx_chan->pixel_type = RGBW_4_BYTE;
        rx_chan->pixel_count = rx_chan->byte_count >> 2;

    } else { // assume default
        rx_chan->pixel_type = GRB_3_BYTE;
        rx_chan->pixel_count = pixel_count;
    }

    rx_chan->frame_count++;
}

static inline void trigger_tx_feeds(uint sm) {
    set_bits(&pixel_feeds_ready, rx_channels[sm].tx_feed_triggers);
}

inline static void on_pio_sm_irq(uint sm, uint32_t pio_sm_irq_flag, uint32_t rxempty_flag,
                                 uint32_t feed_channel, uint32_t feed_channel_mask) {

    // PIO is done but DMA may still be running
    if (dma_hw->ch[feed_channel].ctrl_trig & DMA_CH0_CTRL_TRIG_BUSY_BITS) { // PIO is done, DMA is running
        if (!(pio0_hw->fstat & rxempty_flag)) { // RXF is NOT empty
            tight_loop_contents(); // no-op and return while DMA empties the RXF

        } else { // RXF is empty
            if (dma_channel_aborting & feed_channel_mask) {
                tight_loop_contents(); // no-op and return while DMA channel aborts

            } else {
                uint32_t bytes_fed = get_sm_rx_bytes_fed(feed_channel);
                if (0u == bytes_fed) { // PIO is done, RXF is empty, DMA is running, while ZERO bytes have been fed?
                    // Assume this was a spurious irq, clear the interrupt flag, we're done here.
                    pio0_hw->irq = pio_sm_irq_flag; // handler task loop completed
//                    core0_log_tick_with_value("bytes_fed = %4d, spurious irq?", bytes_fed);

                } else { // PIO is done, RXF is empty, DMA is still running, some bytes have been fed.
                    // Capture the number of bytes fed and abort the current DMA transfer
                    rx_channels[sm].bytes_fed = bytes_fed;
//                    core0_log_tick_with_value("bytes_fed = %4d", bytes_fed);
                    dma_hw->abort = feed_channel_mask;
                    set_bits(&dma_channel_aborting, feed_channel_mask);
                }
            }
        }
    } else { // PIO is done, DMA is stopped
        if (pio0_hw->fstat & rxempty_flag) { // RXF is empty
            // DMA channel has aborted
            clear_bits(&dma_channel_aborting, feed_channel_mask);

        } else { // RXF is NOT empty
            // Assume PIO bytes captured >= RX_BYTES_BUFFER_SIZE and reset the RXF to discard any residual bytes
            pio_sm_clear_fifos(pio0, sm);
//            core0_log_tick_with_value("pio_sm_clear_fifos(%s) - done", sm);
        }

        // PIO is done, DMA is stopped, RXF is empty, we're ready!
        // Time to relaunch the rx_loop, update the rx_buffer stats, and hand off our new data to the tx_feeds

        // unblock the PIO
        pio0_hw->irq = pio_sm_irq_flag; // handler task loop completed

        // restart the DMA
        start_dma_sm_rx_bytes_feed(sm, feed_channel);

        // update rx_channel's buffer stats
        update_rx_pixel_buffer_stats(&rx_channels[sm]);

        // notify the tx_feeds!
        trigger_tx_feeds(sm);
    }
}

static void on_gpio_pins_to_rx_bytes_program_irq() {
    for (uint32_t pio_irq_flags = (pio0_hw->irq & PIO_SM_IRQ_FLAGS);
            (pio_irq_flags);
            pio_irq_flags = (pio0_hw->irq & PIO_SM_IRQ_FLAGS)) {

        if (pio_irq_flags & PIO_SM0_IRQ_FLAG) {
            on_pio_sm_irq(0, PIO_SM0_IRQ_FLAG, PIO_SM0_FSTAT_RXEMPTY_FLAG,
                          DMA_SM0_RX_BYTES_FEED_CHAN, DMA_SM0_RX_BYTES_FEED_CHAN_MASK);
        }
        if (pio_irq_flags & PIO_SM1_IRQ_FLAG) {
            on_pio_sm_irq(1, PIO_SM1_IRQ_FLAG, PIO_SM1_FSTAT_RXEMPTY_FLAG,
                          DMA_SM1_RX_BYTES_FEED_CHAN, DMA_SM1_RX_BYTES_FEED_CHAN_MASK);
        }
        if (pio_irq_flags & PIO_SM2_IRQ_FLAG) {
            on_pio_sm_irq(2, PIO_SM2_IRQ_FLAG, PIO_SM2_FSTAT_RXEMPTY_FLAG,
                          DMA_SM2_RX_BYTES_FEED_CHAN, DMA_SM2_RX_BYTES_FEED_CHAN_MASK);
        }
        if (pio_irq_flags & PIO_SM3_IRQ_FLAG) {
            on_pio_sm_irq(3, PIO_SM3_IRQ_FLAG, PIO_SM3_FSTAT_RXEMPTY_FLAG,
                          DMA_SM3_RX_BYTES_FEED_CHAN, DMA_SM3_RX_BYTES_FEED_CHAN_MASK);
        }
    }
}

static inline void init_new_rx_bytes_handler() {
    // register the on_gpio_pins_to_rx_bytes_program_irq handler
    irq_add_shared_handler(PIO0_IRQ_0, on_gpio_pins_to_rx_bytes_program_irq,
                           PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY); // Add a shared IRQ handler

    // clear irq flags and enable irq sources for pio state machines
    pio0_hw->irq = PIO_SM_IRQ_FLAGS;
    pio_set_irq0_source_mask_enabled(pio0, PIO_IRQ0_SRC_MASK, true);
}

void launch_pixel_rx_loop() {
    init_new_rx_bytes_handler();
    init_pio0_for_gpio_pins_to_rx_bytes_programs();
    init_dma_rx_bytes_feeds();
}