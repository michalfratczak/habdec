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


#include "IQSource_SoapySDR.h"

#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <stdexcept>

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Errors.hpp>

namespace habdec
{

bool IQSource_SoapySDR::start()
{
	if(!p_device_)
		return false;

	int result = 0;
	if(!b_is_running_)
		result = p_device_->activateStream(p_stream_);

	if(result)
	{
		std::cout<<"IQSource_SoapySDR::start error. "<<SoapySDR::errToStr(result)<<std::endl;
		return false;
	}

	b_is_running_ = true;
	return true;
}


bool IQSource_SoapySDR::stop()
{
	if(!p_device_)
		return false;

	int result = 0;
	if(b_is_running_)
    	result = p_device_->deactivateStream(p_stream_);

	if(result)
	{
		std::cout<<"IQSource_SoapySDR::stop error. "<<SoapySDR::errToStr(result)<<std::endl;
		return false;
	}

	b_is_running_ = false;
	return true;
}


bool IQSource_SoapySDR::isRunning() const
{
	if(!p_device_)
		return false;

	return b_is_running_;
}


size_t IQSource_SoapySDR::count() const
{
	return 0;
}


size_t IQSource_SoapySDR::get(void* p_data, const size_t i_count)
{
	if(!p_device_)
	{
		std::cout<<"p_device_ == 0"<<std::endl;
		return 0;
	}

	if(!p_stream_)
	{
		std::cout<<"p_stream_ == 0"<<std::endl;
		return 0;
	}

	if(!isRunning())
	{
		std::cout<<"Not Running."<<std::endl;
		return 0;
	}

	// const size_t buffs_size = 1;
	void* buffs[1] = { reinterpret_cast<void*>(p_data) };
	int receive_flags = 0;
	long long time_ns = 0;
	const int result = p_device_->readStream(p_stream_, buffs, i_count, receive_flags, time_ns);

	if(result<0)
	{
		std::cout<<"IQSource_SoapySDR::get error."<<SoapySDR::errToStr(result)<<std::endl;
		return 0;
	}

	return static_cast<size_t>(result);
}


double IQSource_SoapySDR::samplingRate() const
{
	return sampling_rate_;
}


IQSource_SoapySDR::~IQSource_SoapySDR()
{
	if(p_device_)
	{
		stop();
		p_device_->closeStream(p_stream_);
		//SoapySDR::Device::unmake(p_device_.get());
	}
}


bool IQSource_SoapySDR::init()
{
	p_device_.reset( SoapySDR::Device::make(soapysdr_kwargs_) );

	if(!p_device_)
		return false;

	using namespace std;

	cout<<"IQSource_SoapySDR::init"<<endl;

	const string driver_key = p_device_->getDriverKey();
	cout<<'\t'<<"driver_key "<<driver_key<<endl;

	hwinfo_hwkey_ = p_device_->getHardwareKey();

	const auto serial_it = soapysdr_kwargs_.find("serial");
	if(serial_it != soapysdr_kwargs_.end())
		hwinfo_serial_ = serial_it->second;

	// hw info
	// cout<<'\t'<<"hw_info:"<<endl;
	const SoapySDR::Kwargs hw_info = p_device_->getHardwareInfo();
	for(const auto& it : hw_info)
	{
		// cout<<"\t\tproperty:: "<<it.first<<":"<<it.second<<endl;
		const string property = it.first;
		// cout<<"Prop "<<property<<endl;

		try {
			if(property == "device_id")
			{
				hwinfo_device_id_ = std::stoi(it.second);
			}
		}
		catch(std::invalid_argument& e) {
			cout<<"Failed argument conversion for "<<property<<endl;
		}
	}

	// gains
	hwinfo_gains_.clear();
	for(const auto gain : p_device_->listGains(SOAPY_SDR_RX,0))
		hwinfo_gains_.push_back(gain);

	// freq range
	const SoapySDR::RangeList freq_range_list = p_device_->getFrequencyRange(SOAPY_SDR_RX, 0);
	hwinfo_freq_min_ = freq_range_list.front().minimum();
	hwinfo_freq_max_ = freq_range_list.back().maximum();

	// sampling rates
	hwinfo_sampling_rates_ = p_device_->listSampleRates(SOAPY_SDR_RX, 0);
	sort(hwinfo_sampling_rates_.begin(), hwinfo_sampling_rates_.end());

	// stream formats
	hwinfo_stream_formats_.clear();
	for(const auto stream_format : p_device_->getStreamFormats(SOAPY_SDR_RX, 0))  // doubles
		hwinfo_stream_formats_.push_back(stream_format);

	// gain range
	const SoapySDR::Range gain_range = p_device_->getGainRange(SOAPY_SDR_RX, 0);
	hwinfo_gain_min_ = gain_range.minimum();
	hwinfo_gain_max_ = gain_range.maximum();

	// init

	sampling_rate_ = hwinfo_sampling_rates_.back();
	if(hwinfo_hwkey_ != "Airspy")
		sampling_rate_ = hwinfo_sampling_rates_[3];
	p_device_->setSampleRate(SOAPY_SDR_RX, 0, sampling_rate_);

	p_device_->setGain( SOAPY_SDR_RX, 0, (hwinfo_gain_max_ - hwinfo_gain_min_)/2 );
	p_device_->setFrequency( SOAPY_SDR_RX, 0, 434274500 );
	p_stream_ = p_device_->setupStream(SOAPY_SDR_RX, SOAPY_SDR_CF32);


	// PRINT INFO
	/////////////////

	// hw info
	cout<<'\t'<<"hw_key "<<hwinfo_hwkey_<<endl;
	cout<<'\t'<<"device_id "<<hwinfo_device_id_<<endl;
	cout<<'\t'<<"serial "<<hwinfo_serial_<<endl;

	// gains
	cout<<"\tGains:\t";
	for(const auto& gain : hwinfo_gains_)
		cout<<gain<<", ";
	cout<<endl;

	cout<<"\tGain Range: "<<hwinfo_gain_min_<<" "<<hwinfo_gain_max_<<endl;

	// freq range
	cout<<"\tFreq Range: "<<hwinfo_freq_min_<<" "<<hwinfo_freq_max_<<endl;

	// sampling rates
	cout<<"\tSampling Ranges: ";
	for(const auto sampling_rate : hwinfo_sampling_rates_)
		cout<<sampling_rate<<", ";
	cout<<endl;

	// stream formats
	cout<<"\tStream Formats: ";
	for(const auto stream_format : hwinfo_stream_formats_)
		cout<<stream_format<<", ";
	cout<<endl;


	// others

	// nothing for AirSpy
	cout<<"\tFrontend mapping:"<<p_device_->getFrontendMapping(SOAPY_SDR_RX)<<endl;

	// nothing for AirSpy
	cout<<"\tBandwidths: ";
	for(const auto bandwidth : p_device_->listBandwidths(SOAPY_SDR_RX, 0))  // doubles
		cout<<bandwidth<<", ";
	cout<<endl;

	cout<<"\tHas DC Offset: "<<p_device_->hasDCOffsetMode(SOAPY_SDR_RX, 0)<<endl;

	cout<<"\tHas Freq Correction: "<<p_device_->hasFrequencyCorrection(SOAPY_SDR_RX, 0)<<endl;

	return true;

}


bool IQSource_SoapySDR::setOption(const std::string& option, const void* p_data)
{
	using namespace std;

	void* p_data_nonconst = const_cast<void*>(p_data);

	bool b_verbose = false;

	if(option == "SoapySDR_Kwargs")
	{
		soapysdr_kwargs_ = *reinterpret_cast<SoapySDR::Kwargs*>(p_data_nonconst);
		return true;
	}

	if(!p_device_)
	{
		cout<<"IQSource_SoapySDR::setOption no device initialized."<<endl;
	}

	if(option == "sampling_rate_double")
	{
		double value = *reinterpret_cast<double*>(p_data_nonconst);
		cout<<"Setting "<<option<<" = "<<value<<endl;
		if(p_device_)
		{
			p_device_->setSampleRate(SOAPY_SDR_RX, 0, value);
			sampling_rate_ = p_device_->getSampleRate(SOAPY_SDR_RX, 0);
			cout<<"Set "<<option<<" = "<<sampling_rate_<<endl;
		}
		return true;
	}
	else if(option == "frequency_double")
	{
		double value = *reinterpret_cast<double*>(p_data_nonconst);
		if(p_device_)
		{
			if(b_verbose)
				cout<<"Set "<<option<<" = "<<setprecision(12)<<value<<endl;
			p_device_->setFrequency( SOAPY_SDR_RX, 0, value );
		}
		return true;
	}
	else if(option == "ppm_double")
	{
		double value = *reinterpret_cast<double*>(p_data_nonconst);
		if( p_device_ && p_device_->hasFrequencyCorrection(SOAPY_SDR_RX, 0) )
		{
			if(b_verbose)
				cout<<"Set "<<option<<" = "<<setprecision(1)<<value<<endl;
			p_device_->setFrequencyCorrection( SOAPY_SDR_RX, 0, value );
		}
		return true;
	}
	else if(option == "gain_double")
	{
		double value = *reinterpret_cast<double*>(p_data_nonconst);
		if(p_device_)
		{
			p_device_->setGain( SOAPY_SDR_RX, 0, value );
			if(b_verbose)
				cout<<"Set "<<option<<" = "<<value<<endl;
		}
		return true;
	}
	else if(option == "biastee_double")
	{
		double value = *reinterpret_cast<double*>(p_data_nonconst);
		if(p_device_)
		{
			if(b_verbose)
				cout<<"Set "<<option<<" = "<<setprecision(12)<<value<<endl;
			if(value)
				p_device_->writeSetting("biastee", "true");
			else
				p_device_->writeSetting("biastee", "false");
		}
		return true;
	}
	else if(option == "usb_pack_double")
	{
		double value = *reinterpret_cast<double*>(p_data_nonconst);
		if(p_device_)
		{
			if(b_verbose)
				cout<<"Set "<<option<<" = "<<setprecision(12)<<value<<endl;
			if(value)
				p_device_->writeSetting("bitpack", "true");
			else
				p_device_->writeSetting("bitpack", "false");
		}
		return true;
	}
	else
	{
		cout<<"IQSource_SoapySDR::setOption error. Unknown option: "<<option<<endl;
		return false;
	}

	return false;
}


bool IQSource_SoapySDR::getOption(const std::string& option, void* p_data)
{
	using namespace std;

	bool b_verbose = false;

	if(!p_device_)
	{
		cout<<"IQSource_SoapySDR::setOption no device initialized."<<endl;
	}

	if(option == "sampling_rate_double")
	{
		*reinterpret_cast<double*>(p_data) = sampling_rate_;
		if(b_verbose)
			cout<<"getOption "<<option<<" "<<*reinterpret_cast<double*>(p_data)<<endl;
		return true;
	}
	else if(option == "frequency_double")
	{
		if(p_device_)
		{
			*reinterpret_cast<double*>(p_data) = p_device_->getFrequency( SOAPY_SDR_RX, 0);
			if(b_verbose)
				cout<<"getOption "<<option<<" "<<*reinterpret_cast<double*>(p_data)<<endl;
			return true;
		}
		return false;
	}
	else if(option == "ppm_double")
	{
		if( p_device_ && p_device_->hasFrequencyCorrection(SOAPY_SDR_RX, 0) )
		{
			*reinterpret_cast<double*>(p_data) = p_device_->getFrequencyCorrection( SOAPY_SDR_RX, 0);
			if(b_verbose)
				cout<<"getOption "<<option<<" "<<*reinterpret_cast<double*>(p_data)<<endl;
			return true;
		}
		return false;
	}
	else if(option == "gain_double")
	{
		if(p_device_)
		{
			*reinterpret_cast<double*>(p_data) = p_device_->getGain( SOAPY_SDR_RX, 0 );
			if(b_verbose)
				cout<<"getOption "<<option<<" "<<*reinterpret_cast<double*>(p_data)<<endl;
			return true;
		}
		return false;
	}
	else if(option == "biastee_double")
	{
		if(p_device_)
		{
			double biastee_val =
						p_device_->readSetting("biastee") == string("true");
			*reinterpret_cast<double*>(p_data) = biastee_val;
			if(b_verbose)
				cout<<"getOption "<<option<<" "<<biastee_val<<endl;
			return true;
		}
		return false;
	}
	else if(option == "usb_pack_double")
	{
		if(p_device_)
		{
			double biastee_val =
						p_device_->readSetting("bitpack") == string("true");
			*reinterpret_cast<double*>(p_data) = biastee_val;
			if(b_verbose)
				cout<<"getOption "<<option<<" "<<biastee_val<<endl;
			return true;
		}
		return false;
	}
	else
	{
		cout<<"IQSource_SoapySDR::getOption error. Unknown option: "<<option<<endl;
		return false;
	}

	return false;
}



} // namespace habdec