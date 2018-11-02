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

#include "IQSource.h"

#include <memory>
#include <iostream>
#include <fstream>
#include <thread>
#include <stdio.h>

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>


namespace habdec
{

template<typename T>
class IQSource_File : public habdec::IQSource
{
public:
	typedef T 				TValue;
	typedef std::complex<T> TComplex;

	bool start();
	bool stop();
	bool isRunning() const;
	size_t count() const;
	size_t get(void* p_data, const size_t i_count);
	std::string type() const {return std::string("File");};
	double samplingRate() const;
	bool setOption(const std::string& option, const void* p_data);
	bool getOption(const std::string& option, void* p_data);

	bool init();

	~IQSource_File();

private:
	bool b_is_running_ = false;
	bool b_is_realtime_ = true;
	bool b_loop_ = false;

	std::ifstream file_handle_;
	std::string file_path_;

	double sampling_rate_ = 0;
	size_t count_ = 0;
};

} // namespace



namespace
{
std::ifstream::pos_type filesize(const char* filename)
{
	std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
	return in.tellg();
}
} // namespace

namespace habdec
{

template<typename T>
bool IQSource_File<T>::start()
{
	if(!file_handle_.is_open())
		return false;
	b_is_running_ = true;
	return true;
}


template<typename T>
bool IQSource_File<T>::stop()
{
	if(!file_handle_.is_open())
		return false;
	b_is_running_ = true;
	return true;
}


template<typename T>
bool IQSource_File<T>::isRunning() const
{
	if(!file_handle_.is_open())
		return false;
	return b_is_running_;
}


template<typename T>
size_t IQSource_File<T>::count() const
{
	return count_;
}


template<typename T>
size_t IQSource_File<T>::get(void* p_data, const size_t i_count)
{
	using namespace std;

	if(!file_handle_.is_open())
	{
		std::cout<<"file_handle_ == 0"<<std::endl;
		return 0;
	}

	if(!isRunning())
	{
		std::cout<<"Not Running."<<std::endl;
		return 0;
	}

	if( file_handle_.eof() )
	{
		if(b_loop_)
		{
			cout<<file_path_<<" EOF. REWIND."<<endl;
			file_handle_.clear();
			file_handle_.seekg(0);
		}
		else
		{
			cout<<file_path_<<" EOF."<<endl;
			return 0;
		}
	}

	// READ DATA
	file_handle_.read( reinterpret_cast<char*>(p_data), min(i_count, count_) * sizeof(TComplex) );
	const size_t read_count = file_handle_.gcount() / sizeof(TComplex);

	if(!file_handle_)
	{
		if(read_count != i_count)
			cout<<"IQSource_File<T>::get() read less than desired: "<<read_count<<" of "<<i_count<<endl;
	}

	if(b_is_realtime_)
	{
		size_t wait = double(read_count) / sampling_rate_ * 1000;
		std::this_thread::sleep_for( std::chrono::duration<double, std::milli>(wait) );
	}

	return read_count;
}


template<typename T>
double IQSource_File<T>::samplingRate() const
{
	return sampling_rate_;
}


template<typename T>
IQSource_File<T>::~IQSource_File()
{
	if(file_handle_)
	{
		stop();
		// fclose(file_handle_);
		file_handle_.close();
	}
}


template<typename T>
bool IQSource_File<T>::init()
{
	count_ = filesize( file_path_.c_str() ) / sizeof(TComplex);
	file_handle_.open( file_path_, std::ios::binary );
	if(!file_handle_.is_open())
		return false;
	return true;
}


template<typename T>
bool IQSource_File<T>::setOption(const std::string& option, const void* p_data)
{
	void* p_data_nonconst = const_cast<void*>(p_data);

	if(option == "file_string")
	{
		file_path_ = *reinterpret_cast<std::string*>(p_data_nonconst);
	}
	if(option == "sampling_rate_double")
	{
		sampling_rate_ = *reinterpret_cast<double*>(p_data_nonconst);
	}
	else if(option == "realtime_bool")
	{
		b_is_realtime_ = *reinterpret_cast<bool*>(p_data_nonconst);
	}
	else if(option == "loop_bool")
	{
		b_loop_ = *reinterpret_cast<bool*>(p_data_nonconst);
	}
	else
	{
		std::cout<<"IQSource_File::setOption error. Unknown option: "<<option<<std::endl;
		return false;
	}
	return true;
}

template<typename T>
bool IQSource_File<T>::getOption(const std::string& option, void* p_data)
{
	if(option == "file_string")
	{
		*reinterpret_cast<std::string*>(p_data) = file_path_;
	}
	if(option == "sampling_rate_double")
	{
		*reinterpret_cast<double*>(p_data) = sampling_rate_;
	}
	else if(option == "realtime_bool")
	{
		*reinterpret_cast<bool*>(p_data) = b_is_realtime_;
	}
	else if(option == "loop_bool")
	{
		*reinterpret_cast<bool*>(p_data) = b_loop_;
	}
	else
	{
		std::cout<<"IQSource_File::getOption error. Unknown option: "<<option<<std::endl;
		return false;
	}
	return true;
}


} // namespace habdec