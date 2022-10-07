#include "sondehub_uploader.h"

#include <iostream>
#include <boost/algorithm/string.hpp>
#include "common/utc_now_iso.h"
#include "common/git_repo_sha1.h"
#include <cpr/cpr.h>

namespace sondehub
{


void SondeHubUploader::push(const MinTelemetry& t)
{
    if(api_endpoint_ == "" || uploader_callsign_ == "")
        return;

    std::lock_guard<std::mutex> _lock(queue_mtx_);
    queue_.push_back(t);
}


size_t SondeHubUploader::size() const
{
    std::lock_guard<std::mutex> _lock(queue_mtx_);
    return queue_.size();
}


void SondeHubUploader::upload()
{
    if(api_endpoint_ == "" || uploader_callsign_ == "")
        return;

    processing_queue_.clear();

    {
        std::lock_guard<std::mutex> _lock(queue_mtx_);
        if(!queue_.size())
            return;
        queue_.swap(processing_queue_);
    }

    using namespace std;

    const string utc_now_str = utc_now_iso();
    string payload{"["};

    for(auto& t : processing_queue_) {
        string tele_json = JSON_TEMPLATE;

        boost::replace_all(tele_json, "$uploader_callsign", uploader_callsign_);
        boost::replace_all(tele_json, "$software_version", string(g_GIT_SHA1).substr(0,7));
        boost::replace_all(tele_json, "$time_received", t.time_received);
        boost::replace_all(tele_json, "$upload_time", utc_now_str);
        boost::replace_all(tele_json, "$payload_callsign", t.payload_callsign);
        boost::replace_all(tele_json, "$datetime", t.datetime);
        boost::replace_all(tele_json, "$frame", to_string(t.frame));
        boost::replace_all(tele_json, "$lat", to_string(t.lat));
        boost::replace_all(tele_json, "$lon", to_string(t.lon));
        boost::replace_all(tele_json, "$alt", to_string(static_cast<int>(t.alt)));

        payload += tele_json + string{','};
    }

    payload.at(payload.size()-1) = ']';

    // cout<<payload<<endl;
    // cout<<"uploading to "<<api_endpoint_<<endl;
    // return;

    cpr::Response r = cpr::Put(
				cpr::Url{api_endpoint_},
				cpr::Body{payload},
				cpr::Header{
					{"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:105.0) Gecko/20100101 Firefox/105.0"},
					{"Content-Type", "application/json"}
				}
			);

    if(r.status_code != 200) {
        cout<<"sondehub upload error:\n";
        cout<<r.status_code<<endl;                  // 200
        cout<<r.header["content-type"]<<endl;       // application/json; charset=utf-8
        cout<<r.text<<endl;
    }
}


}