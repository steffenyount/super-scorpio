add_executable(main
        main.c
        pixel_tx_loop/pixel_tx_loop.c
        pixel_rx_loop/pixel_rx_loop.c
        common.h
        tick_log/tick_log.h
        tick_log/tick_log.c
        pixel_feeds/pixel_feeds.h
        pixel_feeds/pixel_feeds.c
        pixel_feeds/empty_feed.h
        pixel_feeds/empty_feed.c
        pixel_feeds/on_off_feed.h
        pixel_feeds/on_off_feed.c
        pixel_channels/tx_channels.h
        pixel_channels/tx_channels.c
        channel_discovery/channel_discovery.h
        channel_discovery/channel_discovery.c
        power_monitor/power_monitor_adc.h
        power_monitor/power_monitor_adc.c
        common.c
        channel_control/pwm_slices.h
        channel_control/pwm_slices.c
        channel_control/pwm_feeds.h
        channel_control/pwm_feeds.c
        power_monitor/power_monitor_feed.h
        power_monitor/power_monitor_feed.c
        power_monitor/power_monitor.h
        power_monitor/power_monitor.c
        power_monitor/power_samples.h
        power_monitor/power_samples.c
        pixel_rx_loop/rx_bytes_feeds.h
        pixel_rx_loop/rx_bytes_feeds.c
        pixel_channels/rx_channels.h
        pixel_channels/rx_channels.c
        pixel_tx_loop/tx_bytes_feed.h
        pixel_tx_loop/tx_bytes_feed.c
        pixel_tx_loop/tx_pixels.h
        pixel_tx_loop/tx_pixels.c
        pixel_channels/pixel_channels.h
        pixel_channels/pixel_channels.c
        pixel_feeds/rx_channel_feed.h
        pixel_feeds/rx_channel_feed.c
        pixel_feeds/pixel_feed_base.h
        channel_layouts/channel_layouts.h
        channel_layouts/channel_layouts.c
        channel_layouts/linear_layout.h
        channel_layouts/linear_layout.c
        channel_layouts/reverse_layout.h
        channel_layouts/reverse_layout.c
        channel_control/channel_control.h
        channel_control/channel_control.c
        power_limiter/power_limiter.h
        power_limiter/power_limiter.c
        pixel_tx_loop/tx_data_feeds.h
        pixel_tx_loop/tx_data_feeds.c
        channel_overrides/channel_overrides.h
        channel_overrides/channel_overrides.c
)

pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/pixel_rx_loop/gpio_pins_to_rx_bytes.pio)
pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/pixel_tx_loop/tx_bytes_to_gpio_pins.pio)

target_include_directories(main PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

# pull in common dependencies
target_link_libraries(main pico_multicore pico_stdlib hardware_adc hardware_clocks hardware_dma hardware_pio hardware_pwm hardware_sync)

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(main)

# add url via pico_set_program_url
auto_set_url(main)
