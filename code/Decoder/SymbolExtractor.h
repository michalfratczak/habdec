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

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>
#include <numeric>

namespace
{

template <typename T>
inline int sgn(T val)
{
	return (T(0) < val) - (val < T(0));
}


template <typename T>
inline int _dir(T left, T right)
{
	if( sgn(left) < sgn(right) )
		return 1;
	else if( sgn(left) > sgn(right) )
		return -1;
	else
		return 0;
}


template<typename TReal, typename TRVector>
void FlipPointAvrg(
	const TRVector& v,
	const size_t i,
	const size_t R,
	TReal& o_avg_l,
	TReal& o_avg_r )
{
	using namespace std;	int _L = max(int(i-R), 0); 	 // left bound
	int _R = min(i+R, v.size()); // right bound
	o_avg_l = std::accumulate(v.begin()+_L, v.begin()+i,  TReal(0)) / (i-_L);
	o_avg_r = std::accumulate(v.begin()+i,  v.begin()+_R, TReal(0)) / (_R-i);
}


} // namespace



namespace habdec
{

template<typename TReal>
class SymbolExtractor
{

public:
	typedef std::vector<TReal> 	TRVector;

	void pushSamples(const TRVector& v); //REAL samples after Freq demodulation

	void samplingRate(const double sm_r) 	{ sampling_rate_ = sm_r; }
	double samplingRate() const 			{ return sampling_rate_; }

	void symbolRate(const double sb_r) 	{ symbol_rate_ = sb_r; } // baud
	double symbolRate() const 			{ return symbol_rate_; }

	std::vector<bool> 	get(size_t count=0);

	size_t samplesPerBit() const { return size_t(round( samplingRate()/symbolRate() )); }

	void operator()();

private:
	double sampling_rate_ = 0;
	double symbol_rate_ = 1;
	TRVector 			samples_;
	std::vector<bool> 	bits_;

	size_t findFirstFlipPoint(const size_t start_offset);
	std::vector<size_t> findFlipPoints();
};




template<typename TReal>
void SymbolExtractor<TReal>::pushSamples(const TRVector& v)
{
	using namespace std;

	if(!v.size())
		return;

	// safety vent
	if(samples_.size() > 3e4)
	{
		// cout<<"SymbolExtractor::pushSamples overflow. Delete samples."<<endl;
		samples_.clear();
	}

	const size_t prev_size = samples_.size();
	samples_.resize( samples_.size() + v.size() );
	memcpy( samples_.data() + prev_size, v.data(), v.size() * sizeof(TReal) );
}


template<typename TReal>
void SymbolExtractor<TReal>::operator()()
{
	if( !sampling_rate_ || !symbol_rate_ )
		return;

	if( samples_.size() <  samplingRate()/symbolRate()*3 ) // have at least 3 symbols
		return;

	std::vector<size_t> flip_points = findFlipPoints();
	if(!flip_points.size())
		return;

	size_t last_flip_point = 0;
	for(const auto flip_point : flip_points)
	{
		TReal avg = TReal(
				std::accumulate(	samples_.begin()+last_flip_point,
							samples_.begin()+flip_point, (TReal)0 )
			) / (flip_point-last_flip_point);

		bool bit_value = avg > 0;
		size_t bit_count = size_t( round( float(flip_point-last_flip_point) / float(samplesPerBit()) ) );
		last_flip_point = flip_point;
		while(bit_count--)
			bits_.push_back(bit_value);
	}

	size_t erase_to = std::min(last_flip_point, samples_.size());
	samples_.erase( samples_.begin(), samples_.begin() + erase_to );
}


template<typename TReal>
size_t SymbolExtractor<TReal>::findFirstFlipPoint(const size_t start_offset)
{
	if( (samples_.size()-start_offset) < samplesPerBit() )
		return 0;

	// FIND FIRST FLIP POINT
	// IT NEEDS TO HAVE DIFFERENT SIGN OF AVERAGE
	// ON IT'S LEFT AND RIGHT SIDE OF LENGTH R
	const size_t R = std::max(4, int(samplesPerBit() / 4));

	std::vector<size_t> flip_points;

	// start at R
	size_t flip_point = start_offset + R;

	TReal avg_left;
	TReal avg_right;
	FlipPointAvrg(samples_, flip_point, R, avg_left, avg_right);

	// SCAN UNTIL FIRST FLIP_POINT
	while(sgn(avg_left) == sgn(avg_right))
	{
		flip_point += 1;

		if(flip_point >= (samples_.size() - samplesPerBit()))
			return 0;

		FlipPointAvrg(samples_, flip_point, R, avg_left, avg_right);
	}

	size_t flip_point_left = flip_point;
	// int value_change_dir = _dir(avg_left, avg_right);

	// SCAN UNTIL LAST FLIP_POINT WITH CHANGING SIGN OF AVERAGE
	while(sgn(avg_left) != sgn(avg_right))
	{
		flip_point += 1;

		if(flip_point >= (samples_.size() - samplesPerBit()))
			return 0;

		FlipPointAvrg(samples_, flip_point, R, avg_left, avg_right);
	}

	size_t flip_point_right = flip_point;

	// in range [flip_point_left, flip_point_right)
	// find point with largest difference in average values

	std::vector<size_t> flip_point_candidates;
	std::vector<float>  flip_point_candidates_weight;
	for(size_t i=flip_point_left; i<flip_point_right; ++i)
	{
		FlipPointAvrg(samples_, i, R, avg_left, avg_right);
		flip_point_candidates.push_back(i);
		flip_point_candidates_weight.push_back( abs(avg_right-avg_left) );
	}

	auto p_max = max_element(flip_point_candidates_weight.begin(), flip_point_candidates_weight.end());
	size_t max_index = p_max - flip_point_candidates_weight.begin();

	return flip_point_candidates[max_index];
}


template<typename TReal>
std::vector<size_t> SymbolExtractor<TReal>::findFlipPoints()
{
	size_t offset = 0;
	size_t flip_point = findFirstFlipPoint(offset);
	std::vector<size_t> all_points;

	while(flip_point)
	{
		all_points.push_back(flip_point);
		offset = flip_point;
		flip_point = findFirstFlipPoint(offset);
	}
	return all_points;
}


template<typename TReal>
std::vector<bool> SymbolExtractor<TReal>::get(size_t count)
{
	if(count == 0)
		count = bits_.size();
	const size_t _count = std::min(count, bits_.size());

	const std::vector<bool>	res( bits_.begin(), bits_.begin() + _count );
	bits_.erase( bits_.begin(), bits_.begin() + _count );

	return res;
}



} // namespace habdec


