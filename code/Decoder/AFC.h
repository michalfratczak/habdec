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
#include <complex>
#include <numeric>
#include <iostream>

#include "IQVector.h"
#include "Average.h"

namespace
{

template<typename TIQVector, typename TRVector>
bool FftPower(const TIQVector& fft_samples, TRVector& o_power_arr);

template<typename VEC>
double ComputeVariance(const VEC& i_containter, const double mean);

template<typename TRVector>
void FindPeaks(	const TRVector& v,
				const double expected_relative_separation,
				int* p_peak1, int* p_peak2 );

}

namespace habdec
{

template<typename TReal>
class AFC
{
public:
	typedef 	std::complex<TReal>			TComplex;
	typedef 	std::vector<TReal>			TRVector;
	typedef 	habdec::IQVector<TReal>		TIQVector;

	AFC() = default;

	void		setFftSamples(const TIQVector& i_fft_samples) { fft_samples_ = i_fft_samples; }
	double		process();
	double		operator()() { return process(); }

	const TRVector& getPowerSpectrum() const { return power_arr_; }
	double		getFrequencyCorrection() const { return frequency_correction_; }
	void		resetFrequencyCorrection(const double frequency_correction);
	double		getShift() const { return shift_Hz_; }
	void		getPeaks(int& pl, int& pr) const { pl = gui_peak_left_; pr = gui_peak_right_; }
	void		getNoiseFloor(double& nf, double& nv) const { nf = noise_floor_; nv = noise_variance_; }

private:
	TIQVector 	fft_samples_; // input FFT samples
	TRVector 	power_arr_;

	double frequency_correction_ = 0;

	double noise_floor_ = 0;
	double noise_variance_ = 0;
	habdec::Average<double> noise_floor_avg_ {100};
	habdec::Average<double> noise_variance_avg_ {100};

	habdec::Average<int> peak_left_avg_ {4};
	habdec::Average<int> peak_right_avg_ {4};
	double 	shift_Hz_ = 0; // peak separation in Hz

	int gui_peak_left_ = 0;
	int gui_peak_right_ = 0;
};


template<typename TReal>
double AFC<TReal>::process()
{
	using namespace std;

	if( !FftPower(fft_samples_, power_arr_) )
	{
		frequency_correction_ = 0;
		return 0;
	}

	// noise floor
	noise_floor_ = double(accumulate(power_arr_.begin(), power_arr_.end(), 0.0)) / power_arr_.size();
	noise_variance_ = ComputeVariance(power_arr_, noise_floor_);
	noise_floor_avg_.add( noise_floor_ );
	noise_variance_avg_.add( noise_variance_ );

	// expected separation between peaks
	// const float fsk_shift = ( 0 < shift_Hz_ && shift_Hz_ < 2000 ) ? shift_Hz_ : 500; //Hz
	const float fsk_shift = 500;
	float expected_relative_separation = fsk_shift/fft_samples_.samplingRate(); // 500 Hz in 39kHz span

	// two maximum peaks
	int p1, p2;
	FindPeaks(power_arr_, expected_relative_separation, &p1, &p2);

	// it cannot be resolved which is left/right
	// unless both are present (stronger than peak_detect_threshold)
	const float peak_detect_threshold = noise_floor_avg_ + 3 * fabs(noise_variance_avg_);

	bool b_p1_detected = false;
	if( power_arr_[p1] > peak_detect_threshold ) // peak detected
		b_p1_detected = true;

	bool b_p2_detected = false;
	if( power_arr_[p2] > peak_detect_threshold ) // peak detected
		b_p2_detected = true;

	// peaks position stability
	const int peak_stability_threshold = 2; // expressed in bins
	bool b_peak_left_stable = false;
	bool b_peak_right_stable = false;
	if(b_p1_detected && b_p2_detected)
	{
		if(p2<p1)
			swap(p1, p2);

		if( peak_left_avg_.add(p1) <= peak_stability_threshold ) // peak stable
			b_peak_left_stable = true;
		if( peak_right_avg_.add(p2) <= peak_stability_threshold ) // peak stable
			b_peak_right_stable = true;
	}

	// peaks gui display
	// unstable peaks are drawn with dimmer color (nagative sign)
	gui_peak_left_ = 0;
	if(b_p1_detected)
	{
		if(b_peak_left_stable)
			gui_peak_left_ = peak_left_avg_;
		else
			gui_peak_left_ = - peak_left_avg_; // not valid
	}

	gui_peak_right_ = 0;
	if(b_p2_detected)
	{
		if(b_peak_right_stable)
			gui_peak_right_ = peak_right_avg_;
		else
			gui_peak_right_ = - peak_right_avg_; // not valid
	}

	// correct frequency only if both peaks stable
	if(b_peak_left_stable && b_peak_right_stable)
	{
		const int peak_left =  round( (double) peak_left_avg_ );
		const int peak_right = round( (double) peak_right_avg_ );
		const int bins_dist = peak_right - peak_left;

		const double Hertz_per_bin = fft_samples_.samplingRate() / fft_samples_.size();
		shift_Hz_ = Hertz_per_bin * bins_dist;

		const double middle_peak = peak_left + bins_dist / 2;
		const double bins_error = middle_peak - double(fft_samples_.size()) / 2;
		const double freq_error = Hertz_per_bin * bins_error;

		const int bin_dist_threshold = 4;
		if( bin_dist_threshold < abs(bins_error) )
			frequency_correction_ = freq_error;
	}

	return frequency_correction_;
}


template<typename TReal>
void AFC<TReal>::resetFrequencyCorrection(const double frequency_correction)
{
	const double bins_per_Hertz = (double)fft_samples_.size() / fft_samples_.samplingRate();
	peak_left_avg_.reset( std::max((double) 0, peak_left_avg_  - frequency_correction * bins_per_Hertz) );
	peak_right_avg_.reset(std::max((double) 0, peak_right_avg_ - frequency_correction * bins_per_Hertz) );
	frequency_correction_ = 0;
}


} // namespace habdec


