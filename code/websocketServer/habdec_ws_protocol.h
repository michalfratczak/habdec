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
#include <string>
#include <sstream>
#include <vector>

// internal message passing structure
// this is not part of protocol
struct HabdecMessage
{
	bool is_binary_ = false;
	bool to_all_clients_ = false;
	std::stringstream data_stream_;
    HabdecMessage() { data_stream_.precision(12); }
};

// handles request and returns list of reponses
std::vector< std::shared_ptr<HabdecMessage> >  HandleRequest(std::string request);
