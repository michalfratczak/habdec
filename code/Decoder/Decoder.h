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

#include <string>
#include <iostream>
#include <vector>
#include <complex>
#include <chrono>
#include <mutex>
#include <functional>

#include "IQVector.h"
#include "Decimator.h"
#include "FFT.h"
#include "FirFilter.h"
#include "filtercoef.h"
#include "FSK2_Demod.h"
#include "SymbolExtractor.h"
#include "RTTY.h"
#include "sentence_extract.h"
#include "Decoder.h"
#include "AFC.h"
#include "SpectrumInfo.h"
#include "print_habhub_sentence.h"
#include "CRC.h"
#include "ssdv_wrapper.h"

namespace habdec
{


template<typename TReal>
class Decoder
{

	// STEPS:
	//
	// multi stage Decimation
	// FFT
	// Auto Frequency Correct
	// FIR Filtering
	// Demodulation (Polar Discriminant)
	// Symbol Extraction
	// RTTY

public:
	using TValue =		TReal;
	using TComplex =	std::complex<TReal>;
	using TRVector =	std::vector<TReal>;
	using TIQVector =	habdec::IQVector<TReal>;
	using TDecimator =	habdec::Decimator< std::complex<TReal>, TReal >;
	using TFIR =		habdec::FirFilter< std::complex<TReal>, TReal>;

	// feed decoder
	bool 	pushSamples(const TIQVector& i_stream);

	// configure options
	void 	lowpass_bw(float i_lowpass_bw);
	float 	lowpass_bw() const;
	void 	lowpass_trans(float i_lowpass_trans);
	float 	lowpass_trans() const;
	void	baud(double i_baud);
	double	baud() const;
	void	rtty_bits(size_t i_ascii_bits);
	size_t	rtty_bits() const;
	void	rtty_stops(float i_ascii_stops);
	float	rtty_stops() const;
	void 	dc_remove(bool);
	bool 	dc_remove() const;

	size_t 	setupDecimationStagesFactor(const size_t i_factor);
	size_t 	setupDecimationStagesBW(const double i_max_sampling_rate);

	// get result
	std::string 	getRTTY();
	std::string 	getLastSentence();

	// get info
	int 		getDecimationFactor() 		const;
	double 		getInputSamplingRate() 		const;
	double 		getDecimatedSamplingRate() 	const;
	double 		getSymbolRate() 			const;

	// get data for GUI
	size_t 				getBinsCount() 	 	const;
	const TIQVector 	getFFT() 		 	const;
	const TRVector 		getDemodulated() 	const;
	const TRVector 		getPowerSpectrum() 	const;
	void	getPeaks(int& pl, int& pr);
	void	getNoiseFloor(double& nf, double& nv);
	double	getShift()	const;
	double	getFrequencyCorrection()	const;
	void	resetFrequencyCorrection(double frequency_correction);

	// template<typename T>
	SpectrumInfo<TReal>	getSpectrumInfo();

	// run it
	void process();
	void operator()()
	{
		typedef std::chrono::nanoseconds TDur;
		auto _start = std::chrono::high_resolution_clock::now();
			process();
		TDur _duration = std::chrono::duration_cast<TDur>(std::chrono::high_resolution_clock::now() - _start);
		// std::cout<<"process() duration "<<_duration.count()<<" nS  "<<double(10e9)/_duration.count()<<"\n";
	};

	bool livePrint() const { return live_print_; }
	void livePrint(bool i_live) { live_print_ = i_live; }

	std::string ssdvBaseFile() const { return ssdv_.base_file(); }
	void ssdvBaseFile(const std::string& _f)  { ssdv_.base_file(_f); }

	// callback on each successfull sentence decode. callsign, sentence_data, CRC
	std::function<void(std::string, std::string, std::string)> sentence_callback_;

	// callback on each decoded characters
	std::function<void(std::string)> character_callback_;

