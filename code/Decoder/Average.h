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
#include <iostream>

namespace habdec
{

template<typename T>
class Average
{

public:
	Average(size_t max_count, T init_val=0) : sum_(0), count_(0), max_count_(std::max(size_t(1),max_count))
	{
		add(init_val);
	};

	double add(const T& val)
	{
		double diff = get() - val;

		if(count_ == max_count_)
		{
			sum_ = get() * (max_count_-1) + val;
		}
		else
		{
			++count_;
			sum_ += val;
		}

		return diff;

	}

	double get() const
	{
		if(!count_)
			return sum_;
		return double(sum_)/count_;
	}

	operator double() const { double val = get(); return val; }

	void reset(const T& val)
	{
		sum_ = val;
		count_ = 1;
	}


private:
	T 		sum_;
	size_t 	count_;
	const size_t 	max_count_;

};

} // namespace habdec