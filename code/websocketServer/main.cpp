/*

	Copyright 2018 Michal Fratczak

	This file is part of habdec.

    habdec is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    habdec is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with habdec.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "program_options.h"

#include <signal.h>
#include <string>
#include <vector>
#include <iostream>
#include <future>
#include <cstdlib>
#include <unordered_set>

#include <boost/algorithm/string.hpp>

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Version.hpp>


#include "IQSource/IQSource_SoapySDR.h"
#include "Decoder/Decoder.h"
#include "common/console_colors.h"
#include "habitat/habitat_interface.h"
#include "GLOBALS.h"
#include "ws_server.h"
#include "common/git_repo_sha1.h"
#include "Base64.h"
#include "common/sentence_parse.h"
#include "common/http_request.h"
#include "common/date.h"
#include "common/utc_now_iso.h"
#include "Decoder/CRC.h"
#include "sondehub/sondehub_uploader.h"
#include <cpr/cpr.h>

using namespace std;


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


void SentenceToPosition(const std::string i_snt,
						float &lat, float &lon, float &alt,
						const std::string coord_format_lat, const std::string coord_format_lon
						)
{
    // "CALLSIGN,123,15:41:24,44.32800,-74.14427,00491,0,0,12,30.7,0.0,0.001,20.2,958*6BC9"
    std::vector<std::string> tokens = split_str(i_snt, ',');
    lat = std::stof(tokens[3]);
    lon = std::stof(tokens[4]);
    alt = std::stof(tokens[5]);

	if(coord_format_lat == "ddmm.mmmm")
	{
		const float degs = trunc(lat / 100);
		const float mins = lat - 100.0f * degs;
		lat = degs + mins / 60.0f;
	}

	if(coord_format_lon == "ddmm.mmmm")
	{
		const float degs = trunc(lon / 100);
		const float mins = lon - 100.0f * degs;
		lon = degs + mins / 60.0f;
	}
}


void PrintDevicesList(const SoapySDR::KwargsList& device_list)
{
	int DEV_NUM = 0;
	cout<<endl;
	for(const auto& dev : device_list)
	{
		cout<<C_RED<<DEV_NUM<<":"<<C_OFF<<endl;
		for(const auto& kv : dev)
			cout<<'\t'<<kv.first<<" "<<kv.second<<endl;

		// print sampling rates
		try {
			auto p_device = SoapySDR::Device::make(dev);
			std::vector<double> sampling_rates = p_device->listSampleRates(SOAPY_SDR_RX, 0);
			sort(sampling_rates.begin(), sampling_rates.end());

			cout<<"\tSampling Rates:"<<endl;
			int sr_index = 0;
			for(auto sr : sampling_rates)
				cout<<"\t\t"<<C_MAGENTA<<sr<<C_OFF<<endl;
			cout<<endl;

			SoapySDR::Device::unmake(p_device);
		}
		catch(std::runtime_error& er) {
			cout<<C_RED<<"Failed opening device "<<DEV_NUM<<" : "
				<<C_MAGENTA<<er.what()<<C_OFF<<endl;
		}

		++DEV_NUM;
	}
}


bool SetupDevice(SoapySDR::Kwargs& o_device)
{
	SoapySDR::KwargsList device_list = SoapySDR::Device::enumerate();

	if(!device_list.size())
	{
		cout<<"\nNo Devices found."<<endl;
		return false;
	}
	else
	{
		cout<<"\nAvailable devices:";
		PrintDevicesList(device_list);
		cout<<endl;
	}

	int device_index = GLOBALS::get().par_.device_;

	if(device_index < 0)
	{
		cout<<C_MAGENTA<<"No SDR device specified. Select one by specifying it's"<<C_RED<<" NUMBER"<<C_OFF<<endl;
		cout<<C_MAGENTA<<"Available Devices:"<<C_OFF<<endl;

		int DEV_NUM = 0;
		PrintDevicesList(device_list);
		cout<<C_OFF<<endl;
		return false;
	}

	if( device_index >= int(device_list.size()) )
	{
		cout<<C_RED<<"No device with number "<<device_index<<endl;
		return false;
	}

	if( device_list.size() == 1 )
	{
		device_index = 0;
	}


	// setup DEVICE
	//
	auto& device = device_list[device_index];
	cout<<"Running with "<<device["driver"]<<endl<<endl;

	// user supplied sampling_rate could be unsupported by device
	// find what's closest available to user supplied value or 2MHz
	{
		auto p_device = SoapySDR::Device::make(device);
		const double user_sr = 	GLOBALS::get().par_.sampling_rate_ ?
								GLOBALS::get().par_.sampling_rate_  : 2e6;
		double sr_diff = std::numeric_limits<double>::max();
		for(double sr : p_device->listSampleRates(SOAPY_SDR_RX, 0))
		{
			if( abs(sr - user_sr) < sr_diff )
			{
				sr_diff = abs(sr - user_sr);
				GLOBALS::get().par_.sampling_rate_ = sr;
			}
		}
		cout<<C_RED<<"Setting sampling rate to "<<GLOBALS::get().par_.sampling_rate_<<C_OFF<<endl;
	}


	GLOBALS::get().p_iq_source_.reset(new habdec::IQSource_SoapySDR);

	if(!GLOBALS::get().p_iq_source_)
	{
		cout<<C_RED<<"Failed creating SoapySDR device. EXIT."<<C_OFF<<endl;
		return 1;
	}

	GLOBALS::get().p_iq_source_->setOption("SoapySDR_Kwargs", &device);

	if( !GLOBALS::get().p_iq_source_->init() )
	{
		cout<<C_RED<<"IQSource_SoapySDR::init failed."<<C_OFF<<endl;
		return 1;
	}

	double gain = GLOBALS::get().par_.gain_;
	GLOBALS::get().p_iq_source_->setOption("gain_double", &gain);
	double biastee = GLOBALS::get().par_.biast_;
	GLOBALS::get().p_iq_source_->setOption("biastee_double", &biastee);
	GLOBALS::get().p_iq_source_->setOption("sampling_rate_double", &GLOBALS::get().par_.sampling_rate_);
	double usb_pack = GLOBALS::get().par_.usb_pack_;
	GLOBALS::get().p_iq_source_->setOption("usb_pack_double", &usb_pack);

	// this works only if device is running. not yet here
	// GLOBALS::get().p_iq_source_->setOption("ppm_double", &GLOBALS::get().ppm_);

	o_device = device;
	return true;
}


void DECODER_THREAD()
{
	using namespace std;
	cout<<"Start DECODER_THREAD"<<endl;

	auto& p_iq_src = GLOBALS::get().p_iq_source_;

	if(!p_iq_src)
		return;

	if(!p_iq_src->start())
	{
		cout<<C_RED<<"Failed to start device. Exit DECODER_THREAD."<<C_OFF<<endl;
		return;
	}

	if(!p_iq_src->isRunning())
	{
		cout<<C_RED<<"Device not running. Exit DECODER_THREAD."<<C_OFF<<endl;
		return;
	}

	// this works only if device is running, but feels out of place here
	// consider moving start() outside of this function
	GLOBALS::get().p_iq_source_->setOption("ppm_double", &GLOBALS::get().par_.ppm_);

	//////
	//

	auto& DECODER = GLOBALS::get().decoder_;

	habdec::IQVector<TReal>	samples;
	samples.resize(256*256);
	samples.samplingRate( p_iq_src->samplingRate() );

	while(1)
	{
		size_t count = p_iq_src->get( samples.data(), samples.size() );
		if(count)
			samples.resize(count);
		DECODER.pushSamples(samples);

		DECODER(); // DECODE !

		// AFC - don't do this too often. Too unstable, needs more work.
		static auto last_afc_time = std::chrono::high_resolution_clock::now();
		if( std::chrono::duration_cast< std::chrono::seconds >
				(std::chrono::high_resolution_clock::now() - last_afc_time).count() > 5
			)
		{
			double freq_corr = DECODER.getFrequencyCorrection();
			if(GLOBALS::get().par_.afc_)
			{
				if( 100 < abs(freq_corr)  )
				{
					GLOBALS::get().par_.frequency_ += freq_corr;
					double f = GLOBALS::get().par_.frequency_;
					p_iq_src->setOption("frequency_double", &f);
					DECODER.resetFrequencyCorrection(freq_corr);
					last_afc_time = std::chrono::high_resolution_clock::now();
				}
			}
		}

		// accumulate demod samples to display more
		{
			std::lock_guard<std::mutex> _lock(GLOBALS::get().demod_accumulated_mtx_);

			const size_t max_sz = 	DECODER.getDecimatedSamplingRate() /
						  			DECODER.getSymbolRate() * 50; // 50 symbols

			auto demod = DECODER.getDemodulated();
			auto& demod_acc = GLOBALS::get().demod_accumulated_;
			demod_acc.insert( demod_acc.end(), demod.cbegin(), demod.cend() );

			// remove leading values
			if( demod_acc.size() > max_sz)
				demod_acc.erase( demod_acc.begin(), demod_acc.begin() + demod_acc.size() - max_sz );
		}
	}
}


void SentenceCallback(	std::string callsign, std::string data, std::string crc,
						std::shared_ptr<WebsocketServer> p_ws,
						std::shared_ptr<sondehub::SondeHubUploader> p_sondehub_uploader)
						// sondehub::SondeHubUploader& sondehub_uploader)
{
	using namespace std;

	const string sentence_no_crc = callsign + "," + data;
	cout<<"SentenceCallback "<<sentence_no_crc<<endl;

	auto maybe_telemetry = habdec::parse_sentence(sentence_no_crc);
	sondehub::MinTelemetry t;
	if(maybe_telemetry) {
		t = maybe_telemetry.value();
	}
	else {
		cout<<"Failed parsing "<<sentence_no_crc<<endl;
		return;
	}
	t.time_received = utc_now_iso();

	// sondehub upload
	// cout<<"push"<<endl;
	p_sondehub_uploader->push(t);

	using Ms = std::chrono::milliseconds;

	auto& stats = GLOBALS::get().stats_;
	stats.last_sentence_timestamp_ = std::chrono::steady_clock::now();

	const string sentence = callsign + "," + data + "*" + crc;

	// notify all websocket clients
	if(p_ws)
	{
		auto p_sentence_msg = std::make_shared<HabdecMessage>();
		p_sentence_msg->to_all_clients_ = true;
		p_sentence_msg->data_stream_<<"cmd::info:sentence="<<sentence;
		p_ws->sessions_send(p_sentence_msg);
	}


	// register in globals
	if( GLOBALS::get().sentences_map_mtx_.try_lock_for(Ms(1000)) )
	{
		GLOBALS::get().sentences_map_[t.frame] = sentence;
		GLOBALS::get().stats_.num_ok_ = GLOBALS::get().sentences_map_.size();
		GLOBALS::get().sentences_map_mtx_.unlock();
	}


	// save opts
	try {
		GLOBALS::DumpToFile("./habdecWebsocketServer.opts");
	}
	catch(const exception& e) {
		cout<<"Failed saving options "<<e.what()<<endl;
	}


	// habitat upload
	if(GLOBALS::get().par_.station_callsign_ != "")
	{
		try {
			string res = habdec::habitat::HabitatUploadSentence( sentence, GLOBALS::get().par_.station_callsign_ );
			if( res != "OK" )
				cout<<C_CLEAR<<C_RED<<"HAB Upload result: "<<res<<endl;
		}
		catch(const exception& e) {
			cout<<"Failed upload to habitat: "<<e.what()<<endl;
		}
	}


	// print distance and elevation
	if( GLOBALS::get().par_.station_lat_ )
	{
		stats.D_ = habdec::CalcGpsDistance(
				GLOBALS::get().par_.station_lat_, GLOBALS::get().par_.station_lon_,
				GLOBALS::get().par_.station_alt_,
				t.lat, t.lon, t.alt );

		stats.dist_max_ = max(stats.dist_max_, stats.D_.dist_line_);
		stats.elev_min_ = min(stats.elev_min_, stats.D_.elevation_);

		cout<<fixed<<setprecision(0)<<"Distance: "<<stats.D_.dist_line_<<" ["<<stats.dist_max_
									<<"] (great circle: "<<stats.D_.dist_circle_<<")"
			<<fixed<<setprecision(2)<<"\tElevation: "<<stats.D_.elevation_
									<<  " ["<<stats.elev_min_<<"] "<<endl;
		cout<<scientific;
	}


	// CLI callback
	if( GLOBALS::get().par_.sentence_cmd_ != "" )
	{
		int res = system( (GLOBALS::get().par_.sentence_cmd_ + " " + sentence).c_str() );
		if(res)
			cout<<C_CLEAR<<C_RED<<"sentence_cmd result: "<<res<<endl;
	}
}

void test_sentence_parsing_and_upload(std::string test_sentence, std::string api_endpoint)
{
	using namespace std;
	using namespace boost;

	sondehub::MinTelemetry t;
	try {
		auto maybe_telemetry = habdec::parse_sentence(test_sentence);
		if(!maybe_telemetry) {
			cout<<"Failed parsing sentence to minimal telemetry"<<endl;
			return;
		}
		else {
			t = maybe_telemetry.value();
		}
	} catch(std::exception const& e) {
		cout<<"Failed parsing sentence to minimal telemetry: "<<e.what()<<endl;
		return;
	}

	//upload to sondehub
	//
	string tele_json =
R"([{
	"software_name": "habdec",
	"software_version": "4ec86fbad93fb30e073f29be0eef53b437c2c64b",
	"uploader_callsign": "$uploader_callsign",
	"time_received": "$time_received",
	"upload_time": "$upload_time",
	"payload_callsign": "$payload_callsign",
	"datetime": "$datetime",
	"lat": $lat,
	"lon": $lon,
	"alt": $alt
}])";

	string utc_now_str = utc_now_iso();
	boost::replace_all(tele_json, "$uploader_callsign", "SP7HAB");
	boost::replace_all(tele_json, "$time_received", utc_now_str);
	boost::replace_all(tele_json, "$upload_time", utc_now_str);
	boost::replace_all(tele_json, "$payload_callsign", t.payload_callsign);
	boost::replace_all(tele_json, "$datetime", t.datetime);
	boost::replace_all(tele_json, "$lat", to_string(t.lat));
	boost::replace_all(tele_json, "$lon", to_string(t.lon));
	boost::replace_all(tele_json, "$alt", to_string(static_cast<int>(t.alt)));
	cout<<tele_json<<endl;

	return;

	cout<<endl<<api_endpoint<<endl;
	cpr::Response r = cpr::Put(
				cpr::Url{api_endpoint},
				cpr::Body{tele_json},
				cpr::Header{
					{"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:105.0) Gecko/20100101 Firefox/105.0"},
					{"Content-Type", "application/json"}
				}
			);

	cout<<r.status_code<<endl;                  // 200
	cout<<r.header["content-type"]<<endl;       // application/json; charset=utf-8
	cout<<r.text<<endl;

}


void Test_SentenceGenerator(
		std::shared_ptr<WebsocketServer> p_ws_server,
		// sondehub::SondeHubUploader& sondehub_uploader
		std::shared_ptr<sondehub::SondeHubUploader> p_sondehub_uploader
	)
{
	using namespace std;
	using namespace chrono;
	using namespace date;


	static int sentence_id = 0;
	while(1) {
		sentence_id++;
		this_thread::sleep_for(( duration<double, milli>(6 * 1000) ));

		auto sys_now = system_clock::now();
    	auto sys_HMS = make_time(sys_now - floor<days>(sys_now) );

		stringstream ss;
		// ss << "SP7HAB-test,";
		ss << sentence_id;
		ss << "," << sys_HMS;
		if(sentence_id%2)
			ss << ",52.5,21.25";
		else
			ss << ",5230.1,02115.1"; // NMEA
		ss << ","<<int(5000 + sentence_id*00);
		ss << ",-25.3";

		string data{ ss.str() };
		// cout<<data<<endl;

		SentenceCallback( 	"SP7HAB-test", data, habdec::CRC(string("SP7HAB-test") + string(",") + data),
							p_ws_server,
							p_sondehub_uploader );
	}
}


int main(int argc, char** argv)
{
	using namespace std;

	signal( SIGINT, 	[](int){exit(1);} );
	signal( SIGILL, 	[](int){exit(1);} );
	signal( SIGFPE, 	[](int){exit(1);} );
	signal( SIGSEGV, 	[](int){exit(1);} );
	signal( SIGTERM, 	[](int){exit(1);} );
	signal( SIGABRT, 	[](int){exit(1);} );

	// thousands separator
	struct thousand_separators : numpunct<char>
	{
		char do_thousands_sep() const { return ','; }
		string do_grouping() const { return "\3"; }
	};
	try{
		cout.imbue( locale(locale(""), new thousand_separators) );
	}
	catch(exception& e)	{

	}

	cout<<"git version: "<<g_GIT_SHA1<<endl;
	cout<<"SOAPY_SDR_API_VERSION: "<<SoapySDR::getAPIVersion()<<endl;

	// setup GLOBALS
	prog_opts(argc, argv);

	auto& G = GLOBALS::get();

	if(	G.par_.test_parse_sentence_ != "" ) {
		test_sentence_parsing_and_upload(
			G.par_.test_parse_sentence_,
			G.par_.sondehub_ + string("/amateur/telemetry") );
		return 0;
	}


	// setup SoapySDR device
	SoapySDR::Kwargs device;
	while(true)
	{
		bool ok = false;
		try {
			ok = SetupDevice(device);
		} catch(std::runtime_error& er) {
			cout<<C_RED<<"Failed opening device: "
				<<C_MAGENTA<<er.what()<<C_OFF<<endl;
		}

		if( ok )
		{
			break;
		}
		else
		{
			if( G.par_.no_exit_ )
			{
				cout<<C_RED<<"Failed Device Setup. Retry."<<C_OFF<<endl;
				std::this_thread::sleep_for( ( std::chrono::duration<double, std::milli>(3000) ));
				continue;
			}
			else
			{
				cout<<C_RED<<"Failed Device Setup. EXIT."<<C_OFF<<endl;
				return 1;
			}
		}
	}


	// station info
	if(	G.par_.station_callsign_ != "" )
	{
		habdec::habitat::UploadStationInfo(	G.par_.station_callsign_,
											device["driver"] + " - habdec" );

		if(		G.par_.station_lat_
			&& 	G.par_.station_lon_ ) {

					// habitat
					cout<<"Uploading station info to HabHub."<<endl;
					habdec::habitat::UploadStationTelemetry(
						G.par_.station_callsign_,
						G.par_.station_lat_, G.par_.station_lon_,
						G.par_.station_alt_, 0, false
					);

					// sondehub
					string sondehub_station_json = R"(
						{
							"uploader_callsign": "$uploader_callsign",
							"uploader_position": [$lat,$lon,$alt],
							"software_name": "habdec",
							"software_version": "$software_version",
							"uploader_radio": "$uploader_radio",
							"mobile": false
						}
					)";
        			boost::replace_all(sondehub_station_json, "$uploader_callsign", G.par_.station_callsign_);
        			boost::replace_all(sondehub_station_json, "$lat", to_string(G.par_.station_lat_));
        			boost::replace_all(sondehub_station_json, "$lon", to_string(G.par_.station_lon_));
        			boost::replace_all(sondehub_station_json, "$alt", to_string(G.par_.station_alt_));
			        boost::replace_all(sondehub_station_json, "$software_version", string(g_GIT_SHA1).substr(0,7));
			        boost::replace_all(sondehub_station_json, "$uploader_radio", device["driver"]);

					cout<<"Uploading station info to SondeHub."<<endl;
					cout<<sondehub_station_json<<endl;

					cpr::Response r = cpr::Put(
						cpr::Url{G.par_.sondehub_ + "/amateur/listeners"},
						cpr::Body{sondehub_station_json},
						cpr::Header{
							{"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:105.0) Gecko/20100101 Firefox/105.0"},
							{"Content-Type", "application/json"}
						}
					);

					if(r.status_code != 200) {
						cout<<"sondehub station info upload error:\n";
						cout<<r.status_code<<endl;                  // 200
						cout<<r.header["content-type"]<<endl;       // application/json; charset=utf-8
						cout<<r.text<<endl;
					}
		}
	}

	// initial options from globals
	//
	auto& DECODER = G.decoder_;
	DECODER.baud(G.par_.baud_);
	DECODER.rtty_bits(G.par_.rtty_ascii_bits_);
	DECODER.rtty_stops(G.par_.rtty_ascii_stops_);
	DECODER.livePrint( G.par_.live_print_ );
	DECODER.dc_remove( G.par_.dc_remove_ );
	DECODER.lowpass_bw( G.par_.lowpass_bw_Hz_ );
	DECODER.lowpass_trans( G.par_.lowpass_tr_ );
	int _decim = G.par_.decimation_;
	DECODER.setupDecimationStagesFactor( pow(2,_decim) );
	DECODER.ssdvBaseFile( G.par_.ssdv_dir_ + "/ssdv_" );

	double freq = G.par_.frequency_;
	G.p_iq_source_->setOption("frequency_double", &freq);

	if(G.par_.station_callsign_ == "")
		cout<<C_RED<<"No --station parameter set. HAB Upload disabled."<<C_OFF<<endl;

	cout<<"Current Options: "<<endl;
	GLOBALS::Print();


	// websocket server
	auto p_ws_server = make_shared<WebsocketServer>(G.par_.command_host_ , G.par_.command_port_);

	// sondehub uploader
	auto p_sondehub_uploader = make_shared<sondehub::SondeHubUploader>(G.par_.sondehub_ + "/amateur/telemetry", G.par_.station_callsign_);


	DECODER.sentence_callback_ =
		[p_ws_server, p_sondehub_uploader](string callsign, string data, string crc)
		{
			async(launch::async, SentenceCallback, callsign, data, crc, p_ws_server, p_sondehub_uploader);
		};

	DECODER.character_callback_ =
		[p_ws_server](string rtty_characters)
		{
			shared_ptr<HabdecMessage> p_msg = make_shared<HabdecMessage>();
			p_msg->data_stream_<<"cmd::info:liveprint="<<rtty_characters;
			p_ws_server->sessions_send(p_msg);
		};

	DECODER.ssdv_callback_ =
		[p_ws_server](string callsign, int image_id, std::vector<uint8_t> jpeg)
		{
			shared_ptr<HabdecMessage> p_msg = make_shared<HabdecMessage>();
			p_msg->is_binary_ = true;
			// p_msg->to_all_clients_ = true;

			// header
			pair<int,int> ssdv_header{ (int)callsign.size(), (int)image_id };
			p_msg->data_stream_<<"SDV_";
			p_msg->data_stream_.write( reinterpret_cast<char*>(&ssdv_header), sizeof(ssdv_header) );
			p_msg->data_stream_<<callsign;
			// base64 encoded JPEG bytes
			string b64_jpeg_out = macaron::Base64().Encode( string((char*)jpeg.data(), jpeg.size()) );
			for(size_t i = 0; i<b64_jpeg_out.size(); ++i)
				p_msg->data_stream_<<(char)b64_jpeg_out[i];
			p_ws_server->sessions_send(p_msg);
		};



	// START THREADS
	//
	unordered_set<thread*> threads;

	// websocket server
	threads.emplace( new thread(
		[p_ws_server]() { (*p_ws_server)(); }
	) );

	// decoder
	threads.emplace( new thread(
		DECODER_THREAD
		// [p_ws_server, p_sondehub_uploader]() {
		// 	Test_SentenceGenerator(p_ws_server, p_sondehub_uploader);
		// }
	) );

	// sondehub uploader
	if(G.par_.station_callsign_ != "" && G.par_.sondehub_ != "") {
		cout<<"Uploading to sondehub "<<G.par_.sondehub_<<endl;
		threads.emplace( new thread(
			[p_sondehub_uploader]() {
				while(1) {
					this_thread::sleep_for(( std::chrono::duration<double, std::milli>(15 * 1000) ));
					p_sondehub_uploader->upload();
				}
			}
		) );
	}


	for(auto t : threads)
		t->join();

	return 0;
}
