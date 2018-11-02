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

#include <memory>
#include <fftw3.h>

namespace habdec
{


class FFT
{
public:
	FFT() = default;
	FFT(const FFT&) = delete;
	FFT& operator=(const FFT&) = delete;
	~FFT();

	void setInput(float* p_input, const size_t i_size);
	void setOutput(float* p_output);
	size_t size() const;

	void operator()();

private:

	void swap_half(); // swaps values on other half of FFT result

	size_t size_ = 0;

	float* p_in_ = 0;
	float* p_out_ = 0;

	// fftw stuff
	fftwf_plan fftw_plan_;
	fftwf_complex* fft_in_ = 0;
	fftwf_complex* fft_out_ = 0;

};


} // namespace habdec
