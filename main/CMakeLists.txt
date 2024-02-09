add_executable(main
        main.c
        pixel_tx/pixel_tx.c
        pixel_rx/pixel_rx.c
        common.h
        pixel_tx/tx_bytes_feed.h
        pixel_tx/tx_bytes_feed.c
        tick_log/tick_log.h
        tick_log/tick_log.c
        pixel_tx/pwm_cc_feed.h
        pixel_tx/pwm_cc_feed.c
        pixel_tx/pwm_cc_feed_director.h
        pixel_tx/pwm_cc_feed_director.c
        pixel_tx/pwm_cc_feed_trigger.h
        pixel_tx/pwm_cc_feed_trigger.c
        pixel_feeds/pixel_feeds.h
        pixel_feeds/pixel_feeds.c
        pixel_feeds/empty_feed.h
        pixel_feeds/empty_feed.c
        pixel_feeds/on_off_feed.h
        pixel_feeds/on_off_feed.c
        pixel_tx/irq_handling.h
        pixel_tx/irq_handling.c
        pixel_tx/gpio_tx_pins.h
        pixel_tx/gpio_tx_pins.c
        pixel_tx/pwm_tx_slices.h
        pixel_tx/pwm_tx_slices.c
)

pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/pixel_tx/tx_bytes_to_pwm_cc.pio)

target_include_directories(main PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

# pull in common dependencies
target_link_libraries(main pico_multicore pico_stdlib hardware_clocks hardware_dma hardware_pio hardware_pwm hardware_sync)

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(main)

# add url via pico_set_program_url
auto_set_url(main)
