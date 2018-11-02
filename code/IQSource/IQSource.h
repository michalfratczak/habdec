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
#include <cstddef>

namespace habdec
{


class IQSource
{
public:
	virtual bool init() = 0;
	virtual bool start() = 0;
	virtual bool stop() = 0;
	virtual bool isRunning() const = 0;
	virtual std::string type() const = 0;
	virtual size_t count() const = 0;
	virtual size_t get(void* p_data, const size_t i_count) = 0;
	virtual double samplingRate() const = 0;
	virtual bool setOption(const std::string& option, const void* p_data) = 0;
	virtual bool getOption(const std::string& option, void* p_data) = 0;
};


} // namespace habdec