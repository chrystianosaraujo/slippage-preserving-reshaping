/*
 * This code is based on logger.h from Adobe's lagrange project.
 * Author: Chrystiano Araujo (araujoc@cs.ubc.ca)
 *
 * The original code can be found at: https://github.com/adobe/lagrange
 *
 * Copyright 2020 Adobe. All rights reserved.
 * This file is licensed to you under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License. You may obtain a copy
 * of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under
 * the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
 * OF ANY KIND, either express or implied. See the License for the specific language
 * governing permissions and limitations under the License.
 */
#include <ca_essentials/core/logger.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <sstream>


namespace {

// Custom logger instance defined by the user, if any
std::shared_ptr<spdlog::logger> s_logger;

} // namespace

namespace ca_essentials {

// Retrieve current logger
spdlog::logger& logger()
{
    if (s_logger) {
        return *s_logger;
    } else {
        // When using factory methods provided by spdlog (_st and _mt functions),
        // names must be unique, since the logger is registered globally.
        // Otherwise, you will need to create the logger manually. See
        // https://github.com/gabime/spdlog/wiki/2.-Creating-loggers
        static auto default_logger = spdlog::stdout_color_mt("console");
        default_logger->set_pattern("[%^%l%$] %v");
        return *default_logger;
    }
}

// Use a custom logger
void set_logger(std::shared_ptr<spdlog::logger> x)
{
    s_logger = std::move(x);
}

} // namespace ca_essentials
