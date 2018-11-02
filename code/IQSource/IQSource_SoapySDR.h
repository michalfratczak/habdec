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

#include "IQSource.h"

#include <memory>
#include <vector>

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>


namespace habdec
{


class IQSource_SoapySDR : public habdec::IQSource
{
public:
	~IQSource_SoapySDR();

	bool start();
	bool stop();
	bool isRunning() const;
	size_t count() const;
	size_t get(void* p_data, const size_t i_count);
	std::string type() const {return std::string("SoapySDR");};
	double samplingRate() const;
	bool setOption(const std::string& option, const void* p_data);
	bool getOption(const std::string& option, void* p_data);

	bool init();

private:
	std::unique_ptr<SoapySDR::Device>	p_device_ = 0;
	SoapySDR::Stream*	p_stream_ = 0;

	bool b_is_running_ = false;
	double sampling_rate_ = 0;

	// SoapySDR hardware info
	SoapySDR::Kwargs	soapysdr_kwargs_; // used for init()
	std::string 		hwinfo_hwkey_;
	std::string 		hwinfo_serial_;
	size_t				hwinfo_device_id_;

	std::vector<std::string> 	hwinfo_gains_;
	std::vector<double> 		hwinfo_sampling_rates_;
	std::vector<std::string> 	hwinfo_stream_formats_;

	double hwinfo_freq_min_ = 0;
	double hwinfo_freq_max_ = 0;
	double hwinfo_gain_min_ = 0;
	double hwinfo_gain_max_ = 0;
};


} // namespace habdec