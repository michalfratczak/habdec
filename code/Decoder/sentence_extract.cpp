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

#include "sentence_extract.h"

#include <iostream>

#include "CRC.h"

namespace habdec
{

std::regex HABHUB_REGEX( R"_(.*?(\$+)([\w,\-]+?),(.+?)(\*|\$)(\w\w\w\w).*)_" );


// usefull for some testing
std::map<std::string,std::string> extractSentence_testing(std::string stream)
{
	using namespace std;

	map<string,string> result;
	if(stream.size()>25)
	{
		static thread_local int index = 0;
		index++;

		result["success"] = string("OK");
		result["callsign"] = "CALLSIGN";
		result["data"] =  to_string(index) + ",15:41:24,"
						+ to_string(52.0 + .01 * index) + "," + to_string(21.0 + .01 * index) + ","
						+ to_string(100 * index)
						+",0,0,12,30.7,0.0,0.001,20.2,958*6BC9";
		result["crc"] = CRC(result["callsign"] + "," + result["data"]);
		result["stream"] = stream.substr(stream.size()/3);
		return result;
	}
	return result;
}


std::map<std::string,std::string> extractSentence(std::string stream)
{
	using namespace std;

	// return extractSentence_testing(stream);

	// replace '\n' in stream with spaces, otherwise regex will fail (C++ 11 issue)
	replace( stream.begin(), stream.end(), '\n', ' ');

	map<string,string> result;
	result["success"] = string("ERR");

	const int CRC_LEN = 4; // usually 4 chars after *

	if(stream.find("*") < string::npos-CRC_LEN)
	{
		smatch match;
		regex_match(stream, match, HABHUB_REGEX);

		if(match.size() >= 5)
		{
			const string callsign = match[2];
			const string data = match[3];
			const string data_end = match[4]; // * or another $
			const string _crc = match[5];

			const size_t cut = min( stream.size(), size_t(match.position(4)+CRC_LEN) );
			stream = stream.substr( cut );

			result["success"] = string("OK");
			result["callsign"] = callsign;
			result["data"] = data;
			result["crc"] = _crc;
			result["stream"] = stream;

		}

	}

	return result;
}

} // namespace habdec
