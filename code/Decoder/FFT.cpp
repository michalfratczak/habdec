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

#include "FFT.h"

#include <cstring>
#include <iostream>
#include <complex>

namespace habdec
{

FFT::~FFT()
{
	if(size_)
		fftwf_destroy_plan(fftw_plan_);
	if(fft_in_)
		fftwf_free(fft_in_);
	if(fft_out_)
		fftwf_free(fft_out_);
}


size_t  FFT::size() const
{
	return size_;
}


void  FFT::setInput(float* p_input, const size_t i_size)
{
	if(i_size != size_)
	{
		std::cout<<"FFT rebuild to "<<i_size<<std::endl;

		if(size_)
			fftwf_destroy_plan(fftw_plan_);

		size_ = i_size;

		if(fft_in_)	fftwf_free(fft_in_);
		if(fft_out_)	fftwf_free(fft_out_);

		fft_in_ = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * size_);
		fft_out_= (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * size_);

		fftw_plan_ = fftwf_plan_dft_1d(size_, fft_in_, fft_out_, FFTW_FORWARD, FFTW_ESTIMATE);
	}

	p_in_ = p_input;
}


void  FFT::setOutput(float* p_output)
{
	p_out_ = p_output;
}


void FFT::swap_half() // swaps values on other half of FFT result
{
	if(!size())
		return;

	std::complex<float>* p_samples_float = reinterpret_cast<std::complex<float>*>(p_out_);

	const size_t half = size()/2;
	for(size_t i=0; i<half; ++i)
		std::swap( p_samples_float[i], p_samples_float[i+half] );
}


void  FFT::operator()()
{
	if( p_in_ && p_out_ )
	{
		memcpy( (void*) fft_in_, p_in_,   size() * sizeof(std::complex<float>) );
		fftwf_execute(fftw_plan_);
		memcpy( (void*) p_out_, fft_out_, size() * sizeof(std::complex<float>) );

		swap_half();
	}
	else if(!p_in_)
	{
		std::cout<<"FFT No Input"<<std::endl;
	}
	else if(!p_out_)
	{
		std::cout<<"FFT No Output"<<std::endl;
	}
}


} // namespace habdec