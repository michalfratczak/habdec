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
#include <cstring>


template<typename T_flt_>
inline T_flt_ sinc(T_flt_ x)
{
	if(x)
		return sin(x)/x;
	else
		return 1;
}


template<typename T_flt_> // float like types...
inline T_flt_ BlackmanHarris(const size_t x, const size_t N)
{
	static const T_flt_ a0 = 0.35874;
	static const T_flt_ a1 = 0.48829;
	static const T_flt_ a2 = 0.14128;
	static const T_flt_ a3 = 0.01168;
	static const T_flt_ PI2 = 2.0*M_PI;
	static const T_flt_ PI4 = 4.0*M_PI;
	static const T_flt_ PI6 = 6.0*M_PI;

	const T_flt_ N_1 =  N-1;

	T_flt_ w = a0 - a1*cos(PI2*x / N_1) + a2*cos(PI4*x / N_1) - a3*cos(PI6*x / N_1);

	return w;
}
