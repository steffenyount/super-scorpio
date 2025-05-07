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
#include "power_monitor.h"
#include "power_monitor_feed.h"
#include "power_monitor_adc.h"

void start_power_monitor() {
    start_power_monitor_feed();
    start_power_monitor_adc();
}

void stop_power_monitor() {
    stop_power_monitor_feed();
    stop_power_monitor_adc();
}