	// callback on each decoded ssdv packet. callsign, image_id, jpeg_bytes
	std::function<void(std::string, int, std::vector<uint8_t>)> ssdv_callback_;

private:
	// IQ buffers
	TIQVector			iq_in_buffer_;			// input IQ buffer
	mutable std::mutex	iq_in_buffer_mtx_;
	TIQVector			iq_samples_temp_;		// temporary processing
	TIQVector			iq_samples_decimated_;	// decimated IQ samples storage
	TIQVector			iq_samples_filtered_;	// FIR filtered IQ samples storage

	// init
	void 	init(const float i_sampling_rate); // called on first pushSamples()

	// multistage decimation
	const double max_decimated_sampling_rate_ = 40e3;
	std::vector<TDecimator>		decimation_stages_;
	int 						decimation_factor_ = 1;

	// DC removal
	bool dc_remove_ = false;

	// FFT
	const size_t 		fft_bins_cnt_ = 1<<12;
	FFT 				fft_; 		//TODO ! this is only float !
	TIQVector			freq_in_; 	// FFT of chunk size
	TIQVector			freq_out_; 	// FFT of chunk size
	mutable std::mutex	freq_out_mtx_;

	// AFC
	AFC<TReal>			afc_;
	mutable std::mutex 	afc_mtx_;

	// Low Pass FIR
	float 				lowpass_bw_Hz_ = 1500;
	float 				lowpass_trans_ = 0.025;
	TFIR				lowpass_fir_;
	mutable std::mutex	lowpass_fir_mtx_;

	// Demodulation
	TRVector			demodulated_; // result of Polar Discriminant
	mutable std::mutex	demodulated_mtx_;

	// Symbol Extraction from demodulated samples
	SymbolExtractor<TReal> 	symbol_extractor_;

	// RTTY reconstruction from bits
	RTTY<bool>		rtty_;
	std::string 	rtty_char_stream_; // result of rtty
	std::string 	last_sentence_; // result of rtty
	// size_t 		last_sentence_len_ = 0;  // optimization for regexp run

	// SSDV
	SSDV_wraper_t 	ssdv_;

	// threading
	mutable std::mutex	process_mutex_;		// mutex for main processing

	bool live_print_ = true; // live print decoded characters

