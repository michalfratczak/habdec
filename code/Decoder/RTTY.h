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

#include "stddef.h"
#include <vector>
#include <iostream>
#include <cctype>

#include "common/console_colors.h"

namespace habdec
{

template<typename TBit>
class RTTY
{

public:
	void 	ascii_bits(size_t i_bits) {nbits_ = i_bits;};
	size_t 	ascii_bits()  const {return nbits_;};
	void 	ascii_stops(float i_stops) {nstops_ = i_stops;};
	float 	ascii_stops()  const {return nstops_;};

	void push(const std::vector<TBit>& i_bits);
	size_t size()	{return chars_.size();}
	std::vector<char>	get();
	size_t operator()();

private:
	size_t nbits_ = 0;
	float nstops_ = 0;

	std::vector<TBit>	bits_;
	std::vector<char>	chars_;
};



template<typename TBit>
void RTTY<TBit>::push(const std::vector<TBit>& i_bits)
{
	using namespace std;
	bits_.insert( bits_.end(), i_bits.begin(), i_bits.end() );
}


template<typename TBit>
std::vector<char>	RTTY<TBit>::get()
{
	auto res = chars_;
	chars_.clear();
	return res;
}



template<typename TBit>
size_t RTTY<TBit>::operator()()
{
	if(!nbits_ && !nstops_)
		return 0;

	if( bits_.size() < (1+nbits_+nstops_) )
		return 0;

	using namespace std;

	size_t n_decoded_chars = 0;
	size_t last_decoded_bit_index = 0;

	for(size_t i=0; i<bits_.size()/*-char_bitlen*/; /**/)
	{

		bool is_char_seq =
			bits_[i] == 0; // start bit?

		// at least 1+8+2 bits available ?
		is_char_seq &= (i + 1+nbits_+nstops_) <= bits_.size();

		// has stop bits ?
		for(size_t s=0; s<nstops_; ++s)
			is_char_seq &= bits_[i+1+nbits_ + s] == 1;

		if( !is_char_seq )
		{
			++i;
			continue;
		}

		// this has to be char sequence
		{
			++i;

			// bit to char
			char c = 0;
			for(size_t _c=0; _c<nbits_; ++_c)
			{
				c += bits_[i] << _c;
				++i;
			}

			// if( isprint(c) || c == '\n' )
			{
				chars_.push_back(c);
				++n_decoded_chars;
			}

			i += nstops_;
			last_decoded_bit_index = i-1; // points to last stop bit
		}

	}

	if(last_decoded_bit_index)
		bits_.erase( bits_.begin(), bits_.begin() + last_decoded_bit_index + 1 );

	return n_decoded_chars;
}



} // namespace habdec