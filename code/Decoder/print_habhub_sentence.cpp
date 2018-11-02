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

#include "print_habhub_sentence.h"

#include <iostream>
#include <iomanip>
#include "CRC.h"
#include "common/console_colors.h"


namespace habdec
{


void printHabhubSentence(
	const std::string& callsign,
	const std::string& data,
	const std::string in_crc )
{
	using namespace std;

	static int success = 0;
	static int failure = 0;

	if( in_crc == CRC(callsign + string(",") + data) )
	{
		cout<<C_CLEAR<<'\r'<<""<<C_MAGENTA
			<<callsign + string(",") + data + string("*") + in_crc
			<<" OK"<<C_OFF;
			++success;
	}
	else
	{
		cout<<C_CLEAR<<'\r'<<""<<C_RED
			<<callsign + string(",") + data + string("*") + in_crc
			<<" ERR"<<C_OFF;
			++failure;
	}

	cout<<"\t\tOK:"<<success
		<< "  ERR:"<<failure
		<<"  Ratio:"<<setprecision(2)<<(float(success)/(success+failure));

}


} // namespace habdec