	std::string 	chr_callback_stream_; // for character_callback_

};


template<typename TReal>
bool habdec::Decoder<TReal>::pushSamples(const TIQVector& i_stream)
{
	{
		std::lock_guard<std::mutex> _lock(iq_in_buffer_mtx_);
		const size_t pre_insert_size = iq_in_buffer_.size();
		iq_in_buffer_.resize( pre_insert_size + i_stream.size() );
		memcpy( iq_in_buffer_.data() + pre_insert_size, i_stream.data(), i_stream.size() * sizeof(TComplex) );
	}

	if( !iq_in_buffer_.samplingRate() )
		init( i_stream.samplingRate() );

	return true;
}


template<typename TReal>
void habdec::Decoder<TReal>::init(const float i_sampling_rate)
{
	iq_in_buffer_.samplingRate(i_sampling_rate);

	// setupDecimationStagesBW(10.1e6/256);

	std::lock_guard<std::mutex> lock(process_mutex_);

	// FFT
	freq_in_.reserve(fft_bins_cnt_);
	freq_out_.reserve(fft_bins_cnt_);
}


template<typename TReal>
void 	habdec::Decoder<TReal>::lowpass_bw(float i_lowpass_bw_Hz)
{
	std::lock_guard<std::mutex>	_lock(lowpass_fir_mtx_);
	lowpass_bw_Hz_ = i_lowpass_bw_Hz;
	lowpass_fir_.LP_BlackmanHarris(lowpass_bw_Hz_ / getDecimatedSamplingRate(), lowpass_trans_);
}

template<typename TReal>
float 	habdec::Decoder<TReal>::lowpass_bw() const
{
	return lowpass_bw_Hz_;
}

template<typename TReal>
void 	habdec::Decoder<TReal>::lowpass_trans(float i_lowpass_trans)
{
	std::lock_guard<std::mutex>	_lock(lowpass_fir_mtx_);
	lowpass_trans_ = i_lowpass_trans;
	lowpass_fir_.LP_BlackmanHarris(lowpass_bw_Hz_ / getDecimatedSamplingRate(), lowpass_trans_);
}

template<typename TReal>
float 	habdec::Decoder<TReal>::lowpass_trans() const
{
	return lowpass_trans_;
}



template<typename TReal>
size_t habdec::Decoder<TReal>::setupDecimationStagesFactor(const size_t i_factor)
{
	using namespace std;

	if( i_factor<1 || i_factor>256 )
	{
		cout<<"Unsupported decimation factor: "<<i_factor<<endl;
		return getDecimationFactor();
	}

	/*if(!iq_in_buffer_.samplingRate())
		return 0;*/

	std::lock_guard<std::mutex> lock(process_mutex_);

	decimation_stages_.clear();
	decimation_factor_ = 1;

	switch(i_factor)
	{
		case 256:
			decimation_stages_.emplace_back( TDecimator(64, &d_256_r_64_kernel[0],  d_256_r_64_len) );
			decimation_stages_.emplace_back( TDecimator(4,  &d_4_r_4_kernel[0], 	d_4_r_4_len) );
			break;
		case 128:
			decimation_stages_.emplace_back( TDecimator(32, &d_128_r_32_kernel[0],  d_128_r_32_len) );
			decimation_stages_.emplace_back( TDecimator(4,  &d_4_r_4_kernel[0], 	d_4_r_4_len) );
			break;
		case 64:
			decimation_stages_.emplace_back( TDecimator(32, &d_64_r_32_kernel[0], 	d_64_r_32_len) );
			decimation_stages_.emplace_back( TDecimator(2,  &d_2_r_2_kernel[0], 	d_2_r_2_len) );
			break;
		case 32:
			decimation_stages_.emplace_back( TDecimator(16, &d_32_r_16_kernel[0], 	d_32_r_16_len) );
			decimation_stages_.emplace_back( TDecimator(2,  &d_2_r_2_kernel[0], 	d_2_r_2_len) );
			break;
		case 16:
			decimation_stages_.emplace_back( TDecimator(8,  &d_16_r_8_kernel[0], 	d_16_r_8_len) );
			decimation_stages_.emplace_back( TDecimator(2,  &d_2_r_2_kernel[0], 	d_2_r_2_len) );
			break;
		case 8:
			decimation_stages_.emplace_back( TDecimator(8,  &d_8_r_8_kernel[0], 	d_8_r_8_len) );
			break;
		case 4:
			decimation_stages_.emplace_back( TDecimator(4,  &d_4_r_4_kernel[0], 	d_4_r_4_len) );
			break;
		case 2:
			decimation_stages_.emplace_back( TDecimator(2,  &d_2_r_2_kernel[0], 	d_2_r_2_len) );
			break;
		default:
			cout<<"Unsupported decimation factor: "<<i_factor<<endl;
			return 0;
	}

	decimation_factor_ = i_factor;

	cout<<"Decoder::setupDecimationStagesFactor Decimation Stages: ";
	for(const auto& d : decimation_stages_)
		cout<<"/"<<d.factor();
	cout<<endl;
	cout<<"Decoder::setupDecimationStagesFactor Post Decimation Sampling Rate = "<<getDecimatedSamplingRate()
		<<", decimation factor = "<<decimation_factor_<<endl;

	return decimation_factor_;
}


template<typename TReal>
size_t habdec::Decoder<TReal>::setupDecimationStagesBW(const double i_max_sampling_rate)
{
	using namespace std;

	if(!iq_in_buffer_.samplingRate())
		return 0;

	std::lock_guard<std::mutex> lock(process_mutex_);

	double result_sampling_rate = iq_in_buffer_.samplingRate();

	decimation_stages_.clear();
	decimation_factor_ = 1;

	while(result_sampling_rate > i_max_sampling_rate)
	{
		// available division ratios:
		// d_2_r_2
		// d_4_r_4
		// d_8_r_8
		// d_16_r_8
		// d_32_r_16
		// d_64_r_32
		// d_128_r_32
		// d_256_r_64

		// divide by two until threshold
		int div;
		for(div=2; div<256; div *= 2)
			if( result_sampling_rate/div <= i_max_sampling_rate )
				break;
		result_sampling_rate /= div;
		decimation_factor_ *= div;

		switch(div)
		{
			case 256:
				decimation_stages_.emplace_back( TDecimator(64, &d_256_r_64_kernel[0],  d_256_r_64_len) );
				decimation_stages_.emplace_back( TDecimator(4,  &d_4_r_4_kernel[0], 	d_4_r_4_len) );
				break;
			case 128:
				decimation_stages_.emplace_back( TDecimator(32, &d_128_r_32_kernel[0],  d_128_r_32_len) );
				decimation_stages_.emplace_back( TDecimator(4,  &d_4_r_4_kernel[0], 	d_4_r_4_len) );
				break;
			case 64:
				decimation_stages_.emplace_back( TDecimator(32, &d_64_r_32_kernel[0], 	d_64_r_32_len) );
				decimation_stages_.emplace_back( TDecimator(2,  &d_2_r_2_kernel[0], 	d_2_r_2_len) );
				break;
			case 32:
				decimation_stages_.emplace_back( TDecimator(16, &d_32_r_16_kernel[0], 	d_32_r_16_len) );
				decimation_stages_.emplace_back( TDecimator(2,  &d_2_r_2_kernel[0], 	d_2_r_2_len) );
				break;
			case 16:
				decimation_stages_.emplace_back( TDecimator(8,  &d_16_r_8_kernel[0], 	d_16_r_8_len) );
				decimation_stages_.emplace_back( TDecimator(2,  &d_2_r_2_kernel[0], 	d_2_r_2_len) );
				break;
			case 8:
				decimation_stages_.emplace_back( TDecimator(8,  &d_8_r_8_kernel[0], 	d_8_r_8_len) );
				break;
			case 4:
				decimation_stages_.emplace_back( TDecimator(4,  &d_4_r_4_kernel[0], 	d_4_r_4_len) );
				break;
			case 2:
				decimation_stages_.emplace_back( TDecimator(2,  &d_2_r_2_kernel[0], 	d_2_r_2_len) );
				break;
		}
	}

	cout<<"Decoder::setupDecimationStagesBW Decimation Stages: ";
	for(const auto& d : decimation_stages_)
		cout<<"/"<<d.factor();
	cout<<endl;
	cout<<"Decoder::setupDecimationStagesBW Post Decimation Sampling Rate = "<<result_sampling_rate
		<<", decimation factor = "<<decimation_factor_<<endl;

	return decimation_factor_;
}


template<typename TReal>
void habdec::Decoder<TReal>::process()
{
	if( !iq_in_buffer_.samplingRate() ) // uninitialized
		return;

	using namespace std;

	std::lock_guard<std::mutex> lock(process_mutex_);

	// copy samples from buffer to temp
	{
		lock_guard<mutex> _lock(iq_in_buffer_mtx_);

		if( int(iq_in_buffer_.size()) < decimation_factor_)
			return;

		const size_t samples_to_decimate_cnt = iq_in_buffer_.size() - (iq_in_buffer_.size() % decimation_factor_);
		iq_samples_temp_.resize(samples_to_decimate_cnt);
		memcpy( iq_samples_temp_.data(), iq_in_buffer_.data(), samples_to_decimate_cnt * sizeof(TComplex) );
		iq_in_buffer_.erase( iq_in_buffer_.begin(), iq_in_buffer_.begin()+samples_to_decimate_cnt );
	}

	// DECIMATE
	//
	size_t decimated_samples_size = iq_samples_temp_.size();
	for(auto& _decimator : decimation_stages_)
	{
		_decimator.setInput ( iq_samples_temp_.data(), decimated_samples_size );
		_decimator.setOutput( iq_samples_temp_.data() );
		decimated_samples_size = _decimator();
	}
	iq_samples_temp_.resize( decimated_samples_size );

	// DC removal
	if(dc_remove_)
	{
		TComplex w_prev = .97f * TComplex(iq_samples_temp_[0]);
		for(size_t i=0; i<iq_samples_temp_.size(); ++i)
		{
			TComplex w = iq_samples_temp_[i] + .97f * w_prev;
			iq_samples_temp_[i] = w - w_prev;
			w_prev = w;
		}
	}

	iq_samples_decimated_.insert( iq_samples_decimated_.end(), iq_samples_temp_.begin(), iq_samples_temp_.end() );
	iq_samples_decimated_.samplingRate( getDecimatedSamplingRate() );

	// FFT
	//
	// collect at least fft_bins_cnt_ samples from decimation
	if( freq_in_.size() < fft_bins_cnt_ && iq_samples_temp_.size() )
	{
		const size_t _to_push = min(fft_bins_cnt_-freq_in_.size(), iq_samples_temp_.size());
		freq_in_.insert (
				freq_in_.end(),
				iq_samples_temp_.begin(), iq_samples_temp_.begin()+_to_push );
	}

	freq_in_.samplingRate( getDecimatedSamplingRate() );
	freq_out_.samplingRate( getDecimatedSamplingRate() );

	// fft compute
	if(freq_in_.size() >= fft_bins_cnt_)
	{
		{
			lock_guard<mutex> _lock(freq_out_mtx_);
			freq_out_.resize(fft_bins_cnt_);
			fft_.setInput(  (float*) freq_in_.data(), fft_bins_cnt_ );
			fft_.setOutput( (float*) freq_out_.data() );
			fft_();
		}
		freq_in_.clear();
	}


	const size_t batch_size = 256;  // why does it have to be 256 ?

	if(iq_samples_decimated_.size() < batch_size)
		return;


	// AFC
	//
	// scoped_lock _lock2(freq_out_mtx_, afc_mtx_); // c++17
	if(unique_lock<mutex>(afc_mtx_, try_to_lock))
	{
		if(unique_lock<mutex>(freq_out_mtx_, try_to_lock))
		{
			if(freq_out_.size() == fft_bins_cnt_)
				afc_.setFftSamples(freq_out_);
			afc_();
		}
	}


	double frequency_correction = getFrequencyCorrection();
	double shift_Hz = afc_.getShift();
	double nf, nv;
	afc_.getNoiseFloor(nf, nv);

	// cout<<frequency_correction<<"\t"<<shift_Hz<<"\t"<<nf<<"\t"<<nv<<endl;


	// Not Decoding above certain sampling rate
	//
	if(getDecimatedSamplingRate() > 4*max_decimated_sampling_rate_)
	{
		iq_samples_temp_.clear();
		iq_samples_decimated_.clear();
		return;
	}


	// Low Pass FIR
	//
	const size_t samples_to_filter_cnt = iq_samples_decimated_.size() - iq_samples_decimated_.size() % batch_size;
	iq_samples_filtered_.resize(samples_to_filter_cnt);
	{
		lock_guard<mutex>	_lock(lowpass_fir_mtx_);
		lowpass_fir_.setInput (iq_samples_decimated_.data(), samples_to_filter_cnt);
		lowpass_fir_.setOutput(iq_samples_filtered_.data());
		lowpass_fir_.LP_BlackmanHarris(lowpass_bw_Hz_ / getDecimatedSamplingRate(), lowpass_trans_);
		lowpass_fir_();
		iq_samples_filtered_.samplingRate( getDecimatedSamplingRate() );
	}
	iq_samples_decimated_.erase( iq_samples_decimated_.begin(), iq_samples_decimated_.begin()+samples_to_filter_cnt );

	// Demod - converts frequency to amplitudes
	//
	{
		lock_guard<mutex> _lock(demodulated_mtx_);
		demodulated_.resize( iq_samples_filtered_.size() );
		FSK2_Demod<TReal>(
				iq_samples_filtered_.data(), iq_samples_filtered_.size(),
				demodulated_.data() );

		symbol_extractor_.samplingRate( getDecimatedSamplingRate() );
		symbol_extractor_.pushSamples(demodulated_);
	}

	// Symbol Extractor
	//
	symbol_extractor_();

	vector<bool> symbols = symbol_extractor_.get();
	if(symbols.size())
	{
		rtty_.push(symbols);
		rtty_();
	}

	if( !rtty_.size() )
		return;


	vector<char> raw_chars = rtty_.get();
	const bool b_new_ssdv = ssdv_.push(raw_chars);

	vector<char> printable_chars;
	copy_if( raw_chars.begin(), raw_chars.end(), back_inserter(printable_chars),
			[](char c){return isprint(c) || c == '\n';}
	);
	rtty_char_stream_.insert( rtty_char_stream_.end(), printable_chars.begin(), printable_chars.end() );
	chr_callback_stream_.insert( chr_callback_stream_.end(), printable_chars.begin(), printable_chars.end() );

	if(live_print_)
	{
		for( auto c : printable_chars )
			cout<<c;
		cout.flush();
	}


	// scan for sentence from time to time
	if( rtty_char_stream_.size() > 20 /*last_sentence_.size()/2*/ )
	{
		bool b_scan_for_sentence = true;
		while(b_scan_for_sentence)
		{
			map<string,string> result = extractSentence(rtty_char_stream_);
			if( result["success"] == "OK" )
			{
				rtty_char_stream_ = result["stream"]; // rest of stream after sentence
				last_sentence_ = result["callsign"] + "," + result["data"] + "*" + result["crc"];
				printHabhubSentence( result["callsign"], result["data"], result["crc"] );
				cout<<endl;

				if( sentence_callback_ &&
					result["crc"] == CRC(result["callsign"] + "," + result["data"]) )
						sentence_callback_( result["callsign"], result["data"], result["crc"] );
			}
			else
			{
				b_scan_for_sentence = false;
			}
		}
	}


	// character_callback_ - every 250ms
	static thread_local auto character_callback_time = std::chrono::high_resolution_clock::now();
	std::chrono::milliseconds character_callback_age =
		std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::high_resolution_clock::now() - character_callback_time );

