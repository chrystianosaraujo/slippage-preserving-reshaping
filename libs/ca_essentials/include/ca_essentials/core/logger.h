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
#pragma once

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include <sstream>

#define LOGGER ca_essentials::logger()

namespace ca_essentials {

// Retrieves the current logger
spdlog::logger& logger();

// Setup a logger object
void set_logger(std::shared_ptr<spdlog::logger> logger);

} // namespace ca_essentials
