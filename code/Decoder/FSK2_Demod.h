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

#include <complex>

namespace habdec
{


template<typename T>
void FSK2_Demod(
		const std::complex<T>* p_cmplx_samples, const size_t samples_count,
		T* o_demod )
{
	// TO DO - this should not be static if threads...
	thread_local static std::complex<T> last_value = p_cmplx_samples[0];

	for(size_t i=1; i<samples_count; ++i)
		o_demod[i] = std::arg( p_cmplx_samples[i] * std::conj(p_cmplx_samples[i-1]) );

	o_demod[0] = std::arg( p_cmplx_samples[0] * std::conj(last_value) );
	last_value = p_cmplx_samples[samples_count-1];
}


} // namespace habdec

