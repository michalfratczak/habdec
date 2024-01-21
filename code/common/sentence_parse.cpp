#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <tuple>
#include <optional>
// #include <chrono>
// #include <ctime>
#include <sstream>
#include <iomanip>
#include "common/date.h"

#include "sentence_parse.h"

namespace
{
std::vector<std::string> split_str(const std::string& text, char sep)
{
    std::vector<std::string> tokens;
    std::size_t start = 0, end = 0;
    while ((end = text.find(sep, start)) != std::string::npos)
    {
        tokens.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(text.substr(start));
    return tokens;
}
}


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
parse_sentence_time(std::string i_time_str)
{
    using namespace std;
    // cout<<"PARSE "<<i_time_str<<endl;

    regex rex{ R"_((\d\d)\D?(\d\d)\D?(\d\d(\.\d+)?)?)_" };
    smatch match;
	regex_match(i_time_str, match, rex);

    if(!match.size())
        return {};

    int hours = stoi(match[1]);
    int minutes = stoi(match[2]);
    float seconds = 0;
    if(string(match[3]).size())
        seconds = stof(match[3]);

    return { std::tuple<int,int,float>(hours,minutes,seconds) };
}


// given H:M:S time from telemetry string, add current system date to construct full UTC timestamp
// if current system time is late/ahead of telemetry time, and we are near midnight => correct day
std::string timestamp_from_HMS(int i_hour, int i_minute, float i_second)
{
    using namespace std;
    using namespace chrono;
    using namespace date;

    auto sys_now = system_clock::now();
    auto sys_YMD = year_month_day(floor<days>(sys_now));
    auto sys_HMS = make_time(sys_now - floor<days>(sys_now) );

    const bool is_inside_window =
                i_hour == 23 || i_hour == 0;

    if(is_inside_window) {
        if(i_hour == 23 && sys_HMS.hours().count() == 0) // system time is ahead of input time
            sys_YMD = year_month_day( floor<days>(sys_now) - days(1) );
        else if(i_hour == 0 && sys_HMS.hours().count() == 23) // system time is late of input time
            sys_YMD = year_month_day( floor<days>(sys_now) + days(1) );
    }

    stringstream  ss;
    ss  << sys_YMD << "T"
        << setfill('0') << setw(2) << i_hour << ":"
        << setfill('0') << setw(2) << i_minute << ":"
        << setfill('0') << setw(2) << i_second << "Z";

    return ss.str();
}



// get one coordinate string in decimal dd.dddd or nmea format ddmm.mmmm
// return float in decimal degrees
float parse_gps_pos(const std::string lat_or_lon)
{
    //NMEA DDMM.MMMM: 5205.5857, 02112.7309
    //decimal:        52.1234, 21.34 (or 121.345)
    std::string coord(lat_or_lon);

    // positive or negative ?
    float sign = 1.0;
    if(lat_or_lon.at(0) == '-') {
        sign = -1.0;
        coord = lat_or_lon.substr(1);
    }

    //check if this NMEA DDMM.MMMM or usual decimal DD.DDDD
    const size_t decimal_pos = coord.find('.');

    if(decimal_pos == 2 || decimal_pos == 3)
        return stof(lat_or_lon); // usual decimal

    // this is NMEA format
    if(decimal_pos == 4) { // latitude
        float lat = stof(coord);
        const float degs = trunc(lat / 100);
		const float mins = lat - 100.0f * degs;
		lat = degs + mins / 60.0f;
        return sign * lat;
    }
    if(decimal_pos == 5) { // longitude
        float lon = stof(coord);
        const float degs = trunc(lon / 100);
		const float mins = lon - 100.0f * degs;
		lon = degs + mins / 60.0f;
        return sign * lon;
    }

    return 0;
}


std::optional<sondehub::MinTelemetry> parse_sentence(const std::string& sentence_without_crc)
{
	using namespace std;
	using namespace habdec;

	auto tokens = split_str(sentence_without_crc, ',');

    if(tokens.size() < 6)
        return {};

	//remove leading $$$ from callsign
	string callsign = tokens[0];
	size_t dollar = callsign.find('$');
	if(dollar != string::npos) {
		while(callsign.at(dollar) == '$' && dollar < callsign.size())
			dollar++;
		callsign = callsign.substr(dollar);
	}

    const int id = stoi(tokens[1]);
    const string hms_time_str = tokens[2];
    const string lat_str = tokens[3];
    const string lon_str = tokens[4];
    const float alt = stof(tokens[5]);

    // lat / lon
	//
	const float lat = parse_gps_pos(lat_str);
	const float lon = parse_gps_pos(lon_str);

    if(!lat && !lon)
        return {}; //no gps fix ?

	// timestamp
	//
	std::optional< std::tuple<int,int,float> > hms = parse_sentence_time(hms_time_str);
	if(!hms) {
		cout<<"Failed parsing time string: "<<hms_time_str<<endl;
		return {};
	}

	auto _t = hms.value();
	const string timestamp = timestamp_from_HMS( get<0>(_t), get<1>(_t),  get<2>(_t) );

    sondehub::MinTelemetry res;
    res.payload_callsign = callsign;
    res.datetime = timestamp;
    res.frame = id;
    res.lat = lat;
    res.lon = lon;
    res.alt = alt;

    return res;
}

}