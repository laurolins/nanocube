#include<stdio.h>
#include<stdlib.h>
#include<math.h>

#define pt_sin_f64(x)  sin(x)
#define pt_cos_f64(x)  cos(x)
#define pt_sqrt_f64(x) sqrt(x)
#define pt_atan2_f64(x,y) atan2(x,y)
#define pt_PI 3.14159265358979323846

typedef float f32;
typedef float f64;

#define pt_EARTH_RADIUS 6371000.0

static f64
haversine_dist(f64 lat1, f64 lon1, f64 lat2, f64 lon2)
{
	f64 rx1 = pt_PI * lon1/180;
	f64 ry1 = pt_PI * lat1/180;
	f64 rx2 = pt_PI * lon2/180;
	f64 ry2 = pt_PI * lat2/180;

	f64 drx = rx2 - rx1;
	f64 dry = ry2 - ry1;

	f64 sin_dry = pt_sin_f64(dry/2);
	f64 sin_drx = pt_sin_f64(drx/2);
	f64 a = sin_dry * sin_dry +
		pt_cos_f64(ry1) * pt_cos_f64(ry2) * sin_drx * sin_drx;

	f64 c = 2 * pt_atan2_f64(pt_sqrt_f64(a), pt_sqrt_f64(1.0 - a));

	return pt_EARTH_RADIUS * c;
}

int main()
{
	f64 lat1 =  40.686662;
	f64 lon1 = -73.982197;
	f64 lat2 =  40.685307;
	f64 lon2 = -73.980792;

	f64 dist = haversine_dist(lat1, lon1, lat2, lon2);

	printf("%f meters\n", dist);

	return 0;
}

