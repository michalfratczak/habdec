#pragma once

#include <string>
#include <map>

namespace habdec
{

std::string HabitatUploadSentence(
        const std::string& i_sentence,
        const std::string& i_listener_callsign
    );


struct HabitatPayload
{
    std::string     name_ {""};
    std::string     id_ {""};
    std::string     flight_id_ {""};
    std::string     desc_ {""};
    int             frequency_ = 0;
    int             baud_ = 0;
    int             ascii_bits_ = 0;
    int             ascii_stops_ = 0;
};

std::ostream&
operator<<(std::ostream& os, const HabitatPayload& p);


struct HabitatFlight
{
    std::string     id_ {""};
    std::string     name_ {""};
    float   lat_ = 0;
    float   lon_ = 0;
    std::map<std::string, HabitatPayload> payloads_;
};

std::ostream&
operator<<(std::ostream& os, const HabitatFlight& f);

std::map<std::string, habdec::HabitatFlight>
ListFlights(int hour_offset);

} // namespace habdec
