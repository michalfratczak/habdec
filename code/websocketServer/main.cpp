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

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>


#include "IQSource/IQSource_SoapySDR.h"
#include "Decoder/Decoder.h"
#include "common/console_colors.h"
#include "habitat/habitat_interface.h"
#include "GLOBALS.h"
#include "server.h"


bool G_DO_EXIT = false;

using namespace std;


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
	}
}


bool SetupDevice()
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

	int device_index = GLOBALS::get().device_;
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

	if(!GLOBALS::get().sampling_rate_)
	{
		PrintDevicesList(device_list);
		cout<<C_RED<<"Sampling Rate not set."<<C_OFF<<endl;
		return false;
	}

	// setup DEVICE
	//
	auto& device = device_list[device_index];
	cout<<"Running with "<<device["driver"]<<endl<<endl;

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

	double freq = GLOBALS::get().frequency_;
	GLOBALS::get().p_iq_source_->setOption("frequency_double", &freq);
	double gain = GLOBALS::get().gain_;
	GLOBALS::get().p_iq_source_->setOption("gain_double", &gain);
	double biastee = GLOBALS::get().biast_;
	GLOBALS::get().p_iq_source_->setOption("biastee_double", &biastee);
	GLOBALS::get().p_iq_source_->setOption("sampling_rate_double", &GLOBALS::get().sampling_rate_);

	return true;
}


void DECODER_FEED_THREAD()
{
	using namespace std;
	cout<<"Start DECODER_FEED_THREAD"<<endl;

	auto& p_iq_src = GLOBALS::get().p_iq_source_;

	if(!p_iq_src)
		return;

	if(!p_iq_src->start())
	{
		cout<<C_RED<<"Failed to start device. Exit DECODER_FEED_THREAD."<<C_OFF<<endl;
		return;
	}

	if(!p_iq_src->isRunning())
	{
		cout<<C_RED<<"Device not running. Exit DECODER_FEED_THREAD."<<C_OFF<<endl;
		return;
	}

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
			if(GLOBALS::get().afc_)
			{
				if( 100 < abs(freq_corr)  )
				{
					GLOBALS::get().frequency_ += freq_corr;
					double f = GLOBALS::get().frequency_;
					p_iq_src->setOption("frequency_double", &f);
					DECODER.resetFrequencyCorrection(freq_corr);
					last_afc_time = std::chrono::high_resolution_clock::now();

					// notify webgui
					{
						lock_guard<mutex> _lock( GLOBALS::get().out_commands_mtx_ );
						GLOBALS::get().out_commands_.emplace_back("cmd::set:frequency=" + to_string(f / 1e6));
					}
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

void LOG_THREAD()
{
	using namespace std;
	using namespace habdec;

	while(!G_DO_EXIT)
	{
		this_thread::sleep_for( chrono::duration<double, milli>(500) );

		vector<string> sentences;
		{
			lock_guard<mutex> _lock( GLOBALS::get().senteces_to_log_mtx_ );
			sentences = move( GLOBALS::get().senteces_to_log_ );
		}

		for(auto& sentence : sentences)
		{
			if(GLOBALS::get().station_callsign_ != "")
			{
				string res = HabitatUploadSentence( sentence, GLOBALS::get().station_callsign_ );
				if( res != "OK" )
					cout<<C_CLEAR<<C_RED<<"HAB Upload result: "<<res<<endl;
			}

			if( GLOBALS::get().sentence_cmd_ != "" )
			{
				int res = system( (GLOBALS::get().sentence_cmd_ + " " + sentence).c_str() );
				if(res)
					cout<<C_CLEAR<<C_RED<<"sentence_cmd result: "<<res<<endl;
			}
		}
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

	// setup GLOBALS
	prog_opts(argc, argv);

	// setup SoapySDR device
	if(!SetupDevice())
	{
		cout<<C_RED<<"Failed Device Setup. EXIT."<<C_OFF<<endl;
		return 1;
	}

	// initial options
	auto& DECODER = GLOBALS::get().decoder_;
	DECODER.baud(GLOBALS::get().baud_);
	DECODER.rtty_bits(GLOBALS::get().rtty_ascii_bits_);
	DECODER.rtty_stops(GLOBALS::get().rtty_ascii_stops_);
	DECODER.lowpass_bw(.05);
	DECODER.lowpass_trans(.0025);
	DECODER.livePrint( GLOBALS::get().live_print_ );

	if(GLOBALS::get().station_callsign_ == "")
		cout<<C_RED<<"No --station parameter set. HAB Upload disabled."<<C_OFF<<endl;

	// for every decoded message
	// put it on two ques: websocket upload, HAB upload
	DECODER.success_callback_ =
		[](std::string callsign, std::string data, std::string crc)
		{
			{
				lock_guard<mutex> _lock( GLOBALS::get().senteces_to_log_mtx_ );
				GLOBALS::get().senteces_to_log_.emplace_back( callsign + "," + data + "*" + crc );
			}
			{
				lock_guard<mutex> _lock( GLOBALS::get().senteces_to_web_mtx_ );
				GLOBALS::get().senteces_to_web_.emplace_back( callsign + "," + data + "*" + crc );
			}
			GLOBALS::DumpToFile("./habdecWebsocketServer.opts");
		};

	// feed decoder with IQ samples
	std::thread* decoder_feed_thread = new std::thread(DECODER_FEED_THREAD);

	// HAB upload
	std::thread* 	log_thread = new std::thread(LOG_THREAD);

	// websocket server thread. this call is blocking
	RunCommandServer( GLOBALS::get().command_host_ , GLOBALS::get().command_port_ );

	G_DO_EXIT = true;
	decoder_feed_thread->join();
	log_thread->join();

	return 0;
}
