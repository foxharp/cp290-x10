
/* sun.c

calulates time and azmith of sunrise and sunset. Variables lat_d and lat_m
are the degree and minutes of latitude. long_d and long_m are degree and minutes
of longitude. Adjust these for your location.

and, tzone needs to be adjusted too.  it's the center of your timezone

8/17/86 Letcher Ross, Snohomish Washington
9/24/86  Times returned are now minutes past midnight.
		 Sunset is in 24 hour format.


11/7/94 modified to fit into x10 program.
*/

#include <stdio.h>
#include <time.h>
#include <math.h>

int sunr, suns;

int days[] = { 31, 28, 31,  30, 31, 30,  31, 31, 30,  31, 30, 31 };

extern char latitude[], longitude[];

void sun_();

/* double lat_d = 42, lat_m = 20, long_d = 71, long_m = 5; */
double lat_d, lat_m, long_d, long_m;
double tzone;  /* distance from UTC in hours */

sunriseset(rise, mp)
int rise;
int *mp;
{

    struct tm *tp;
    long now;
    time(&now);
    tp = localtime(&now);
    tzone = timezone / 3600;
    sun_(tp->tm_yday);
    if (rise) {
	*mp = sunr;
    } else {
	*mp = suns;
    }
    if (tp->tm_isdst > 0)  /* daylight saving?  off by an hour.  i think */
    	*mp += 60;
}

void sun_(day_of_yr)
int day_of_yr;
{
	int i;
	int tim();
	double d, dy, x, e, y, z, la, tp, dd, ee, cl, sd, td, st, cd, zz;
	double t, ct, tt;
	double pl = 3.14159/26;
	double j = 57.29578;
	extern int sunr, suns;

	if (lat_d == 0.0) {
		if (!*latitude || !*longitude) {
		    error("must set LATITUDE and LONGITUDE in config file");
			
		}
		if (sscanf(latitude, "%lf:%lf", &lat_d, &lat_m) != 2)
			error("bad latitude format");
		if (sscanf(longitude, "%lf:%lf", &long_d, &long_m) != 2)
			error("bad longitude format");
	}
	
	td = (long_d + long_m / 60 - (tzone * 15))/15;
	la = lat_d + lat_m / 60;
	dy = (double)day_of_yr;
	x = dy/7;
	d = .456 - 22.915 * cos(pl*x) - .43 * cos(2*pl*x) - .156 * cos(3*pl*x)
		+ 3.83 * sin(pl*x) + .06 * sin(2*pl*x) - .082 * sin(3*pl*x);
	e = 8.000001e-03 + .51 * cos(pl*x) - 3.197 * cos(2*pl*x) - .106
		* cos(3*pl*x) - .15 * cos(4*pl*x) - 7.317 * sin(pl*x) - 9.471
		* sin(2*pl*x) - .391 * sin(3*pl*x) - .242 * sin(4*pl*x);
	cl = cos(la/j);
	sd = sin(d/j);
	cd = cos(d/j);
	y = sd/cl;
	z = 90 - j * atan(y/sqrt(1-y*y));
	zz = z;

	st = sin(z/j)/cd;
	if( fabs(st) >= 1 ){
		t=6; tt=6;
	}
	else{
		ct = sqrt(1 - st * st);
		t = j/15*atan(st/ct);
		tt = t;
	}
	if( d < 0){
		t = 12 - t;
		tt = t;
	}
	t = t + td - e / 60 -.04;
	sunr = tim(t);
	t = 12 - tt;
	t = t + td - e / 60+ .04;
	suns = tim(t) + 720;
	return ;
}
int tim(t)
double t;
{
	int t1;
	int time;
	double dtime;
	double t2;

	t1 = (int)t;
	t2 = t - (double)t1;
	/*
	dtime = t1*100;
	*/
	dtime = t1 * 60; 
	t2 = ((t2 * 600 + 5) / 10);
	time = (int)(t2 + dtime);
	return (time);
}

sunrise(mp)
int *mp;
{
    sunriseset(1,mp);
}

sunset(mp)
int *mp;
{
    sunriseset(0,mp);
}
