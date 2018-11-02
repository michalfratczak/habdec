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

#include <vector>
#include <iostream>

namespace habdec
{

/*
	just a vector with spectrum power values
	and some extra info
*/

template<typename T>
class SpectrumInfo : public std::vector<T>
{
public:
	typedef T TValue;

	mutable T min_ = 0;
	mutable T max_ = 0;
	mutable double noise_floor_ = 0;
	mutable double noise_variance_ = 0;
	mutable double sampling_rate_ = 0;
	mutable double shift_ = 0;
	mutable int peak_left_ = 0;
	mutable int peak_right_ = 0;
	mutable bool peak_left_valid_ = false;
	mutable bool peak_right_valid_ = false;

	SpectrumInfo() = default;

	template<typename U>
	SpectrumInfo(const SpectrumInfo<U>& rhs) :
		std::vector<T>::vector(rhs),
		noise_floor_(rhs.noise_floor_),
		noise_variance_(rhs.noise_variance_),
		peak_left_(rhs.peak_left_),
		peak_right_(rhs.peak_right_),
		peak_left_valid_(rhs.peak_left_valid_),
		peak_right_valid_(rhs.peak_right_valid_)
	{
		//
	}

	template<typename U>
	const SpectrumInfo<T>& operator=(const SpectrumInfo<U>& rhs)
	{
		std::vector<T>::operator=(rhs);
		noise_floor_ = rhs.noise_floor_;
		noise_variance_ = rhs.noise_variance_;
		peak_left_ = rhs.peak_left_;
		peak_right_ = rhs.peak_right_;
		peak_left_valid_ = rhs.peak_left_valid_;
		peak_right_valid_ = rhs.peak_right_valid_;
		return *this;
	}

	const SpectrumInfo<T>& operator=(const std::vector<T>& rhs)
	{
		std::vector<T>::operator=(rhs);
		return *this;
	}

};



} // namespace habdec