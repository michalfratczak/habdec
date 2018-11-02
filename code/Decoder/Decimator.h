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
#include <vector>
#include <complex>
#include <memory>
#include <iostream>

namespace habdec
{

template<typename Ts, typename Tt>
class Decimator
{
public:
	Decimator() {};
	Decimator(int decimation_factor, const Tt* p_taps, const size_t taps_size);

	void setInput(const Ts* p_in, const size_t in_size);
	void setOutput(Ts* p_out);
	void setFilter(const Tt* p_taps, const size_t taps_size);

	size_t operator()();

	void setFactor(const int i_f) {decimation_factor_ = i_f;}
	int factor() 	const {return decimation_factor_;}

private:
	int	decimation_factor_ = 0;

	const Ts*	p_in_ = 0;
	size_t		p_in_size_ = 0;
	Ts*			p_out_ = 0; //size is p_in_size_/decimation_factor_
	std::vector<Ts>		p_buff_;
	std::vector<Tt>		p_taps_;
	size_t	decimated_samples_count_ = 0;
};


template<typename Ts, typename Tt>
Decimator<Ts,Tt>::Decimator(int decimation_factor, const Tt* p_taps, const size_t taps_size)
{
	setFactor(decimation_factor);
	setFilter(p_taps, taps_size);
}


template<typename Ts, typename Tt>
void Decimator<Ts,Tt>::setInput(const Ts* p_in, const size_t in_size)
{
	p_in_ = p_in;
	p_in_size_ = in_size;

	const size_t new_buff_size = p_in_size_ + p_taps_.size() + decimation_factor_;
	if(p_buff_.size() < new_buff_size)
	{
		p_buff_.resize(new_buff_size);
		memset( p_buff_.data(), 0, sizeof(Ts) * p_taps_.size() );
	}
}


template<typename Ts, typename Tt>
void Decimator<Ts,Tt>::setFilter(const Tt *p_taps, const size_t taps_size)
{
	p_taps_.resize(taps_size);
	memcpy( (void*) p_taps_.data(), p_taps, taps_size * sizeof(Tt));
}


template<typename Ts, typename Tt>
void Decimator<Ts,Tt>::setOutput(Ts* p_out)
{
	p_out_ = p_out;
}


template<typename Ts, typename Tt>
size_t Decimator<Ts,Tt>::operator()()
{
	if(!decimation_factor_ || !p_in_ || !p_out_ || !p_buff_.size())
	{
		std::cout<<"Decimator not fully initialised"<<std::endl;
		return 0;
	}

	decimated_samples_count_ = 0;

	if(!p_buff_.size())
	{
		std::cout<<"p_buff_ = 0"<<std::endl;
		return 0;
	}

	/*if( !(p_taps_.size()%2) )
	{
		std::cout<<"taps count not odd "<<p_taps_.size()<<std::endl;
		return 0;
	}*/

	// copy samples to buffer, offseted by taps_count-1
	memcpy( p_buff_.data()+p_taps_.size()-1,
			p_in_,
			p_in_size_ * sizeof(Ts) );


	// apply filter
	for(	size_t in = 0, out=0;
			in < p_in_size_ && out < p_in_size_ / decimation_factor_;
			in += decimation_factor_, out += 1 // downsampling
		)
	{
		Ts accumulator = 0;
		for(size_t t=0; t < p_taps_.size(); ++t)
			accumulator += p_buff_[in+t] * p_taps_[t];
		p_out_[out] = accumulator;
		++decimated_samples_count_;
	}

	// wrapping. copy last part (p_taps_.size()-1) of (unfiltered) input signal to start of buffer
	memcpy(	p_buff_.data(),
			p_in_ + p_in_size_-p_taps_.size()+1,
			(p_taps_.size()-1) * sizeof(Ts)	);

	return decimated_samples_count_;
}


} // namespace habdec
