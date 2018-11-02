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
#include <atomic>

namespace habdec
{

// IQ vector with sampling rate
//
template<typename T>
class IQVector : public std::vector< std::complex<T> >
{
public:
	typedef T 				TValue;
	typedef std::complex<T> TComplex;

	IQVector() = default;
	IQVector(const IQVector<T>& rhs);
	IQVector<T>	operator=(const IQVector<T>& rhs);

	double samplingRate() const {return sampling_rate_;}
	void samplingRate(const double i_samplingRate) {sampling_rate_ = i_samplingRate;}

private:
	std::atomic<double> sampling_rate_ = {0};
};


template<typename T>
IQVector<T>::IQVector(const IQVector<T>& rhs) : std::vector< std::complex<T> >::vector(rhs)
{
	sampling_rate_.store(rhs.sampling_rate_);
}


template<typename T>
IQVector<T>	IQVector<T>::operator=(const IQVector<T>& rhs)
{
	std::vector< std::complex<T> >::operator=(rhs);
	sampling_rate_.store(rhs.sampling_rate_);
	return *this;
}


template<typename T>
std::ostream& operator<<( std::ostream& output, const IQVector<T>& iqVector )
{
	for(size_t i=0; i<iqVector.size(); ++i)
		output<<iqVector[i]<<" ";
	return output;
}


} // namespace habdec
