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

#include <iostream>
#include <cstdint>

#include "Decoder/SpectrumInfo.h"
#include "CompressedVector.h"

struct SpectrumInfoHeader
{
	int32_t header_size_ = (int32_t)sizeof(SpectrumInfoHeader);
	float noise_floor_ = 0;
	float noise_variance_ = 0;
	float sampling_rate_ = 0;
	float shift_ = 0;
	int32_t peak_left_ = 0;
	int32_t peak_right_ = 0;
	int32_t peak_left_valid_ = 0;
	int32_t peak_right_valid_ = 0;
	float min_ = 0;
	float max_ = 0;
	// int32_t normalized_ = 0;

	int32_t type_size_ = 0;
	int32_t size_ = 0;
};


struct DemodHeader
{
	int32_t header_size_ = sizeof(DemodHeader);
	float min_ = 0;
	float max_ = 0;
	int32_t type_size_ = 0;
	int32_t size_ = 0;
};



template<typename TSpectrumInfo, typename TTransport>
void SerializeSpectrum(const TSpectrumInfo& spectrum_info, std::stringstream& ostr, TTransport* unused)
{
	// convert spectrum_info values to type TTransport
	habdec::CompressedVector<TTransport>	compressed(spectrum_info);

	// make header
	SpectrumInfoHeader header;
	header.noise_floor_ = spectrum_info.noise_floor_;
	header.noise_variance_ = spectrum_info.noise_variance_;
	header.sampling_rate_ = spectrum_info.sampling_rate_;
	header.shift_ = spectrum_info.shift_;
	header.peak_left_ = spectrum_info.peak_left_;
	header.peak_right_ = spectrum_info.peak_right_;
	header.peak_left_valid_ = spectrum_info.peak_left_valid_;
	header.peak_right_valid_ = spectrum_info.peak_right_valid_;
	header.size_ = spectrum_info.size();

	header.min_ = compressed.min_;
	header.max_ = compressed.max_;

	header.type_size_ = sizeof(TTransport);

	ostr.write( reinterpret_cast<char*>(&header), sizeof(header) );
	ostr.write( reinterpret_cast<char*>( compressed.values_.data()), compressed.values_.size() * sizeof(TTransport) );
}


template<typename DemodVector, typename TTransport>
void SerializeDemodulation(const DemodVector& vec, std::stringstream& ostr, TTransport* unused)
{
	habdec::CompressedVector<TTransport>	compressed(vec);

	// make header
	DemodHeader header;
	header.size_ = vec.size();
	header.min_ = compressed.min_;
	header.max_ = compressed.max_;
	header.type_size_ = sizeof(TTransport);

	ostr.write( reinterpret_cast<char*>(&header), sizeof(header) );
	ostr.write( reinterpret_cast<char*>( compressed.values_.data()), compressed.values_.size() * sizeof(TTransport) );
}
