// formatters.hpp

// Boost Logging library
//
// Author: John Torjo, www.torjo.com
//
// Copyright (C) 2007 John Torjo (see www.torjo.com for email)
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org for updates, documentation, and revision history.
// See http://www.torjo.com/log2/ for more details

#pragma once

#include <pika/config.hpp>
#include <pika/logging/manipulator.hpp>

#include <memory>
#include <string>

namespace pika::util::logging::formatter {

    /**
@brief prefixes each message with an index.

Example:
@code
L_ << "my message";
L_ << "my 2nd message";
@endcode

This will output something similar to:

@code
[1] my message
[2] my 2nd message
@endcode
*/
    struct idx : manipulator
    {
        PIKA_EXPORT static std::unique_ptr<idx> make();

        PIKA_EXPORT ~idx();

    protected:
        idx() = default;
    };

    /**
@brief Prefixes the message with a high-precision time (.
You pass the format string at construction.

@code
#include <pika/logging/format/formatter/high_precision_time.hpp>
@endcode

Internally, it uses pika::util::date_time::microsec_time_clock.
So, our precision matches this class.

The format can contain escape sequences:
$dd - day, 2 digits
$MM - month, 2 digits
$yy - year, 2 digits
$yyyy - year, 4 digits
$hh - hour, 2 digits
$mm - minute, 2 digits
$ss - second, 2 digits
$mili - milliseconds
$micro - microseconds (if the high precision clock allows; otherwise, it pads zeros)
$nano - nanoseconds (if the high precision clock allows; otherwise, it pads zeros)


Example:

@code
high_precision_time("$mm:$ss:$micro");
@endcode

@param convert [optional] In case there needs to be a conversion between
std::(w)string and the string that holds your logged message. See convert_format.
*/
    struct high_precision_time : manipulator
    {
        PIKA_EXPORT static std::unique_ptr<high_precision_time> make(std::string const& format);

        PIKA_EXPORT ~high_precision_time();

    protected:
        explicit high_precision_time(std::string const& format)
        {
            configure(format);
        }
    };

    /**
@brief Writes the thread_id to the log

@param convert [optional] In case there needs to be a conversion between
std::(w)string and the string that holds your logged message. See convert_format.
*/
    struct thread_id : manipulator
    {
        PIKA_EXPORT static std::unique_ptr<thread_id> make();

        PIKA_EXPORT ~thread_id();

    protected:
        thread_id() = default;
    };

}    // namespace pika::util::logging::formatter
