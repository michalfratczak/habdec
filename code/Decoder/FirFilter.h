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

#include <cstring>
#include <memory>
#include <vector>
#include <iostream>

#include "habdec_windows.h"


namespace habdec
{


template<typename Ts, typename Tt> // Tsamples, Ttaps
class FirFilter
{
public:
	bool setInput(const Ts* p_input, const size_t in_size);
	bool setOutput(Ts*);
	bool setFilter(const Tt*, const size_t taps_size);
	size_t taps_size();
	bool operator()();

	void LP_BlackmanHarris(const float i_relative_filt_width, const float trans_bw = 0);

private:
	const Ts* p_input_ = 0;
	size_t p_input_size_ = 0;

	Ts*	p_output_ = 0;

	std::vector<Tt>	taps_;

	std::vector<Ts>	buff_;

	void dotProduct();
};




template<typename Ts, typename Tt>
bool inline FirFilter<Ts, Tt>::operator()()
{
	dotProduct();
	return true;
}


template<typename Ts, typename Tt>
size_t inline FirFilter<Ts, Tt>::taps_size()
{
	return taps_.size()	;
}


template<typename Ts, typename Tt>
bool inline FirFilter<Ts, Tt>::setInput(const Ts* p_input, const size_t in_size)
{
	if(!p_input || !in_size)
		return false;

	p_input_ = p_input;
	p_input_size_ = in_size;

	return true;
}


template<typename Ts, typename Tt>
bool inline FirFilter<Ts, Tt>::setFilter(const Tt* p_filter, const size_t in_size)
{
	if(!p_filter || !in_size)
		return false;

	taps_.resize(in_size);
	memcpy( taps_.data(), p_filter, in_size * sizeof(Tt) );

	return true;
}


template<typename Ts, typename Tt>
bool inline FirFilter<Ts, Tt>::setOutput(Ts* p_output)
{
	if(!p_output)
		return false;

	p_output_ = p_output;

	return true;
}


template<typename Ts, typename Tt>
void FirFilter<Ts, Tt>::dotProduct()
{
	using namespace std;

	if(!taps_.size())
	{
		cout<<"FirFilter::dotProduct() no taps. return."<<endl;
		return;
	}

	if( taps_.size() > p_input_size_+1 )
	{
		cout<<"FirFilter::dotProduct() more taps than samples. return. "<<p_input_size_<<" "<<taps_.size()<<endl;
		return;
	}

	if(p_input_ == p_output_)
	{
		cout<<"FirFilter::dotProduct() ERROR.  in==out"<<endl;
		return;
	}


	// reinit buffer if size changed
	const size_t new_buff_size = p_input_size_ + taps_.size();
	if(buff_.size() < new_buff_size)
	{
		// cout<<"FirFilter resize buff_ "<<p_input_size_<<" + "<<taps_.size()<<endl;
		buff_.resize(new_buff_size);
		memset( buff_.data(), 0, sizeof(Ts) * taps_.size() );
	}

	// copy samples to buffer, offseted by taps_count-1
	memcpy( buff_.data() + taps_.size()-1,
			p_input_,
			p_input_size_ * sizeof(Ts) );

	// inner product
	for(size_t i=0; i<p_input_size_; ++i)
	{
		Ts accumulator = 0;
		for(size_t t=0; t<taps_.size(); ++t)
			accumulator += buff_[i+t] * taps_[t];
		p_output_[i] = accumulator;
	}

	// wrapping. copy last part (taps_.size()-1) of (unfiltered) input signal to start of buffer
	memcpy(	buff_.data(),
			p_input_ + p_input_size_ - (taps_.size() - 1),
			// p_input_ + p_input_size_ - min(taps_.size()-1, p_input_size_),
			(taps_.size()-1) * sizeof(Ts) );

}


template<typename Ts, typename Tt>
void FirFilter<Ts, Tt>::LP_BlackmanHarris(const float i_relative_filt_width, const float trans_bw)
{
	if(!p_input_size_)
	{
		std::cout<<"FirFilter::LP_BlackmanHarris No Input set."<<std::endl;
		return;
	}

	const float f_cut_rel = i_relative_filt_width;
	const float _transition_bw = trans_bw ?
				trans_bw : i_relative_filt_width*i_relative_filt_width;

	size_t taps_sz = 4.0f/_transition_bw;
	if(taps_sz>p_input_size_)
		taps_sz = p_input_size_;
	taps_sz |= 1; // must be odd number

	if(taps_sz <= 4)
		return;

	if(taps_sz == taps_.size())
			return;

	taps_.resize(taps_sz);

	double _sum = 0;
	const int _mid = int(taps_sz/2); // !! INT IS VERY IMPORTANT HERE !!
	for(int i=0; i<int(taps_sz); i++)
	{
		taps_[i] = sinc(2.0f*f_cut_rel*(i-_mid)) * BlackmanHarris<Tt>(size_t(i), taps_sz);
		_sum += taps_[i];
	}

	// normalize taps_
	for(size_t i=0; i<taps_sz; ++i)
		taps_[i] /= _sum;
}


} // namespace habdec
