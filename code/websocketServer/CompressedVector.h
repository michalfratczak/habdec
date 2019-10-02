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
#include <algorithm>
#include <iostream>

namespace habdec
{

template<typename T>
class CompressedVector
{

public:

	typedef T TValue;

	mutable double min_ = 0;
	mutable double max_ = 0;
	std::vector<T>	values_;

	virtual ~CompressedVector() = default;
	CompressedVector() = default;

	template<typename U>
	CompressedVector(const CompressedVector<U>& rhs) :
							min_(rhs.min_),
							max_(rhs.max_)
							// normalized_(rhs.normalized_)
	{
		copyValues<U>(rhs);
	}

	template<typename U>
	CompressedVector(const std::vector<U>& rhs_vec)
	{
		min_ = *std::min_element(rhs_vec.begin(), rhs_vec.end());
		max_ = *std::max_element(rhs_vec.begin(), rhs_vec.end());
		copyValues<U>(rhs_vec, min_, max_);
	}

	template<typename U>
	const CompressedVector<T>& operator=(const CompressedVector<U>& rhs)
	{
		min_ = rhs.min_;
		max_ = rhs.max_;
		copyValues<U>(rhs);
		return *this;
	}

	double calcMin() const
	{
		if(!values_.size())
			return 0;
		min_ = *std::min_element(values_.begin(), values_.end());
		return min_;
	}

	double calcMax() const
	{
		if(!values_.size())
			return 0;
		max_ = *std::max_element(values_.begin(), values_.end());
		return max_;
	}
	
private:

	template<typename U>
	void copyValues(const std::vector<U>& rhs, double i_min, double i_max);

};

} // namespace habdec