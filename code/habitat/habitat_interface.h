#pragma once

#include <string>
#include <map>

namespace habdec
{

std::string HabitatUploadSentence(
        const std::string& i_sentence,
        const std::string& i_listener_callsign
    );

} // namespace