namespace
{


template<typename TReal>
bool hasNan(const TReal* v, size_t sz)
{
	for(size_t i=0; i<sz; ++i)
		if(v[i] != v[i])
			return true;
	return false;
}


template<typename TReal>
bool hasInf(const TReal* v, size_t sz)
{
	for(size_t i=0; i<sz; ++i)
		if(std::isinf(v[i]))
			return true;
	return false;
}


template<typename VEC>
double ComputeVariance(const VEC& i_containter, const double mean)
{
	double var = 0;
	for(const auto& v : i_containter )
	  var += (v - mean) * (v - mean);
	var /= i_containter.size();
	return sqrt(var);
}


template<typename TIQVector, typename TRVector>
bool FftPower(const TIQVector& fft_samples, TRVector& o_power_arr)
{
	// http://nl.mathworks.com/help/signal/ug/power-spectral-density-estimates-using-fft.html

	using namespace std;

	if(!fft_samples.size())
		return 0;

	if(!fft_samples.samplingRate())
		return 0;

	auto* p_fft_samples_data = const_cast<typename TIQVector::TComplex*>(fft_samples.data());

	if( hasNan(reinterpret_cast<typename TIQVector::TValue*>(p_fft_samples_data), fft_samples.size()*2) )
	{
		cout<<"FftSpectrum Nan"<<endl;
		return 0;
	}

	if( hasInf(reinterpret_cast<typename TIQVector::TValue*>(p_fft_samples_data), fft_samples.size()*2) )
	{
		cout<<"FftSpectrum Inf"<<endl;
		return 0;
	}


	o_power_arr.resize(fft_samples.size());

	for(size_t i=0; i<fft_samples.size(); ++i)
	{
		o_power_arr[i] = norm(fft_samples[i]) / fft_samples.size(); // normalize by N
		o_power_arr[i] = o_power_arr[i] * o_power_arr[i];
		o_power_arr[i] /= fft_samples.samplingRate();
		o_power_arr[i] = 10.0f * log10( o_power_arr[i] );
	}

	if( hasNan(o_power_arr.data(), o_power_arr.size()) )
	{
		cout<<"Power Nan"<<endl;
		return 0;
	}

	if( hasInf(o_power_arr.data(), o_power_arr.size()) )
	{
		cout<<"Power Inf"<<endl;
		return 0;
	}

	return 1;
}


template<typename TRVector>
void FindPeaks(	const TRVector& v,
				const double expected_relative_separation,
				int* p_peak1, int* p_peak2 )
{
	using namespace std;

	// FIND TWO MAXIMUMS SEPARATED AT LEAST BY BINS_SEP
	//

	int bins_sep = round(expected_relative_separation * v.size());
	bins_sep = std::max(8, bins_sep);

	// first peak
	auto i_max = max_element(v.begin(), v.end());
	int p1 = i_max - v.begin();
	float p1_val = *i_max;

	// second peak
	int begin = std::max( p1 - 2 * bins_sep, 0 );
	int end = 	std::min( p1 + 2 * bins_sep, (int) v.size() );
	int p2 = 0;
	float p2_val = v[0];
	for(int i=begin; i<end; ++i)
	{
		if(v[i] > p2_val && abs(i-p1) > bins_sep/2)
		{
			p2 = i;
			p2_val = v[i];
		}
	}

	if(p2<p1)
	{
		swap(p1, p2);
		swap(p1_val, p2_val);
	}

	*p_peak1 = p1;
	*p_peak2 = p2;
}


} // namespace

