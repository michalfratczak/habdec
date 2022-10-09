#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <optional>

#include "sondehub/sondehub_uploader.h"

namespace habdec
{

// input time in any of formats:
//     123456
//     12:34:56
//     12_34_56
//     separator can by ay non-digit character
// or format without seconds:
//     1234
//     12:34
//     ...
// or seconds with fractional
//      12:34:56.7
// return tuple hour,minute,seconds
std::optional< std::tuple<int,int,float> >
parse_sentence_time(std::string i_time_str);


// given H:M:S time from telemetry string, add current system date to construct full UTC timestamp
// if current system time is late/ahead of telemetry time, and we are near midnight => correct day
std::string timestamp_from_HMS(int i_hour, int i_minute, float i_second);


// get one coordinate string in decimal dd.dddd or nmea format ddmm.mmmm
// return float in decimal degrees
float parse_gps_pos(const std::string lat_or_lon);

std::optional<sondehub::MinTelemetry> parse_sentence(const std::string& sentence_without_crc);

}