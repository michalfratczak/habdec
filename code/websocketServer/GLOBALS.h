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

#pragma once

#include <string>
#include <iomanip>
#include <fstream>
#include <vector>
#include <mutex>


#include "IQSource/IQSource.h"
#include "Decoder/Decoder.h"


typedef float TReal; // we operate on float IQ samples
typedef habdec::Decoder<TReal> TDecoder;


// singleton class keeping all global data
class GLOBALS
{
public:
	GLOBALS(GLOBALS const&)			= delete;
	void operator=(GLOBALS const&)	= delete;

	static GLOBALS& get()
	{
		static GLOBALS instance_;
		return instance_;
	}

	// decoder and IQ source
	TDecoder			decoder_;
	std::unique_ptr<habdec::IQSource> 	p_iq_source_;

	enum class TransportDataType {kUnknown=0, kChar, kShort, kFloat}; // 8, 16, 32 bits
	TransportDataType  	transport_data_type_ = TransportDataType::kChar;

	std::vector<TDecoder::TValue>	demod_accumulated_; // acuumulated demod samples, used for GUI display
	std::mutex demod_accumulated_mtx_;

	// decoded sentences que
	std::vector<std::string> 		senteces_to_log_;
	std::mutex 						senteces_to_log_mtx_;
	std::vector<std::string> 		senteces_to_web_;
	std::mutex 						senteces_to_web_mtx_;

	// outgoing commands. used to sync webgui
	std::vector<std::string> 		out_commands_;
	std::mutex 						out_commands_mtx_;

	// OPTS:
	int 			device_ = -1; 	// index to SoapySDR::Device::enumerate()
	std::string 	command_host_ = "0.0.0.0";
	int 		 	command_port_ = 5555;
	std::string 	station_callsign_ = ""; // habitat upload
	float 	station_lat_ = 0;
	float 	station_lon_ = 0;
	double	sampling_rate_ = 0;
	double 	frequency_ = 434349500.0f;
	double 	gain_ = 15;
	bool 	biast_ = false;
	float 	baud_ = 300;
	int 	rtty_ascii_bits_ = 8;
	float 	rtty_ascii_stops_ = 2;
	bool 	live_print_ = true;
	bool 	afc_ = false;
	double  ppm_ = 0;
	bool 	usb_pack_ = false; // airspy usb bitpack
	bool 	dc_remove_ = false;
	std::string 	sentence_cmd_ = "";
	std::string 	habitat_payload_ = "";

	static bool DumpToFile(std::string fName)
	{
		using namespace std;
		try{
			fstream oFile(fName, fstream::out);

			oFile<<"device = "<<GLOBALS::get().device_<<endl;
			oFile<<"sampling_rate = "<<GLOBALS::get().sampling_rate_<<endl;
			oFile<<"port = "<<GLOBALS::get().command_host_<<":"<<GLOBALS::get().command_port_<<endl;
			oFile<<"station = "<<GLOBALS::get().station_callsign_<<endl;
			oFile<<"latlon = "<<GLOBALS::get().station_lat_<<endl;
			oFile<<"latlon = "<<GLOBALS::get().station_lon_<<endl;

			oFile<<"freq = "<<setprecision(9)<<GLOBALS::get().frequency_/1e6<<endl;
			oFile<<"ppm = "<<setprecision(9)<<GLOBALS::get().ppm_<<endl;
			oFile<<"gain = "<<GLOBALS::get().gain_<<endl;
			oFile<<"biast = "<<GLOBALS::get().biast_<<endl;
			oFile<<"print = "<<GLOBALS::get().live_print_<<endl;
			oFile<<"rtty = "<<GLOBALS::get().baud_<<endl;
			oFile<<"rtty = "<<GLOBALS::get().rtty_ascii_bits_<<endl;
			oFile<<"rtty = "<<GLOBALS::get().rtty_ascii_stops_<<endl;
			oFile<<"afc = "<<GLOBALS::get().afc_<<endl;
			oFile<<"usb_pack = "<<GLOBALS::get().usb_pack_<<endl;
			oFile<<"dc_remove = "<<GLOBALS::get().dc_remove_<<endl;
			oFile<<"sentence_cmd = "<<GLOBALS::get().sentence_cmd_<<endl;
			oFile<<"payload = "<<GLOBALS::get().habitat_payload_<<endl;
		}
		catch (exception& e) {
			cout<<"Can't save config "<<fName<<endl;
			return false;
		}

		return true;
	}

	static void Print()
	{
		using namespace std;
		cout<<"\tdevice: "<<GLOBALS::get().device_<<endl;
		cout<<"\tsampling_rate: "<<GLOBALS::get().sampling_rate_<<endl;
		cout<<"\tcommand_host: "<<GLOBALS::get().command_host_<<endl;
		cout<<"\tcommand_port: "<<GLOBALS::get().command_port_<<endl;
		cout<<"\tsentence_cmd: "<<GLOBALS::get().sentence_cmd_<<endl;
		cout<<"\tpayload: "<<GLOBALS::get().habitat_payload_<<endl;
		cout<<"\tstation: "<<GLOBALS::get().station_callsign_<<endl;
		cout<<"\tlatlon: "<<GLOBALS::get().station_lat_<<" "<<GLOBALS::get().station_lon_<<endl;
		cout<<"\tfreq: "<<GLOBALS::get().frequency_<<endl;
		cout<<"\tppm: "<<GLOBALS::get().ppm_<<endl;
		cout<<"\tgain: "<<GLOBALS::get().gain_<<endl;
		cout<<"\tlive_print: "<<GLOBALS::get().live_print_<<endl;
		cout<<"\tbaud: "<<GLOBALS::get().baud_<<endl;
		cout<<"\trtty_ascii_bits: "<<GLOBALS::get().rtty_ascii_bits_<<endl;
		cout<<"\trtty_ascii_stops: "<<GLOBALS::get().rtty_ascii_stops_<<endl;
		cout<<"\tbiast: "<<GLOBALS::get().biast_<<endl;
		cout<<"\tusb_pack: "<<GLOBALS::get().usb_pack_<<endl;
		cout<<"\tdc_remove: "<<GLOBALS::get().dc_remove_<<endl;
	}


private:
	GLOBALS() {};

};