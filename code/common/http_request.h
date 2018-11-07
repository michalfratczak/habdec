
#pragma once

#include <string>

namespace habdec
{

enum class HTTP_VERB { kUnknown = 0, kGet, kPost };

int HttpRequest(
        const std::string host,
        const std::string target,
        const int port,
        HTTP_VERB i_verb,
        const std::string i_content_type,
        const std::string i_body,
        std::string& o_body
    );

} // namespace habdec