	if( 	chr_callback_stream_.size()
		&&	character_callback_age > std::chrono::milliseconds(250)
	 )
	{
		if(character_callback_)
			character_callback_( chr_callback_stream_ );
		chr_callback_stream_.clear();
		character_callback_time = std::chrono::high_resolution_clock::now();
	}

	if(b_new_ssdv && ssdv_callback_)
		ssdv_callback_( ssdv_.last_img_k_.first, ssdv_.last_img_k_.second, ssdv_.get_jpeg(ssdv_.last_img_k_) );

	// overflow protection
	if(rtty_char_stream_.size() > 1000)
		rtty_char_stream_.erase( 0, rtty_char_stream_.rfind('$') );

} // process()


template<typename TReal>
std::string Decoder<TReal>::getRTTY()
{
	return rtty_char_stream_;
}


template<typename TReal>
std::string Decoder<TReal>::getLastSentence()
{
	return last_sentence_;
}


template<typename TReal>
void Decoder<TReal>::baud(double i_baud)
{
	std::lock_guard<std::mutex> _lock(process_mutex_);
	symbol_extractor_.symbolRate(i_baud);
}


template<typename TReal>
double Decoder<TReal>::baud() const
{
	return symbol_extractor_.symbolRate();
}


template<typename TReal>
void Decoder<TReal>::rtty_bits(size_t i_ascii_bits)
{
	std::lock_guard<std::mutex> _lock(process_mutex_);
	rtty_.ascii_bits(i_ascii_bits);
}


