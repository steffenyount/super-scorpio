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

#include "channel_discovery/channel_discovery.h"
#include "pixel_rx_loop/pixel_rx_loop.h"
#include "pixel_tx_loop/pixel_tx_loop.h"
#include "power_monitor/power_monitor.h"
#include "pixel_channels/pixel_channels.h"
#include "pixel_feeds/pixel_feeds.h"
#include "tick_log/tick_log.h"
#include "channel_layouts/channel_layouts.h"
#include "power_limiter/power_limiter.h"
#include "channel_control/channel_control.h"
#include "channel_overrides/channel_overrides.h"

#include <stdio.h>
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pixel_channels/rx_channels.h"

void core1_main() {
    init_systick();
    sync_systicks();

    run_pixel_tx_loop();
}

int main() {
    stdio_usb_init();
    init_dma_bus_priority();
    init_gpio_pads_and_io();
    init_systick();
    init_core0_irqs();

    for (int ii = 7; ii > 0; ii--) {
        printf("Starting Super Scorpio! %d\n", ii);
        sleep_ms(1000);
    }
    printf("init_pixel_channels()\n");
    init_pixel_channels();
    printf("init_pixel_control()\n");
    init_pixel_control();

    printf("start_power_monitor()\n");
    start_power_monitor();
    printf("discover_tx_channels()\n");
    discover_tx_channel_pixels();
    printf("apply_channel_overrides()\n");
    apply_channel_overrides();
    printf("limit_tx_channel_power()\n");
    limit_tx_channel_power();
//    printf("stop_power_monitor()\n");
//    stop_power_monitor();


    printf("init_pixel_feeds()\n");
    init_pixel_feeds();

    printf("done\n");

    printf("\nprint_tick_logs()\n");
    print_tick_logs();
    printf("print_tick_logs() - done\n");

    multicore_launch_core1(core1_main);
    printf("sync_systicks()\n");
    sync_systicks();

    printf("sleep_ms(1000)\n");
    sleep_ms(1000);


    printf("launch_pixel_rx_loop()\n");
    launch_pixel_rx_loop();
    printf("sleep_ms(1000)\n");
    sleep_ms(1000);

    printf("\nprint_tick_logs()\n");
    uint32_t loops = 0u;
    while (true) {
        if (loops++ > 320) break;
        print_tick_logs();
//        dump_new_rx_pixels_buffers();
        if ((loops & 0x1fu) == 0x1fu) {
            reset_core1_tick_logs_msg_limit();
        }
        sleep_ms(1000);
//        tight_loop_contents();
    }
    printf("print_tick_logs() - done\n");
    while (true) {
        tight_loop_contents();
    }
}