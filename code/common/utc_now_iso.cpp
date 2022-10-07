#include "utc_now_iso.h"
#include <sstream>
#include <chrono>
#include "date.h"

std::string utc_now_iso()
{
    using namespace std;
    using namespace chrono;
    using namespace date;

    auto sys_now = system_clock::now();
    auto sys_YMD = year_month_day(floor<days>(sys_now));
    auto sys_HMS = make_time(sys_now - floor<days>(sys_now) );

    stringstream  ss;
    ss<<sys_YMD<<"T"<<sys_HMS<<"Z";

    return ss.str();
}
