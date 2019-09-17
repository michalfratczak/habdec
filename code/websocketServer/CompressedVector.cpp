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


#include <vector>
#include <algorithm>
#include <iostream>

#include "CompressedVector.h"

namespace habdec
{


template<>
template<>
void CompressedVector<unsigned char>::copyValues(const std::vector<unsigned char>& rhs, double i_min, double i_max)
{
	if(!rhs.size())
	{
		values_.clear();
		return;
	}
	values_.resize(rhs.size());
	std::copy(rhs.begin(), rhs.end(), values_.begin());
}


template<>
template<>
void CompressedVector<uint16_t>::copyValues(const std::vector<uint16_t>& rhs, double i_min, double i_max)
{
	if(!rhs.size())
	{
		values_.clear();
		return;
	}
	values_.resize(rhs.size());
	std::copy(rhs.begin(), rhs.end(), values_.begin());
}


template<>
template<>
void CompressedVector<float>::copyValues(const std::vector<float>& rhs, double i_min, double i_max)
{
	if(!rhs.size())
	{
		values_.clear();
		return;
	}
	values_.resize(rhs.size());
	std::copy(rhs.begin(), rhs.end(), values_.begin());
}


// specialization:  float --> unsigned char
template<>
template<>
void CompressedVector<unsigned char>::copyValues(const std::vector<float>& rhs, double i_min, double i_max)
{
	// std::cout<<"specialization:  float --> unsigned char"<<std::endl;
	values_.clear();
	// normalized_ = true; // ALWAYS NORMALIZED FOR NON-FLOAT TYPES

	if(!rhs.size())
		return;

	values_.reserve(rhs.size());
	for(auto rhs_v : rhs)
	{
		// if(!rhs.normalized_)
			rhs_v = float(rhs_v - i_min) / (i_max - i_min);
		unsigned char v_out = rhs_v * std::numeric_limits<unsigned char>::max();
		values_.push_back(v_out);
	}
}


// specialization:  float --> uint16_t
template<>
template<>
void CompressedVector<uint16_t>::copyValues(const std::vector<float>& rhs, double i_min, double i_max)
{
	// std::cout<<"specialization:  float --> uint16_t"<<std::endl;
	values_.clear();
	// normalized_ = true; // ALWAYS NORMALIZED FOR NON-FLOAT TYPES

	if(!rhs.size())
		return;

	values_.reserve(rhs.size());
	for(auto rhs_v : rhs)
	{
		// if(!rhs.normalized_)
		rhs_v = float(rhs_v - i_min) / (i_max - i_min);
		uint16_t v_out = rhs_v * std::numeric_limits<uint16_t>::max();
		values_.push_back(v_out);
	}
}



// specialization:  unsigned char --> float
template<>
template<>
void CompressedVector<float>::copyValues(const std::vector<unsigned char>& rhs, double i_min, double i_max)
{
	// std::cout<<"specialization:  unsigned char --> float"<<std::endl;
	values_.clear();
	// normalized_ = true; // ALWAYS NORMALIZED FOR NON-FLOAT TYPES

	if(!rhs.size())
		return;

	values_.reserve(rhs.size());
	for(auto rhs_v : rhs)
	{
		float rhs_v_0_1 = float(rhs_v) / std::numeric_limits<unsigned char>::max();
		values_.push_back(rhs_v_0_1);
	}
}


// specialization:  uint16_t --> float
template<>
template<>
void CompressedVector<float>::copyValues(const std::vector<uint16_t>& rhs, double i_min, double i_max)
{
	// std::cout<<"specialization:  uint16_t --> float"<<std::endl;
	values_.clear();
	// normalized_ = true; // ALWAYS NORMALIZED FOR NON-FLOAT TYPES

	if(!rhs.size())
		return;

	values_.reserve(rhs.size());
	for(auto rhs_v : rhs)
	{
		float rhs_v_0_1 = float(rhs_v) / std::numeric_limits<uint16_t>::max();
		values_.push_back(rhs_v_0_1);
	}
}


} // namespace habdec