template<typename TReal>
size_t Decoder<TReal>::rtty_bits()  const
{
	return rtty_.ascii_bits();
}


template<typename TReal>
void Decoder<TReal>::rtty_stops(float i_ascii_stops)
{
	std::lock_guard<std::mutex> _lock(process_mutex_);
	rtty_.ascii_stops(i_ascii_stops);
}


template<typename TReal>
float Decoder<TReal>::rtty_stops() const
{
	return rtty_.ascii_stops();
}


template<typename TReal>
void Decoder<TReal>::dc_remove(bool dc_rem)
{
	dc_remove_ = dc_rem;
}


template<typename TReal>
bool Decoder<TReal>::dc_remove() const
{
	return dc_remove_;
}


template<typename TReal>
int Decoder<TReal>::getDecimationFactor() const
{
	//std::lock_guard<std::mutex> _lock(process_mutex_);
	// this probably should be locked with process_mutex_
	return decimation_factor_;
}


template<typename TReal>
double Decoder<TReal>::getInputSamplingRate() const
{
	return iq_in_buffer_.samplingRate();
}


template<typename TReal>
double Decoder<TReal>::getDecimatedSamplingRate() const
{
	return getInputSamplingRate() / getDecimationFactor();
}


template<typename TReal>
double Decoder<TReal>::getSymbolRate() const
{
	return symbol_extractor_.symbolRate();
}


