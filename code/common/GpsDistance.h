#define _USE_MATH_DEFINES
#include<cmath>

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>


namespace habdec
{

struct GpsDistance
{
    double   dist_line_ = 0;
    double   dist_circle_ = 0;
    double   dist_radians_ = 0;  // angular distance over Earth surface, in radians
    double   elevation_ = 0;     // degrees
    double   bearing_ = 0;       // degrees
};

std::ostream& operator<<(std::ostream& o, const GpsDistance& rhs );

GpsDistance CalcGpsDistance(double lat1, double lon1, double alt1, double lat2, double lon2, double alt2);

} // namespace habdec
