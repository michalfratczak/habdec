#include "GpsDistance.h"

#include<cmath>

// #include <iostream>
// #include <algorithm>
// #include <iterator>

namespace habdec
{
    

std::ostream& operator<<(std::ostream& o, const GpsDistance& rhs )
{
    o<<"dist_line_: "<<rhs.dist_line_<<"\n"
     <<"dist_circle_: "<<rhs.dist_circle_<<"\n"
     <<"dist_radians_: "<<rhs.dist_radians_<<"\n"
     <<"elevation_: "<<rhs.elevation_<<"\n"
     <<"bearing_: "<<rhs.bearing_<<"\n";
     
    return o;
}


// taken from https://github.com/projecthorus/radiosonde_auto_rx/blob/master/auto_rx/autorx/utils.py
GpsDistance CalcGpsDistance(double lat1, double lon1, double alt1, double lat2, double lon2, double alt2)
{
    /*
    Calculate and return information from 2 (lat, lon, alt) points
    Input and output latitudes, longitudes, angles, bearings and elevations are
    in degrees, and input altitudes and output distances are in meters.
    */

    const double radius = 6371000.0; // Earth radius in meters

    // to radians
    lat1 *= M_PI / 180.0;
    lat2 *= M_PI / 180.0;
    lon1 *= M_PI / 180.0;
    lon2 *= M_PI / 180.0;

    // Calculate the bearing, the angle at the centre, and the great circle
    // distance using Vincenty's_formulae with f = 0 (a sphere). See
    // http://en.wikipedia.org/wiki/Great_circle_distance#Formulas and
    // http://en.wikipedia.org/wiki/Great-circle_navigation and
    // http://en.wikipedia.org/wiki/Vincenty%27s_formulae
    double d_lon = lon2 - lon1;
    double sa = cos(lat2) * sin(d_lon);
    double sb = (cos(lat1) * sin(lat2)) - (sin(lat1) * cos(lat2) * cos(d_lon));
    double bearing = atan2(sa, sb);
    double aa = sqrt((sa*sa) + (sb*sb));
    double ab = (sin(lat1) * sin(lat2)) + (cos(lat1) * cos(lat2) * cos(d_lon));
    double angle_at_centre = atan2(aa, ab);
    double great_circle_distance = angle_at_centre * radius;

    // Armed with the angle at the centre, calculating the remaining items
    // is a simple 2D triangley circley problem:

    // Use the triangle with sides (r + alt1), (r + alt2), distance in a
    // straight line. The angle between (r + alt1) and (r + alt2) is the
    // angle at the centre. The angle between distance in a straight line and
    // (r + alt1) is the elevation plus pi/2.

    // Use sum of angle in a triangle to express the third angle in terms
    // of the other two. Use sine rule on sides (r + alt1) and (r + alt2),
    // expand with compound angle formulae and solve for tan elevation by
    // dividing both sides by cos elevation
    double ta = radius + alt1;
    double tb = radius + alt2;
    double ea = (cos(angle_at_centre) * tb) - ta;
    double eb = sin(angle_at_centre) * tb;
    double elevation = atan2(ea, eb);

    // Use cosine rule to find unknown side.
    double line_distance = sqrt((ta*ta) + (tb*tb) - 2 * tb * ta * cos(angle_at_centre));

    // Give a bearing in range 0 <= b < 2pi
    if (bearing < 0)
        bearing += 2 * M_PI;


    GpsDistance res;
    res.dist_line_ = line_distance;
    res.dist_circle_ = great_circle_distance;
    res.dist_radians_ = angle_at_centre;
    res.elevation_ = elevation / ( M_PI / 180.0 );
    res.bearing_ = bearing / ( M_PI / 180.0 );
    return res;

}

} // namespace habdec