template<typename TReal>
size_t Decoder<TReal>::getBinsCount() const
{
	return fft_bins_cnt_;
}


template<typename TReal>
const typename Decoder<TReal>::TIQVector Decoder<TReal>::getFFT() const
{
	std::lock_guard<std::mutex> _lock(freq_out_mtx_);
	return freq_out_;
}


template<typename TReal>
const typename Decoder<TReal>::TRVector Decoder<TReal>::getDemodulated() const
{
	std::lock_guard<std::mutex> _lock(demodulated_mtx_);
	return demodulated_;
}


template<typename TReal>
const typename Decoder<TReal>::TRVector Decoder<TReal>::getPowerSpectrum() const
{
	std::lock_guard<std::mutex> _lock(afc_mtx_);
	// std::this_thread::sleep_for( ( std::chrono::duration<double, std::milli>(1000) ));
	return afc_.getPowerSpectrum();
}


template<typename TReal>
void Decoder<TReal>::getPeaks(int& pl, int& pr)
{
	std::lock_guard<std::mutex> _lock(afc_mtx_);
	afc_.getPeaks(pl, pr);
}


template<typename TReal>
void habdec::Decoder<TReal>::getNoiseFloor(double& nf, double& nv)
{
	std::lock_guard<std::mutex> _lock(afc_mtx_);
	afc_.getNoiseFloor(nf, nv);
}


