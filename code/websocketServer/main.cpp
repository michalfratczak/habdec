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

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Version.hpp>


#include "IQSource/IQSource_SoapySDR.h"
#include "Decoder/Decoder.h"
#include "common/console_colors.h"
#include "habitat/habitat_interface.h"
#include "GLOBALS.h"
#include "server.h"
#include "common/git_repo_sha1.h"
#include "common/GpsDistance.h"


bool G_DO_EXIT = false;

using namespace std;


std::vector<std::string> split_string(const std::string& text, char sep)
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


void SentenceToPosition(std::string i_snt, float &lat, float &lon, float &alt)
{
    // "CALLSIGN,123,15:41:24,44.32800,-74.14427,00491,0,0,12,30.7,0.0,0.001,20.2,958*6BC9"
    std::vector<std::string> tokens = split_string(i_snt, ',');
    lat = std::stof(tokens[3]);
    lon = std::stof(tokens[4]);
    alt = std::stof(tokens[5]);
}


void PrintDevicesList(const SoapySDR::KwargsList& device_list)
{
	int DEV_NUM = 0;
	cout<<endl;
	for(const auto& dev : device_list)
	{
		cout<<C_RED<<DEV_NUM++<<":"<<C_OFF<<endl;
		for(const auto& kv : dev)
			cout<<'\t'<<kv.first<<" "<<kv.second<<endl;

		// print sampling rates
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
	if( device_index >= int(device_list.size()) )
	{
		cout<<C_RED<<"No device with number "<<device_index<<endl;
		return false;
	}

	if( device_list.size() == 1 )
	{
		device_index = 0;
	}
	else if(device_index < 0)
	{
		cout<<C_MAGENTA<<"No SDR device specified. Select one by specifying it's"<<C_RED<<" NUMBER"<<C_OFF<<endl;
		cout<<C_MAGENTA<<"Available Devices:"<<C_OFF<<endl;

		int DEV_NUM = 0;
		PrintDevicesList(device_list);
		cout<<C_OFF<<endl;
		return false;
	}

	// setup DEVICE
	//
	auto& device = device_list[device_index];
	cout<<"Running with "<<device["driver"]<<endl<<endl;

	if( !GLOBALS::get().par_.sampling_rate_ )
	{
		// find what is nearest to 2e6
		double sr_diff = std::numeric_limits<double>::max();
		auto p_device = SoapySDR::Device::make(device);
		for(double sr : p_device->listSampleRates(SOAPY_SDR_RX, 0))
		{
			if( abs(sr - 2e6) < sr_diff )
			{
				sr_diff = abs(sr - 2e6);
				GLOBALS::get().par_.sampling_rate_ = sr;
			}
		}
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

	typedef std::chrono::nanoseconds TDur;

	auto& DECODER = GLOBALS::get().decoder_;

	habdec::IQVector<TReal>	samples;
	samples.resize(256*256);
	samples.samplingRate( p_iq_src->samplingRate() );

	while(!G_DO_EXIT)
	{
		auto _start = std::chrono::high_resolution_clock::now();

		size_t count = p_iq_src->get( samples.data(), samples.size() );
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


		TDur _duration = std::chrono::duration_cast<TDur>(std::chrono::high_resolution_clock::now() - _start);

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


void SentenceCallback(std::string callsign, std::string data, std::string crc)
{
	using namespace std;
	using Ms = std::chrono::milliseconds;

	string sentence = callsign + "," + data + "*" + crc;
	int sentence_number = stoi( data.substr(0, data.find(',')) );

	// register in globals
	if( GLOBALS::get().sentences_map_mtx_.try_lock_for(Ms(1000)) )
	{
		GLOBALS::get().sentences_map_[sentence_number] = sentence;
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
		float lat, lon, alt;
		SentenceToPosition(sentence, lat, lon, alt);
		habdec::GpsDistance _D = habdec::CalcGpsDistance(
				GLOBALS::get().par_.station_lat_, GLOBALS::get().par_.station_lon_, 0, // station
				lat, lon, alt );  // payload
		static thread_local double distance_max = 0;
		distance_max =  max(distance_max, _D.dist_circle_);
		static thread_local double elevation_min = 0;
		elevation_min = min(elevation_min, _D.elevation_);

		cout<<std::fixed<<std::setprecision(0)<<"Distance: "<<_D.dist_circle_<<" ["<<distance_max<<"] "
			<<std::fixed<<std::setprecision(2)<<"\tElevation: "<<_D.elevation_<<  " ["<<elevation_min<<"] "<<endl;
		cout<<scientific;
	}


	// user callback
	if( GLOBALS::get().par_.sentence_cmd_ != "" )
	{
		int res = system( (GLOBALS::get().par_.sentence_cmd_ + " " + sentence).c_str() );
		if(res)
			cout<<C_CLEAR<<C_RED<<"sentence_cmd result: "<<res<<endl;
	}
}


int main(int argc, char** argv)
{
	signal( SIGINT, 	[](int){exit(1);} );
	signal( SIGILL, 	[](int){exit(1);} );
	signal( SIGFPE, 	[](int){exit(1);} );
	signal( SIGSEGV, 	[](int){exit(1);} );
	signal( SIGTERM, 	[](int){exit(1);} );
	signal( SIGABRT, 	[](int){exit(1);} );

	// thousands separator
	struct thousand_separators : std::numpunct<char>
	{
		char do_thousands_sep() const { return ','; }
		string do_grouping() const { return "\3"; }
	};
	try{
		std::cout.imbue( std::locale(locale(""), new thousand_separators) );
	}
	catch(exception& e)	{

	}

	cout<<"git version: "<<g_GIT_SHA1<<endl;
	cout<<"SOAPY_SDR_API_VERSION: "<<SoapySDR::getAPIVersion()<<endl;

	// setup GLOBALS
	prog_opts(argc, argv);

	// setup SoapySDR device
	SoapySDR::Kwargs device;
	if(!SetupDevice(device))
	{
		cout<<C_RED<<"Failed Device Setup. EXIT."<<C_OFF<<endl;
		return 1;
	}

	// station info
	if(	GLOBALS::get().par_.station_callsign_ != "" )
	{
		habdec::habitat::UploadStationInfo(	GLOBALS::get().par_.station_callsign_,
											device["driver"] + " - habdec" );

		if(		GLOBALS::get().par_.station_lat_
			&& 	GLOBALS::get().par_.station_lon_ )
					habdec::habitat::UploadStationTelemetry(
						GLOBALS::get().par_.station_callsign_,
						GLOBALS::get().par_.station_lat_, GLOBALS::get().par_.station_lon_,
						0, 0, false
					);
	}

	// initial options from globals
	//
	auto& DECODER = GLOBALS::get().decoder_;
	DECODER.baud(GLOBALS::get().par_.baud_);
	DECODER.rtty_bits(GLOBALS::get().par_.rtty_ascii_bits_);
	DECODER.rtty_stops(GLOBALS::get().par_.rtty_ascii_stops_);
	DECODER.livePrint( GLOBALS::get().par_.live_print_ );
	DECODER.dc_remove( GLOBALS::get().par_.dc_remove_ );
	DECODER.lowpass_bw( GLOBALS::get().par_.lowpass_bw_Hz_ );
	DECODER.lowpass_trans( GLOBALS::get().par_.lowpass_tr_ );
	int _decim = GLOBALS::get().par_.decimation_;
	DECODER.setupDecimationStagesFactor( pow(2,_decim) );

	double freq = GLOBALS::get().par_.frequency_;
	GLOBALS::get().p_iq_source_->setOption("frequency_double", &freq);

	if(GLOBALS::get().par_.station_callsign_ == "")
		cout<<C_RED<<"No --station parameter set. HAB Upload disabled."<<C_OFF<<endl;

	cout<<"Current Options: "<<endl;
	GLOBALS::Print();

	DECODER.success_callback_ =
		[](std::string callsign, std::string data, std::string crc)
		{
			std::async(std::launch::async, SentenceCallback, callsign, data, crc);
		};

	// feed decoder with IQ samples and decode
	std::thread* decoder_thread = new std::thread(DECODER_THREAD);

	// websocket server thread. this call is blocking
	RunCommandServer( GLOBALS::get().par_.command_host_ , GLOBALS::get().par_.command_port_ );

	G_DO_EXIT = true;
	decoder_thread->join();

	return 0;
}
