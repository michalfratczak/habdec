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
#include <limits>


#include "IQSource/IQSource.h"
#include "Decoder/Decoder.h"
#include "common/GpsDistance.h"


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

	std::vector<TDecoder::TValue>	demod_accumulated_; // acuumulated demod samples, used for GUI display
	std::mutex demod_accumulated_mtx_;

	// decoded sentences, key is sentence id
	std::map<int, std::string>			sentences_map_;
	std::timed_mutex					sentences_map_mtx_;

	struct STATS
	{
		unsigned int num_ok_ = 0; // num of decoded packets
		habdec::GpsDistance D_;
		double dist_max_ = 0;
		double elev_min_ = 90.0;
		std::chrono::steady_clock::time_point last_sentence_timestamp_ = std::chrono::steady_clock::now();
	};
	STATS stats_;

	// options
	struct PARAMS
	{
		int 			device_ = 0; 	// index to SoapySDR::Device::enumerate()
		std::string 	command_host_ = "0.0.0.0";
		int 		 	command_port_ = 5555;
		std::string 	station_callsign_ = ""; // habitat upload
		float 	station_lat_ = 0;
		float 	station_lon_ = 0;
		float 	station_alt_ = 0;
		double	sampling_rate_ = 0;
		int 	decimation_ = 6;
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
		float	lowpass_bw_Hz_ = 1500;
		float	lowpass_tr_ = 0.025;
		bool	no_exit_ = false;
		std::string 	sentence_cmd_ = "";
		std::string 	habitat_payload_ = "";
		std::string		coord_format_lat_ = "dd.dddd"; // encoding of lat/lon coords: dd.dddd or ddmm.mmmm
		std::string		coord_format_lon_ = "dd.dddd"; // encoding of lat/lon coords: dd.dddd or ddmm.mmmm
		std::string		ssdv_dir_ = ".";
		// int 	datasize_ = 1;
		TransportDataType  	transport_data_type_ = TransportDataType::kChar;


		bool operator==(const PARAMS& rhs) const
		{
			return
				   frequency_ == rhs.frequency_
				&& gain_ == rhs.gain_
				&& lowpass_bw_Hz_ == rhs.lowpass_bw_Hz_
				&& lowpass_tr_ == rhs.lowpass_tr_
				&& decimation_ == rhs.decimation_
				&& biast_ == rhs.biast_
				&& baud_ == rhs.baud_
				&& rtty_ascii_bits_ == rhs.rtty_ascii_bits_
				&& rtty_ascii_stops_ == rhs.rtty_ascii_stops_
				&& live_print_ == rhs.live_print_
				&& afc_ == rhs.afc_
				&& ppm_ == rhs.ppm_
				&& dc_remove_ == rhs.dc_remove_
				&& transport_data_type_ == rhs.transport_data_type_
				&& usb_pack_ == rhs.usb_pack_
				&& device_ == rhs.device_
				&& command_host_ == rhs.command_host_
				&& command_port_ == rhs.command_port_
				&& station_callsign_ == rhs.station_callsign_
				&& station_lat_ == rhs.station_lat_
				&& station_lon_ == rhs.station_lon_
				&& station_alt_ == rhs.station_alt_
				&& sampling_rate_ == rhs.sampling_rate_
				&& sentence_cmd_ == rhs.sentence_cmd_
				&& habitat_payload_ == rhs.habitat_payload_
				&& no_exit_ == rhs.no_exit_
				&& coord_format_lat_ == rhs.coord_format_lat_
				&& coord_format_lon_ == rhs.coord_format_lon_
				;
		}
		bool operator!=(const PARAMS& rhs) const { return !operator==(rhs); }
	};
	PARAMS par_;

	static bool DumpToFile(std::string fName)
	{
		using namespace std;
		try{
			fstream oFile(fName, fstream::out);

			oFile<<"device = "<<GLOBALS::get().par_.device_<<endl;
			oFile<<"sampling_rate = "<<GLOBALS::get().par_.sampling_rate_<<endl;
			oFile<<"port = "<<GLOBALS::get().par_.command_host_<<":"<<GLOBALS::get().par_.command_port_<<endl;
			oFile<<"station = "<<GLOBALS::get().par_.station_callsign_<<endl;
			oFile<<"latlon = "<<GLOBALS::get().par_.station_lat_<<endl;
			oFile<<"latlon = "<<GLOBALS::get().par_.station_lon_<<endl;
			oFile<<"alt = "<<GLOBALS::get().par_.station_alt_<<endl;

			oFile<<"freq = "<<setprecision(9)<<GLOBALS::get().par_.frequency_/1e6<<endl;
			oFile<<"dec = "<<GLOBALS::get().par_.decimation_<<endl;
			oFile<<"ppm = "<<setprecision(9)<<GLOBALS::get().par_.ppm_<<endl;
			oFile<<"gain = "<<GLOBALS::get().par_.gain_<<endl;
			oFile<<"biast = "<<GLOBALS::get().par_.biast_<<endl;
			oFile<<"print = "<<GLOBALS::get().par_.live_print_<<endl;
			oFile<<"rtty = "<<GLOBALS::get().par_.baud_<<endl;
			oFile<<"rtty = "<<GLOBALS::get().par_.rtty_ascii_bits_<<endl;
			oFile<<"rtty = "<<GLOBALS::get().par_.rtty_ascii_stops_<<endl;
			oFile<<"afc = "<<GLOBALS::get().par_.afc_<<endl;
			oFile<<"usb_pack = "<<GLOBALS::get().par_.usb_pack_<<endl;
			oFile<<"dc_remove = "<<GLOBALS::get().par_.dc_remove_<<endl;
			oFile<<"lowpass = "<<GLOBALS::get().par_.lowpass_bw_Hz_<<endl;
			oFile<<"lp_trans = "<<GLOBALS::get().par_.lowpass_tr_<<endl;
			oFile<<"sentence_cmd = "<<GLOBALS::get().par_.sentence_cmd_<<endl;
			oFile<<"payload = "<<GLOBALS::get().par_.habitat_payload_<<endl;
			oFile<<"no_exit = "<<GLOBALS::get().par_.no_exit_<<endl;
			oFile	<<"nmea = "<< ( 	GLOBALS::get().par_.coord_format_lat_ == "ddmm.mmmm"
									&& 	GLOBALS::get().par_.coord_format_lon_ == "ddmm.mmmm" )<<endl;

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
		cout<<"\tdevice: "<<GLOBALS::get().par_.device_<<endl;
		cout<<"\tsampling_rate: "<<GLOBALS::get().par_.sampling_rate_<<endl;
		cout<<"\tdecimation: "<<GLOBALS::get().par_.decimation_<<endl;
		cout<<"\tcommand_host: "<<GLOBALS::get().par_.command_host_<<endl;
		cout<<"\tcommand_port: "<<GLOBALS::get().par_.command_port_<<endl;
		cout<<"\tsentence_cmd: "<<GLOBALS::get().par_.sentence_cmd_<<endl;
		cout<<"\tpayload: "<<GLOBALS::get().par_.habitat_payload_<<endl;
		cout<<"\tstation: "<<GLOBALS::get().par_.station_callsign_<<endl;
		cout<<"\tlatlon: "<<GLOBALS::get().par_.station_lat_<<" "<<GLOBALS::get().par_.station_lon_<<endl;
		cout<<"\talt: "<<GLOBALS::get().par_.station_alt_<<endl;
		cout<<"\tfreq: "<<GLOBALS::get().par_.frequency_<<endl;
		cout<<"\tppm: "<<GLOBALS::get().par_.ppm_<<endl;
		cout<<"\tgain: "<<GLOBALS::get().par_.gain_<<endl;
		cout<<"\tlive_print: "<<GLOBALS::get().par_.live_print_<<endl;
		cout<<"\tbaud: "<<GLOBALS::get().par_.baud_<<endl;
		cout<<"\trtty_ascii_bits: "<<GLOBALS::get().par_.rtty_ascii_bits_<<endl;
		cout<<"\trtty_ascii_stops: "<<GLOBALS::get().par_.rtty_ascii_stops_<<endl;
		cout<<"\tbiast: "<<GLOBALS::get().par_.biast_<<endl;
		cout<<"\tusb_pack: "<<GLOBALS::get().par_.usb_pack_<<endl;
		cout<<"\tdc_remove: "<<GLOBALS::get().par_.dc_remove_<<endl;
		cout<<"\tlowpass: "<<GLOBALS::get().par_.lowpass_bw_Hz_<<endl;
		cout<<"\tlp_trans: "<<GLOBALS::get().par_.lowpass_tr_<<endl;
		cout<<"\tno_exit: "<<GLOBALS::get().par_.no_exit_<<endl;
		cout<<"\tnmea = "<< ( 	GLOBALS::get().par_.coord_format_lat_ == "ddmm.mmmm"
							&& 	GLOBALS::get().par_.coord_format_lon_ == "ddmm.mmmm" )<<endl;
	}


private:
	GLOBALS() {};

};