template<typename TReal>
double habdec::Decoder<TReal>::getShift() const
{
	return afc_.getShift();
}


template<typename TReal>
double habdec::Decoder<TReal>::getFrequencyCorrection() const
{
	return afc_.getFrequencyCorrection();
}


template<typename TReal>
void habdec::Decoder<TReal>::resetFrequencyCorrection(double frequency_correction)
{
	afc_.resetFrequencyCorrection(frequency_correction);
}


template<typename TReal>
SpectrumInfo<TReal> habdec::Decoder<TReal>::getSpectrumInfo()
{
	SpectrumInfo<TReal> spectr_info;
	spectr_info = getPowerSpectrum();
	if(!spectr_info.size())
		return spectr_info;
	spectr_info.min_ = *std::min_element( spectr_info.cbegin(), spectr_info.cend() );
	spectr_info.max_ = *std::max_element( spectr_info.cbegin(), spectr_info.cend() );

	int pl, pr;
	getPeaks(pl, pr);
	spectr_info.peak_left_ = std::abs(pl);
	spectr_info.peak_left_valid_ = pl>0;
	spectr_info.peak_right_ = std::abs(pr);
	spectr_info.peak_right_valid_ = pr>0;

	getNoiseFloor(spectr_info.noise_floor_, spectr_info.noise_variance_);
	spectr_info.sampling_rate_ = getDecimatedSamplingRate();

	spectr_info.shift_ = getShift();

	return spectr_info;
}


template<typename T>
std::ostream& operator<<( std::ostream& output, const std::vector<T>& v )
{
	for(const auto& a : v)
		output<<a<<" ";
	return output;
}


} // namespace habdec
