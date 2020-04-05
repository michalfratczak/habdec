
#include "habitat_interface.h"
#include "common/http_request.h"

#include <string>
#include <iostream>
#include <map>
#include <chrono>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "common/console_colors.h"

namespace
{

std::string FlightsListUrl(int hour_offset = 0)
{
	using namespace std;

    auto now = chrono::system_clock::now() + chrono::hours(hour_offset);
    long int secs_since_ep = chrono::duration_cast<chrono::seconds>(
		now.time_since_epoch() ).count();

	return string("/habitat/_design/flight/_view/end_start_including_payloads?startkey=["
				+ to_string(secs_since_ep) + "]"
				+ "&include_docs=True");
}



std::map<std::string, habdec::habitat::HabitatFlight> ParseFlightsJson(const std::string& i_json_str)
{
    using namespace std;
    namespace pt = boost::property_tree;
    using namespace habdec::habitat;

    pt::ptree root;
    stringstream ss; ss << i_json_str;
    pt::read_json(ss, root);

    std::map<string, HabitatFlight> flights_map;

    // flights
    //
    for( pt::ptree::value_type& row : root.get_child("rows") )
    {
        auto& doc = row.second.get_child("doc");

        if( doc.get<string>("type") != "flight" )
            continue;
        /*if( !doc.get<bool>("approved") )
            continue;*/

        HabitatFlight _f;

        _f.name_ = doc.get<string>("name");
        _f.id_ = doc.get<string>("_id");
        _f.lat_ = doc.get_child("launch").get_child("location").get<float>("latitude");
        _f.lon_ = doc.get_child("launch").get_child("location").get<float>("longitude");

        flights_map[_f.id_] = _f;
    }

    // payloads
    //
    for( pt::ptree::value_type& row : root.get_child("rows") )
    {
        auto& doc = row.second.get_child("doc");

        if( doc.get<string>("type") != "payload_configuration" )
            continue;

        HabitatPayload _p;

        _p.flight_id_ = row.second.get<string>("id");
        _p.name_ = doc.get<string>("name");
        _p.id_ = doc.get<string>("_id");
        if( doc.count("metadata") )
            _p.desc_ = doc.get_child("metadata").get<string>("description", "no description");

        auto& transmissions = doc.get_child("transmissions");
        for(auto& t : transmissions )
        {
            if( t.second.get<string>("modulation") != "RTTY" )
                continue;

            string ascii_bits = t.second.get<string>("encoding");
            if( ascii_bits == "ASCII-7" )
                _p.ascii_bits_ = 7;
            else if( ascii_bits == "ASCII-8" )
                _p.ascii_bits_ = 8;
            else
                continue;

            _p.baud_ = t.second.get<float>("baud");
            _p.ascii_stops_ = t.second.get<float>("stop");
            _p.frequency_ = t.second.get<float>("frequency");

            break; // use first of RTTY transmissions
        }

        auto& sentences = doc.get_child("sentences");
        for(auto& sentence : sentences)
        {
            auto& fields = sentence.second.get_child("fields");
            for(auto& field : fields)
            {
                if( field.second.get<string>("name") == "latitude" )
                    _p.coord_format_lat_ = field.second.get<string>("format");
                if( field.second.get<string>("name") == "longitude" )
                    _p.coord_format_lon_ = field.second.get<string>("format");
            }
        }

        if(_p.baud_ && _p.ascii_bits_ && _p.ascii_stops_)
            flights_map[_p.flight_id_].payloads_[_p.id_] = _p;
    }

    return flights_map;
}

} // namespace


namespace habdec
{
namespace habitat
{

std::ostream& operator<<(std::ostream& os, const habdec::habitat::HabitatPayload& p)
{
    os  << "Payload: " << p.name_ << " "
        << C_RED << p.id_ << C_OFF << "\n"
        << "\t" << p.desc_ << "\n"
        << "\tFlight ID: " << p.flight_id_<< "\n"
        << "\tfreq " << p.frequency_ << "\n"
        << "\tbaud/ascii/stops "
        << p.baud_ << " "
        << p.ascii_bits_ << " "
        << p.ascii_stops_;
    return os;
}


std::ostream& operator<<(std::ostream& os, const habdec::habitat::HabitatFlight& f)
{
    os  << "Flight: " << f.name_ << " " << f.id_ << "\n"
        << "\tlat/lon: " << f.lat_ << " " << f.lon_ << "\n";
    for(const auto& p : f.payloads_)
        os << p.second << "\n";
    return os;
}


std::map<std::string, habdec::habitat::HabitatFlight> ListFlights(int hour_offset)
{
    using namespace std;
    using namespace habdec::habitat;

    string docs_json;
    int result = HttpRequest(
            "habitat.habhub.org", FlightsListUrl(hour_offset), 80,
            habdec::HTTP_VERB::kGet, "application/json", "", docs_json );

    std::map<std::string, HabitatFlight> flights;

	if( result )
        flights = ParseFlightsJson( docs_json );
	else
		cout<<"Habitat Flights List error: "<<result<<endl;
    return flights;
}

} // namespace habitat
} // namespace habdec