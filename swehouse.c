
/*******************************************************
$Header: swehouse.c,v 1.31 2000/09/18 18:54:50 dieter Exp $
module swehouse.c
house and (simple) aspect calculation 

*******************************************************/
/* Copyright (C) 1997, 1998 Astrodienst AG, Switzerland.  All rights reserved.
  
  This file is part of Swiss Ephemeris Free Edition.
  
  Swiss Ephemeris is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Swiss Ephemeris Public License
  ("SEPL" or the "License") for full details.
  
  Every copy of Swiss Ephemeris must include a copy of the License,
  normally in a plain ASCII text file named LICENSE.  The License grants you
  the right to copy, modify and redistribute Swiss Ephemeris, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notices and this notice be preserved on
  all copies.

  For uses of the Swiss Ephemeris which do not fall under the definitions
  laid down in the Public License, the Swiss Ephemeris Professional Edition
  must be purchased by the developer before he/she distributes any of his
  software or makes available any product or service built upon the use of
  the Swiss Ephemeris.

  Authors of the Swiss Ephemeris: Dieter Koch and Alois Treindl

  The authors of Swiss Ephemeris have no control or influence over any of
  the derived works, i.e. over software or services created by other
  programmers which use Swiss Ephemeris functions.

  The names of the authors or of the copyright holder (Astrodienst) must not
  be used for promoting any software, product or service which uses or contains
  the Swiss Ephemeris. This copyright notice is the ONLY place where the
  names of the authors can legally appear, except in cases where they have
  given special permission in writing.

  The trademarks 'Swiss Ephemeris' and 'Swiss Ephemeris inside' may be used
  for promoting such software, products or services.
*/

#include "swephexp.h"
#include "sweph.h"
#include "swephlib.h"
#include "swehouse.h"
#include <string.h>

#define MILLIARCSEC 	(1.0 / 3600000.0)

static double Asc1(double, double, double, double);
static double Asc2(double, double, double, double);
static int CalcH(
	double th, double fi, double ekl, char hsy, 
	int iteration_count, struct houses *hsp );
static int sidereal_houses_ecl_t0(double tjde, 
                           double armc, 
                           double eps, 
                           double *nutlo, 
                           double lat, 
			   int hsys, 
                           double *cusp, 
                           double *ascmc);
static int sidereal_houses_trad(double tjde, 
                           double armc, 
                           double eps, 
                           double nutl, 
                           double lat, 
			   int hsys, 
                           double *cusp, 
                           double *ascmc);
static int sidereal_houses_ssypl(double tjde, 
                           double armc, 
                           double eps, 
                           double *nutlo, 
                           double lat, 
			   int hsys, 
                           double *cusp, 
                           double *ascmc);

/* housasp.c 
 * cusps are returned in double cusp[13]:
 * cusp[1...12]	houses 1 - 12
 * additional points are returned in ascmc[10].
 * ascmc[0] = ascendant
 * ascmc[1] = mc
 * ascmc[2] = armc
 * ascmc[3] = vertex
 * ascmc[4] = equasc		* "equatorial ascendant" *
 * ascmc[5] = coasc1		* "co-ascendant" (W. Koch) *
 * ascmc[6] = coasc2		* "co-ascendant" (M. Munkasey) *
 * ascmc[7] = polasc		* "polar ascendant" (M. Munkasey) *
 */
int FAR PASCAL_CONV swe_houses(double tjd_ut,
				double geolat,
				double geolon,
				int hsys,
				double *cusp,
				double *ascmc)
{
  int i, retc = 0;
  double armc, eps, nutlo[2];
  double tjde = tjd_ut + swe_deltat(tjd_ut);
  eps = swi_epsiln(tjde) * RADTODEG;
  swi_nutation(tjde, nutlo);
  for (i = 0; i < 2; i++)
    nutlo[i] *= RADTODEG;
  armc = swe_degnorm(swe_sidtime0(tjd_ut, eps + nutlo[1], nutlo[0]) * 15 + geolon);
#ifdef TRACE
  swi_open_trace(NULL);
  if (swi_trace_count <= TRACE_COUNT_MAX) {
    if (swi_fp_trace_c != NULL) {
      fputs("\n/*SWE_HOUSES*/\n", swi_fp_trace_c);
      fprintf(swi_fp_trace_c, "#if 0\n");
      fprintf(swi_fp_trace_c, "  tjd = %.9f;", tjd_ut);
      fprintf(swi_fp_trace_c, " geolon = %.9f;", geolon);
      fprintf(swi_fp_trace_c, " geolat = %.9f;", geolat);
      fprintf(swi_fp_trace_c, " hsys = %d;\n", hsys);
      fprintf(swi_fp_trace_c, "  retc = swe_houses(tjd, geolat, geolon, hsys, cusp, ascmc);\n");
      fprintf(swi_fp_trace_c, "  /* swe_houses calls swe_houses_armc as follows: */\n");
      fprintf(swi_fp_trace_c, "#endif\n");
      fflush(swi_fp_trace_c);
    }
  }
#endif
  retc = swe_houses_armc(armc, geolat, eps + nutlo[1], hsys, cusp, ascmc);
  return retc;
}

/* housasp.c 
 * cusps are returned in double cusp[13]:
 * cusp[1...12]	houses 1 - 12
 * additional points are returned in ascmc[10].
 * ascmc[0] = ascendant
 * ascmc[1] = mc
 * ascmc[2] = armc
 * ascmc[3] = vertex
 * ascmc[4] = equasc		* "equatorial ascendant" *
 * ascmc[5] = coasc1		* "co-ascendant" (W. Koch) *
 * ascmc[6] = coasc2		* "co-ascendant" (M. Munkasey) *
 * ascmc[7] = polasc		* "polar ascendant" (M. Munkasey) *
 */
int FAR PASCAL_CONV swe_houses_ex(double tjd_ut,
                                int32 iflag, 
				double geolat,
				double geolon,
				int hsys,
				double *cusp,
				double *ascmc)
{
  int i, retc = 0;
  double armc, eps_mean, nutlo[2];
  double tjde = tjd_ut + swe_deltat(tjd_ut);
  struct sid_data *sip = &swed.sidd;
  if ((iflag & SEFLG_SIDEREAL) && !swed.ayana_is_set)
    swe_set_sid_mode(SE_SIDM_FAGAN_BRADLEY, 0, 0);
  eps_mean = swi_epsiln(tjde) * RADTODEG;
  swi_nutation(tjde, nutlo);
  for (i = 0; i < 2; i++)
    nutlo[i] *= RADTODEG;
#ifdef TRACE
  swi_open_trace(NULL);
  if (swi_trace_count <= TRACE_COUNT_MAX) {
    if (swi_fp_trace_c != NULL) {
      fputs("\n/*SWE_HOUSES_EX*/\n", swi_fp_trace_c);
      fprintf(swi_fp_trace_c, "#if 0\n");
      fprintf(swi_fp_trace_c, "  tjd = %.9f;", tjd_ut);
      fprintf(swi_fp_trace_c, " iflag = %d;\n", iflag);
      fprintf(swi_fp_trace_c, " geolon = %.9f;", geolon);
      fprintf(swi_fp_trace_c, " geolat = %.9f;", geolat);
      fprintf(swi_fp_trace_c, " hsys = %d;\n", hsys);
      fprintf(swi_fp_trace_c, "  retc = swe_houses_ex(tjd, iflag, geolat, geolon, hsys, cusp, ascmc);\n");
      fprintf(swi_fp_trace_c, "  /* swe_houses calls swe_houses_armc as follows: */\n");
      fprintf(swi_fp_trace_c, "#endif\n");
      fflush(swi_fp_trace_c);
    }
  }
#endif
    /*houses_to_sidereal(tjde, geolat, hsys, eps, cusp, ascmc, iflag);*/
  armc = swe_degnorm(swe_sidtime0(tjd_ut, eps_mean + nutlo[1], nutlo[0]) * 15 + geolon);
  if (iflag & SEFLG_SIDEREAL) { 
    if (sip->sid_mode & SE_SIDBIT_ECL_T0)
      retc = sidereal_houses_ecl_t0(tjde, armc, eps_mean + nutlo[1], nutlo, geolat, hsys, cusp, ascmc);
    else if (sip->sid_mode & SE_SIDBIT_SSY_PLANE)
      retc = sidereal_houses_ssypl(tjde, armc, eps_mean + nutlo[1], nutlo, geolat, hsys, cusp, ascmc);
    else
      retc = sidereal_houses_trad(tjde, armc, eps_mean + nutlo[1], nutlo[0], geolat, hsys, cusp, ascmc);
  } else {
    retc = swe_houses_armc(armc, geolat, eps_mean + nutlo[1], hsys, cusp, ascmc);
  }
  if (iflag & SEFLG_RADIANS) {
    for (i = 1; i <= 12; i++)
      cusp[i] *= DEGTORAD;
    for (i = 0; i < SE_NASCMC; i++)
      ascmc[i] *= DEGTORAD;
  }
  return retc;
}

/*
 * houses to sidereal
 * ------------------
 * there are two methods: 
 * a) the traditional one
 *    houses are computed tropically, then nutation and the ayanamsa
 *    are subtracted.
 * b) the projection on the ecliptic of t0
 *    The house computation is then as follows:
 *
 * Be t the birth date and t0 the epoch at which ayanamsa = 0.
 * 1. Compute the angle between the mean ecliptic at t0 and 
 *    the true equator at t.
 *    The intersection point of these two circles we call the 
 *    "auxiliary vernal point", and the angle between them the 
 *    "auxiliary obliquity".
 * 2. Compute the distance of the auxiliary vernal point from the 
 *    vernal point at t. (this is a section on the equator)
 * 3. subtract this value from the armc of t = aux. armc.
 * 4. Compute the axes and houses for this aux. armc and aux. obliquity.
 * 5. Compute the distance between the auxiliary vernal point and the
 *    vernal point at t0 (this is the ayanamsa at t, measured on the
 *    ecliptic of t0)
 * 6. subtract this distance from all house cusps.
 * 7. subtract ayanamsa_t0 from all house cusps.
 */
static int sidereal_houses_ecl_t0(double tjde, 
                           double armc, 
                           double eps, 
                           double *nutlo, 
                           double lat, 
			   int hsys, 
                           double *cusp, 
                           double *ascmc)
{
  int i, j, retc = OK;
  double x[6], xvpx[6], x2[6], epst0, xnorm[6];
  double rxy, rxyz, c2, epsx, sgn, fac, dvpx, dvpxe;
  double armcx;
  struct sid_data *sip = &swed.sidd;
  /* epsilon at t0 */
  epst0 = swi_epsiln(sip->t0);
  /* cartesian coordinates of an imaginary moving body on the
   * the mean ecliptic of t0; we take the vernal point: */
  x[0] = x[4] = 1; 
  x[1] = x[2] = x[3] = x[5] = 0;
  /* to equator */
  swi_coortrf(x, x, -epst0);
  swi_coortrf(x+3, x+3, -epst0);
  /* to tjd_et */
  swi_precess(x, sip->t0, J_TO_J2000);
  swi_precess(x, tjde, J2000_TO_J);
  swi_precess(x+3, sip->t0, J_TO_J2000);
  swi_precess(x+3, tjde, J2000_TO_J);
  /* to true equator of tjd_et */
  swi_coortrf(x, x, (eps - nutlo[1]) * DEGTORAD);
  swi_coortrf(x+3, x+3, (eps - nutlo[1]) * DEGTORAD);
  swi_cartpol_sp(x, x);
  x[0] += nutlo[0] * DEGTORAD;
  swi_polcart_sp(x, x);
  swi_coortrf(x, x, -eps * DEGTORAD);
  swi_coortrf(x+3, x+3, -eps * DEGTORAD);
  /* now, we have the moving point precessed to tjd_et.
   * next, we compute the auxiliary epsilon: */
  swi_cross_prod(x, x+3, xnorm);
  rxy =  xnorm[0] * xnorm[0] + xnorm[1] * xnorm[1];
  c2 = (rxy + xnorm[2] * xnorm[2]);
  rxyz = sqrt(c2);
  rxy = sqrt(rxy);
  epsx = asin(rxy / rxyz) * RADTODEG;           /* 1a */
  /* auxiliary vernal point */
  if (fabs(x[5]) < 1e-15)
    x[5] = 1e-15;
  fac = x[2] / x[5];
  sgn = x[5] / fabs(x[5]);
  for (j = 0; j <= 2; j++)
    xvpx[j] = (x[j] - fac * x[j+3]) * sgn;      /* 1b */
  /* distance of the auxiliary vernal point from 
   * the zero point at tjd_et (a section on the equator): */
  swi_cartpol(xvpx, x2);
  dvpx = x2[0] * RADTODEG;                      /* 2 */
  /* auxiliary armc */
  armcx = swe_degnorm(armc - dvpx);        /* 3 */
  /* compute axes and houses: */
  retc = swe_houses_armc(armcx, lat, epsx, hsys, cusp, ascmc);  /* 4 */
  /* distance between auxiliary vernal point and
   * vernal point of t0 (a section on the sidereal plane) */
  dvpxe = acos(swi_dot_prod_unit(x, xvpx)) * RADTODEG;  /* 5 */
  if (tjde < sip->t0)
    dvpxe = -dvpxe;
  for (i = 1; i <= 12; i++)                     /* 6, 7 */
    cusp[i] = swe_degnorm(cusp[i] - dvpxe - sip->ayan_t0);
  for (i = 0; i <= SE_NASCMC; i++)
    ascmc[i] = swe_degnorm(ascmc[i] - dvpxe - sip->ayan_t0);
  return retc;
}

/*
 * Be t the birth date and t0 the epoch at which ayanamsa = 0.
 * 1. Compute the angle between the solar system rotation plane and 
 *    the true equator at t.
 *    The intersection point of these two circles we call the 
 *    "auxiliary vernal point", and the angle between them the 
 *    "auxiliary obliquity".
 * 2. Compute the distance of the auxiliary vernal point from the 
 *    zero point at t. (this is a section on the equator)
 * 3. subtract this value from the armc of t = aux. armc.
 * 4. Compute the axes and houses for this aux. armc and aux. obliquity.
 * 5. Compute the distance between the auxiliary vernal point at t
 *    and the zero point of the solar system plane J2000
 *    (a section measured on the solar system plane)
 * 6. subtract this distance from all house cusps.
 * 7. compute the ayanamsa of J2000 on the solar system plane, 
 *    referred to t0
 * 8. subtract ayanamsa_t0 from all house cusps.
 * 9. subtract ayanamsa_2000 from all house cusps.
 */
static int sidereal_houses_ssypl(double tjde, 
                           double armc, 
                           double eps, 
                           double *nutlo, 
                           double lat, 
			   int hsys, 
                           double *cusp, 
                           double *ascmc)
{
  int i, j, retc = OK;
  double x[6], x0[6], xvpx[6], x2[6], epst0, xnorm[6];
  double rxy, rxyz, c2, epsx, eps2000, sgn, fac, dvpx, dvpxe, x00;
  double armcx;
  struct sid_data *sip = &swed.sidd;
  /* epsilon at t0 */
  epst0 = swi_epsiln(sip->t0);
  eps2000 = swi_epsiln(J2000);
  /* cartesian coordinates of the zero point on the
   * the solar system rotation plane */
  x[0] = x[4] = 1; 
  x[1] = x[2] = x[3] = x[5] = 0;
  /* to ecliptic 2000 */
  swi_coortrf(x, x, -SSY_PLANE_INCL);
  swi_coortrf(x+3, x+3, -SSY_PLANE_INCL);
  swi_cartpol_sp(x, x);
  x[0] += SSY_PLANE_NODE_E2000;
  swi_polcart_sp(x, x);
  /* to equator 2000 */
  swi_coortrf(x, x, -eps2000);
  swi_coortrf(x+3, x+3, -eps2000);
  /* to mean equator of t */
  swi_precess(x, tjde, J2000_TO_J);
  swi_precess(x+3, tjde, J2000_TO_J);
  /* to true equator of t */
  swi_coortrf(x, x, (eps - nutlo[1]) * DEGTORAD);
  swi_coortrf(x+3, x+3, (eps - nutlo[1]) * DEGTORAD);
  swi_cartpol_sp(x, x);
  x[0] += nutlo[0] * DEGTORAD;
  swi_polcart_sp(x, x);
  swi_coortrf(x, x, -eps * DEGTORAD);
  swi_coortrf(x+3, x+3, -eps * DEGTORAD);
  /* now, we have the moving point precessed to tjd_et.
   * next, we compute the auxiliary epsilon: */
  swi_cross_prod(x, x+3, xnorm);
  rxy =  xnorm[0] * xnorm[0] + xnorm[1] * xnorm[1];
  c2 = (rxy + xnorm[2] * xnorm[2]);
  rxyz = sqrt(c2);
  rxy = sqrt(rxy);
  epsx = asin(rxy / rxyz) * RADTODEG;           /* 1a */
  /* auxiliary vernal point */
  if (fabs(x[5]) < 1e-15)
    x[5] = 1e-15;
  fac = x[2] / x[5];
  sgn = x[5] / fabs(x[5]);
  for (j = 0; j <= 2; j++)
    xvpx[j] = (x[j] - fac * x[j+3]) * sgn;      /* 1b */
  /* distance of the auxiliary vernal point from 
   * mean vernal point at tjd_et (a section on the equator): */
  swi_cartpol(xvpx, x2);
  dvpx = x2[0] * RADTODEG;                      /* 2 */
  /* auxiliary armc */
  armcx = swe_degnorm(armc - dvpx);        /* 3 */
  /* compute axes and houses: */
  retc = swe_houses_armc(armcx, lat, epsx, hsys, cusp, ascmc);  /* 4 */
  /* distance between the auxiliary vernal point at t and
   * the sidereal zero point of 2000 at t
   * (a section on the sidereal plane).
   */ 
  dvpxe = acos(swi_dot_prod_unit(x, xvpx)) * RADTODEG;  /* 5 */
                /* (always positive for dates after 5400 bc) */
  dvpxe -= SSY_PLANE_NODE * RADTODEG;
  /* ayanamsa between t0 and J2000, measured on solar system plane: */
  /* position of zero point of t0 */
  x0[0] = 1; 
  x0[1] = x0[2] = 0; 
  /* zero point of t0 in J2000 system */
  if (sip->t0 != J2000)
    swi_precess(x0, sip->t0, J_TO_J2000);
  /* zero point to ecliptic 2000 */
  swi_coortrf(x0, x0, eps2000);
  /* to solar system plane */
  swi_cartpol(x0, x0); 
  x0[0] -= SSY_PLANE_NODE_E2000;
  swi_polcart(x0, x0);
  swi_coortrf(x0, x0, SSY_PLANE_INCL);
  swi_cartpol(x0, x0);
  x0[0] += SSY_PLANE_NODE;
  x00 = x0[0] * RADTODEG;                       /* 7 */
  for (i = 1; i <= 12; i++)                     /* 6, 8, 9 */
    cusp[i] = swe_degnorm(cusp[i] - dvpxe - sip->ayan_t0 - x00);
  for (i = 0; i <= SE_NASCMC; i++)
    ascmc[i] = swe_degnorm(ascmc[i] - dvpxe - sip->ayan_t0 - x00);
  return retc;
}

/* common simplified procedure */
static int sidereal_houses_trad(double tjde, 
                           double armc, 
                           double eps, 
                           double nutl, 
                           double lat, 
			   int hsys, 
                           double *cusp, 
                           double *ascmc) 
{
  int i, retc = OK;
  double ay;
  retc = swe_houses_armc(armc, lat, eps, hsys, cusp, ascmc);
  ay = swe_get_ayanamsa(tjde);
  for (i = 1; i <= 12; i++)
    cusp[i] = swe_degnorm(cusp[i] - ay - nutl);
  for (i = 0; i < SE_NASCMC; i++) {
    if (i == 2)	/* armc */
      continue;
    ascmc[i] = swe_degnorm(ascmc[i] - ay - nutl);
  }
  return retc;
}

/* 
 * this function is required for very special computations
 * where no date is given for house calculation,
 * e.g. for composite charts or progressive charts.
 * cusps are returned in double cusp[13]:
 * cusp[1...12]	houses 1 - 12
 * additional points are returned in ascmc[10].
 * ascmc[0] = ascendant
 * ascmc[1] = mc
 * ascmc[2] = armc
 * ascmc[3] = vertex
 * ascmc[4] = equasc		* "equatorial ascendant" *
 * ascmc[5] = coasc1		* "co-ascendant" (W. Koch) *
 * ascmc[6] = coasc2		* "co-ascendant" (M. Munkasey) *
 * ascmc[7] = polasc		* "polar ascendant" (M. Munkasey) *
 */
int FAR PASCAL_CONV swe_houses_armc(
				double armc,
				double geolat,
				double eps,
				int hsys,
				double *cusp,
				double *ascmc)
{
  struct houses h;
  int i, retc = 0;
  retc = CalcH(armc,    
	       geolat,
	       eps, 
	       (char)hsys, 2, &h);
  cusp[0] = 0;
  cusp[1] = h.cusp[1];   /* cusp 1 */ 
  cusp[2] = h.cusp[2];   /* cusp 2 */ 
  cusp[3] = h.cusp[3];   /* cusp 3 */ 
  cusp[4] = h.cusp[4];   
  cusp[5] = h.cusp[5];  
  cusp[6] = h.cusp[6]; 
  cusp[7] = h.cusp[7];
  cusp[8] = h.cusp[8];
  cusp[9] = h.cusp[9];
  cusp[10] = h.cusp[10];
  cusp[11] = h.cusp[11];
  cusp[12] = h.cusp[12];
  ascmc[0] = h.ac;        /* Asc */    
  ascmc[1] = h.mc;        /* Mid */    
  ascmc[2] = armc;   
  ascmc[3] = h.vertex;
  ascmc[4] = h.equasc;
  ascmc[5] = h.coasc1;	/* "co-ascendant" (W. Koch) */
  ascmc[6] = h.coasc2;	/* "co-ascendant" (M. Munkasey) */
  ascmc[7] = h.polasc;	/* "polar ascendant" (M. Munkasey) */
  for (i = SE_NASCMC; i < 10; i++)
    ascmc[i] = 0;
#ifdef TRACE
  swi_open_trace(NULL);
  if (swi_trace_count <= TRACE_COUNT_MAX) {
    if (swi_fp_trace_c != NULL) {
      fputs("\n/*SWE_HOUSES_ARMC*/\n", swi_fp_trace_c);
      fprintf(swi_fp_trace_c, "  armc = %.9f;", armc);
      fprintf(swi_fp_trace_c, " geolat = %.9f;", geolat);
      fprintf(swi_fp_trace_c, " eps = %.9f;", eps);
      fprintf(swi_fp_trace_c, " hsys = %d;\n", hsys);
      fprintf(swi_fp_trace_c, "  retc = swe_houses_armc(armc, geolat, eps, hsys, cusp, ascmc);\n");
      fputs("  printf(\"swe_houses_armc: %f\\t%f\\t%f\\t%c\\t\\n\", ", swi_fp_trace_c);
      fputs("  armc, geolat, eps, hsys);\n", swi_fp_trace_c);
      fputs("  printf(\"retc = %d\\n\", retc);\n", swi_fp_trace_c);
      fputs("  printf(\"cusp:\\n\");\n", swi_fp_trace_c);
      fputs("  for (i = 0; i < 12; i++)\n", swi_fp_trace_c);
      fputs("    printf(\"  %d\\t%f\\n\", i, cusp[i]);\n", swi_fp_trace_c);
      fputs("  printf(\"ascmc:\\n\");\n", swi_fp_trace_c);
      fputs("  for (i = 0; i < 10; i++)\n", swi_fp_trace_c);
      fputs("    printf(\"  %d\\t%f\\n\", i, ascmc[i]);\n", swi_fp_trace_c);
      fflush(swi_fp_trace_c);
    }
    if (swi_fp_trace_out != NULL) {
      fprintf(swi_fp_trace_out, "swe_houses_armc: %f\t%f\t%f\t%c\t\n", armc, geolat, eps, hsys);
      fprintf(swi_fp_trace_out, "retc = %d\n", retc);
      fputs("cusp:\n", swi_fp_trace_out);
      for (i = 0; i < 12; i++)
	fprintf(swi_fp_trace_out, "  %d\t%f\n", i, cusp[i]);
      fputs("ascmc:\n", swi_fp_trace_out);
      for (i = 0; i < 10; i++)
	fprintf(swi_fp_trace_out, "  %d\t%f\n", i, ascmc[i]);
      fflush(swi_fp_trace_out);
    }
  }
#endif
#if 0 
/* for test of swe_house_pos(). 
 * 1st house will be 0, second 30, etc. */
for (i = 1; i <=12; i++) {
  double x[6];
  x[0] = cusp[i]; x[1] = 0; x[2] = 1;
  cusp[i] = (swe_house_pos(armc, geolat, eps, hsys, x, NULL) - 1) * 30;
}
#endif
  return retc;
}

static int CalcH(
	double th, double fi, double ekl, char hsy, 
	int iteration_count, struct houses *hsp )
/* ********************************************************* 
 *  Arguments: th = sidereal time (angle 0..360 degrees     
 *             hsy = letter code for house system;         
 *                   A  equal                         
 *                   E  equal                         
 *                   C  Campanus                        
 *                   H  horizon / azimut
 *                   K  Koch                             
 *                   O  Porphyry                          
 *                   P  Placidus                          
 *                   R  Regiomontanus                  
 *                   V  equal Vehlow                 
 *                   X  axial rotation system/ Meridian houses 
 *             fi = geographic latitude             
 *             ekl = obliquity of the ecliptic               
 *             iteration_count = number of iterations in    
 *             Placidus calculation; can be 1 or 2.        
 * ********************************************************* 
 *  Koch and Placidus don't work in the polar circle.        
 *  We swap MC/IC so that MC is always before AC in the zodiac 
 *  We than divide the quadrants into 3 equal parts.         
 * *********************************************************
 *  All angles are expressed in degrees.                     
 *  Special trigonometric functions sind, cosd etc. are 
 *  implemented for arguments in degrees.                 
 ***********************************************************/
{
  double tane, tanfi, cosfi, tant, sina, cosa, th2;
  double a, c, f, fh1, fh2, xh1, xh2, rectasc, ad3, acmc, vemc;
  int 	i, retc = OK;
  double sine, cose;
  cose  = cosd(ekl);
  sine  = sind(ekl);
  tane  = tand(ekl);
  /* north and south poles */
  if (fabs(fabs(fi) - 90) < VERY_SMALL) { 
    if (fi < 0)
      fi = -90 + VERY_SMALL;
    else
      fi = 90 - VERY_SMALL;
  } 
  tanfi = tand(fi);
  /* mc */
  if (fabs(th - 90) > VERY_SMALL
    && fabs(th - 270) > VERY_SMALL) {
    tant = tand(th);
    hsp->mc = atand(tant / cose);
    if (th > 90 && th <= 270) 
      hsp->mc = swe_degnorm(hsp->mc + 180);
  } else {
    if (fabs(th - 90) <= VERY_SMALL)
      hsp->mc = 90;
    else 
      hsp->mc = 270;
  } /*  if */
  hsp->mc = swe_degnorm(hsp->mc);
  /* ascendant */
  hsp->ac = Asc1 (th + 90, fi, sine, cose);
  hsp->cusp[1] = hsp->ac;
  hsp->cusp[10] = hsp->mc;
  if (hsy > 95) hsy = (char) (hsy - 32);/* translate into capital letter */
  switch (hsy) {
  case 'A':	/* equal houses */
  case 'E':
    /*
    * within polar circle we swap AC/DC if AC is on wrong side
    */
    acmc = swe_difdeg2n(hsp->ac, hsp->mc);
    if (acmc < 0) {
      hsp->ac = swe_degnorm(hsp->ac + 180);
      hsp->cusp[1] = hsp->ac;
    }
    for (i = 2; i <=12; i++)
      hsp->cusp [i] = swe_degnorm(hsp->cusp [1] + (i-1) * 30);
    break;
  case 'C': /* Campanus houses and Horizon or Azimut system */
  case 'H':
    if (hsy == 'H') {
      if (fi > 0)
        fi = 90 - fi;
      else
        fi = -90 - fi;
      /* equator */
      if (fabs(fabs(fi) - 90) < VERY_SMALL) { 
        if (fi < 0)
          fi = -90 + VERY_SMALL;
        else
          fi = 90 - VERY_SMALL;
      } 
      th = swe_degnorm(th + 180);
    }
    fh1 = asind(sind (fi) / 2);
    fh2 = asind(sqrt (3.0) / 2 * sind(fi));
    cosfi = cosd(fi);
    if (fabs(cosfi) == 0) {	/* '==' should be save! */ 
      if (fi > 0) 
	xh1 = xh2 = 90; /* cosfi = VERY_SMALL; */
      else
	xh1 = xh2 = 270; /* cosfi = -VERY_SMALL; */
    } else {
      xh1 = atand(sqrt (3.0) / cosfi);
      xh2 = atand(1 / sqrt (3.0) / cosfi);
    }
    hsp->cusp [11] = Asc1 (th + 90 - xh1, fh1, sine, cose);
    hsp->cusp [12] = Asc1 (th + 90 - xh2, fh2, sine, cose);
    if (hsy == 'H') 
      hsp->cusp [1] = Asc1 (th + 90, fi, sine, cose);
    hsp->cusp [2] = Asc1 (th + 90 + xh2, fh2, sine, cose);
    hsp->cusp [3] = Asc1 (th + 90 + xh1, fh1, sine, cose);
    /* within polar circle, when mc sinks below horizon and 
     * ascendant changes to western hemisphere, all cusps
     * must be added 180 degrees. 
     * houses will be in clockwise direction */
    if (fabs(fi) >= 90 - ekl) {  /* within polar circle */
      acmc = swe_difdeg2n(hsp->ac, hsp->mc);
      if (acmc < 0) {
        hsp->ac = swe_degnorm(hsp->ac + 180);
        hsp->mc = swe_degnorm(hsp->mc + 180);
	for (i = 1; i <= 12; i++)
	  hsp->cusp[i] = swe_degnorm(hsp->cusp[i] + 180);
      }
    }
    if (hsy == 'H') {
      for (i = 1; i <= 3; i++)
        hsp->cusp[i] = swe_degnorm(hsp->cusp[i] + 180);
      for (i = 11; i <= 12; i++)
        hsp->cusp[i] = swe_degnorm(hsp->cusp[i] + 180);
      /* restore fi and th */
      if (fi > 0)
        fi = 90 - fi;
      else
        fi = -90 - fi;
      th = swe_degnorm(th + 180);
    }
    break;
  case 'K': /* Koch houses */
    if (fabs(fi) >= 90 - ekl) {  /* within polar circle */
      retc = ERR;
      goto porphyry;
    } 
    sina = sind(hsp->mc) * sine / cosd(fi); 	/* always << 1, 
				      because fi < polar circle */
    cosa = sqrt(1 - sina * sina);		/* always >> 0 */
    c = atand(tanfi / cosa);
    ad3 = asind(sind(c) * sina) / 3.0;
    hsp->cusp [11] = Asc1 (th + 30 - 2 * ad3, fi, sine, cose);
    hsp->cusp [12] = Asc1 (th + 60 - ad3, fi, sine, cose);
    hsp->cusp [2] = Asc1 (th + 120 + ad3, fi, sine, cose);
    hsp->cusp [3] = Asc1 (th + 150 + 2 * ad3, fi, sine, cose);
    break;
  case 'O':	/* Porphyry houses */
porphyry:
    /*
     * within polar circle we swap AC/DC if AC is on wrong side
     */
    acmc = swe_difdeg2n(hsp->ac, hsp->mc);
    if (acmc < 0) {
     hsp->ac = swe_degnorm(hsp->ac + 180);
     hsp->cusp[1] = hsp->ac;
     acmc = swe_difdeg2n(hsp->ac, hsp->mc);
    }
    hsp->cusp [2] = swe_degnorm(hsp->ac + (180 - acmc) / 3);
    hsp->cusp [3] = swe_degnorm(hsp->ac + (180 - acmc) / 3 * 2);
    hsp->cusp [11] = swe_degnorm(hsp->mc + acmc / 3);
    hsp->cusp [12] = swe_degnorm(hsp->mc + acmc / 3 * 2);
    break;
  case 'R':	/* Regiomontanus houses */
    fh1 = atand (tanfi * 0.5);
    fh2 = atand (tanfi * cosd(30));
    hsp->cusp [11] =  Asc1 (30 + th, fh1, sine, cose); 
    hsp->cusp [12] =  Asc1 (60 + th, fh2, sine, cose); 
    hsp->cusp [2] =  Asc1 (120 + th, fh2, sine, cose); 
    hsp->cusp [3] =  Asc1 (150 + th, fh1, sine, cose); 
    /* within polar circle, when mc sinks below horizon and 
     * ascendant changes to western hemisphere, all cusps
     * must be added 180 degrees.
     * houses will be in clockwise direction */
    if (fabs(fi) >= 90 - ekl) {  /* within polar circle */
      acmc = swe_difdeg2n(hsp->ac, hsp->mc);
      if (acmc < 0) {
        hsp->ac = swe_degnorm(hsp->ac + 180);
        hsp->mc = swe_degnorm(hsp->mc + 180);
	for (i = 1; i <= 12; i++)
	  hsp->cusp[i] = swe_degnorm(hsp->cusp[i] + 180);
      }
    }
    break;
  case 'T':	/* 'topocentric' houses */
    fh1 = atand (tanfi / 3.0);
    fh2 = atand (tanfi * 2.0 / 3.0);
    hsp->cusp [11] =  Asc1 (30 + th, fh1, sine, cose); 
    hsp->cusp [12] =  Asc1 (60 + th, fh2, sine, cose); 
    hsp->cusp [2] =  Asc1 (120 + th, fh2, sine, cose); 
    hsp->cusp [3] =  Asc1 (150 + th, fh1, sine, cose); 
    /* within polar circle, when mc sinks below horizon and 
     * ascendant changes to western hemisphere, all cusps
     * must be added 180 degrees.
     * houses will be in clockwise direction */
    if (fabs(fi) >= 90 - ekl) {  /* within polar circle */
      acmc = swe_difdeg2n(hsp->ac, hsp->mc);
      if (acmc < 0) {
        hsp->ac = swe_degnorm(hsp->ac + 180);
        hsp->mc = swe_degnorm(hsp->mc + 180);
	for (i = 1; i <= 12; i++)
	  hsp->cusp[i] = swe_degnorm(hsp->cusp[i] + 180);
      }
    }
    break;
  case 'V':	/* equal houses after Vehlow */
    /*
    * within polar circle we swap AC/DC if AC is on wrong side
    */
    acmc = swe_difdeg2n(hsp->ac, hsp->mc);
    if (acmc < 0) {
      hsp->ac = swe_degnorm(hsp->ac + 180);
      hsp->cusp[1] = hsp->ac;
    }
    hsp->cusp [1] = swe_degnorm(hsp->ac - 15);
    for (i = 2; i <=12; i++)
      hsp->cusp [i] = swe_degnorm(hsp->cusp [1] + (i-1) * 30);
    break;
  case 'X': {
    /* 
     * Meridian or axial rotation system:
     * ecliptic points whose rectascensions
     * are armc + n * 30 
     */
    int j;
    double a = th;
    for (i = 1; i <= 12; i++) {
      j = i + 10;
      if (j > 12) j -= 12;
      a = swe_degnorm(a + 30);
      if (fabs(a - 90) > VERY_SMALL
        && fabs(a - 270) > VERY_SMALL) {
        tant = tand(a);
        hsp->cusp[j] = atand(tant / cose);
        if (a > 90 && a <= 270) 
          hsp->cusp[j] = swe_degnorm(hsp->cusp[j] + 180);
      } else {
        if (fabs(a - 90) <= VERY_SMALL)
          hsp->cusp[j] = 90;
        else 
          hsp->cusp[j] = 270;
      } /*  if */
      hsp->cusp[j] = swe_degnorm(hsp->cusp[j]);
    }
    }
    break;
  case 'B': {	/* Alcabitius */
    /* created by Alois 17-sep-2000, followed example in Matrix
       electrical library. The code reproduces the example!
       I think the Alcabitius code in Walter Pullen's Astrolog 5.40
       is wrong, because he remains in RA and forgets the transform to
       the ecliptic. */
    double dek, r, sna, sda, sn3, sd3;
#if FALSE
    if (fabs(fi) >= 90 - ekl) {  /* within polar circle */
      retc = ERR;
      goto porphyry;
    } 
#endif
    acmc = swe_difdeg2n(hsp->ac, hsp->mc);
    if (acmc < 0) {
     hsp->ac = swe_degnorm(hsp->ac + 180);
     hsp->cusp[1] = hsp->ac;
     acmc = swe_difdeg2n(hsp->ac, hsp->mc);
    }
    dek = asind(sind(hsp->ac) * sine);	/* declination of Ascendant */
    /* must treat the case fi == 90 or -90 */
    r = -tanfi * tand(dek);	
    /* must treat the case of abs(r) > 1; probably does not happen
     * because dek becomes smaller when fi is large, as ac is close to
     * zero Aries/Libra in that case.
     */
    sda = acos(r) * RADTODEG;	/* semidiurnal arc, measured on equator */
    sna = 180 - sda;		/* complement, seminocturnal arc */
    sd3 = sda / 3;
    sn3 = sna / 3;
    rectasc = swe_degnorm(th + sd3);	/* cusp 11 */
    hsp->cusp [11] = Asc1 (rectasc, 0, sine, cose);
    rectasc = swe_degnorm(th + 2 * sd3);	/* cusp 12 */
    hsp->cusp [12] = Asc1 (rectasc, 0, sine, cose);
    rectasc = swe_degnorm(th + 180 - 2 * sn3);	/* cusp 2 */
    hsp->cusp [2] = Asc1 (rectasc, 0, sine, cose);
    rectasc = swe_degnorm(th + 180 -  sn3);	/* cusp 3 */
    hsp->cusp [3] = Asc1 (rectasc, 0, sine, cose);
    }
    break;
  default:	/* Placidus houses */
#ifndef _WINDOWS
    if (hsy != 'P') 
      fprintf (stderr, "swe_houses: make Placidus, unknown key %c\n", hsy);
#endif
    if (fabs(fi) >= 90 - ekl) {  /* within polar circle */
      retc = ERR;
      goto porphyry;
    } 
    a = asind(tand(fi) * tane);
    fh1 = atand(sind(a / 3) / tane);
    fh2 = atand(sind(a * 2 / 3) / tane);
    /* ************  house 11 ******************** */
    rectasc = swe_degnorm(30 + th);
    tant = tand(asind(sine * sind(Asc1 (rectasc, fh1, sine, cose))));
    if (fabs(tant) < VERY_SMALL) {
      hsp->cusp [11] = rectasc;
    } else {
      /* pole height */
      f = atand(sind(asind(tanfi * tant) / 3)  /tant);  
      hsp->cusp [11] = Asc1 (rectasc, f, sine, cose);
      for (i = 1; i <= iteration_count; i++) {
	tant = tand(asind(sine * sind(hsp->cusp [11])));
	if (fabs(tant) < VERY_SMALL) {
	  hsp->cusp [11] = rectasc;
	  break;
	}
	/* pole height */
	f = atand(sind(asind(tanfi * tant) / 3) / tant);
	hsp->cusp [11] = Asc1 (rectasc, f, sine, cose);
      }
    }
    /* ************  house 12 ******************** */
    rectasc = swe_degnorm(60 + th);
    tant = tand(asind(sine*sind(Asc1 (rectasc,  fh2, sine, cose))));
    if (fabs(tant) < VERY_SMALL) {
      hsp->cusp [12] = rectasc;
    } else {
      f = atand(sind(asind(tanfi * tant) / 1.5) / tant);  
      /*  pole height */
      hsp->cusp [12] = Asc1 (rectasc, f, sine, cose);
      for (i = 1; i <= iteration_count; i++) {
	tant = tand(asind(sine * sind(hsp->cusp [12])));
	if (fabs(tant) < VERY_SMALL) {
	  hsp->cusp [12] = rectasc;
	  break;
	}
	f = atand(sind(asind(tanfi * tant) / 1.5) / tant);  
	/*  pole height */
	hsp->cusp [12] = Asc1 (rectasc, f, sine, cose);
      }
    }
    /* ************  house  2 ******************** */
    rectasc = swe_degnorm(120 + th);
    tant = tand(asind(sine * sind(Asc1 (rectasc, fh2, sine, cose))));
    if (fabs(tant) < VERY_SMALL) {
      hsp->cusp [2] = rectasc;
    } else {
      f = atand(sind(asind(tanfi * tant) / 1.5) / tant);
      /*  pole height */
      hsp->cusp [2] = Asc1 (rectasc, f, sine, cose);
      for (i = 1; i <= iteration_count; i++) {
	tant = tand(asind(sine * sind(hsp->cusp [2])));
	if (fabs(tant) < VERY_SMALL) {
	  hsp->cusp [2] = rectasc;
	  break;
	}
	f = atand(sind(asind(tanfi * tant) / 1.5) / tant);
	/*  pole height */
	hsp->cusp [2] = Asc1 (rectasc, f, sine, cose);
      }
    }
    /* ************  house  3 ******************** */
    rectasc = swe_degnorm(150 + th);
    tant = tand(asind(sine * sind(Asc1 (rectasc, fh1, sine, cose))));
    if (fabs(tant) < VERY_SMALL) {
      hsp->cusp [3] = rectasc;
    } else {
      f = atand(sind(asind(tanfi * tant) / 3) / tant);  
      /*  pole height */
      hsp->cusp [3] = Asc1(rectasc, f, sine, cose);
      for (i = 1; i <= iteration_count; i++) {
	tant = tand(asind(sine * sind(hsp->cusp [3])));
	if (fabs(tant) < VERY_SMALL) {
	  hsp->cusp [3] = rectasc;
	  break;
	}
	f = atand(sind(asind(tanfi * tant) / 3) / tant);
	/*  pole height */
	hsp->cusp [3] = Asc1 (rectasc, f, sine, cose);
      }
    }
    break;
  } /* end switch */
  hsp->cusp [4] = swe_degnorm(hsp->cusp [10] + 180);
  hsp->cusp [5] = swe_degnorm(hsp->cusp [11] + 180);
  hsp->cusp [6] = swe_degnorm(hsp->cusp [12] + 180);
  hsp->cusp [7] = swe_degnorm(hsp->cusp [1] + 180);
  hsp->cusp [8] = swe_degnorm(hsp->cusp [2] + 180);
  hsp->cusp [9] = swe_degnorm(hsp->cusp [3] + 180);
  /* vertex */
  if (fi >= 0)
    f = 90 - fi;
  else
    f = -90 - fi;
  hsp->vertex = Asc1 (th - 90, f, sine, cose);
  /* with tropical latitudes, the vertex behaves strange, 
   * in a similar way as the ascendant within the polar 
   * circle. we keep it always on the western hemisphere.*/
  if (fabs(fi) <= ekl) {
    vemc = swe_difdeg2n(hsp->vertex, hsp->mc);
    if (vemc > 0)
      hsp->vertex = swe_degnorm(hsp->vertex + 180);
  }
  /* 
   * some strange points:
   */
  /* equasc (equatorial ascendant) */
  th2 = swe_degnorm(th + 90);
  if (fabs(th2 - 90) > VERY_SMALL
    && fabs(th2 - 270) > VERY_SMALL) {
    tant = tand(th2);
    hsp->equasc = atand(tant / cose);
    if (th2 > 90 && th2 <= 270)
      hsp->equasc = swe_degnorm(hsp->equasc + 180);
  } else {
    if (fabs(th2 - 90) <= VERY_SMALL)
      hsp->equasc = 90;
    else
      hsp->equasc = 270;
  } /*  if */
  hsp->equasc = swe_degnorm(hsp->equasc);
  /* "co-ascendant" W. Koch */
  hsp->coasc1 = swe_degnorm(Asc1 (th - 90, fi, sine, cose) + 180);
  /* "co-ascendant" M. Munkasey */
  if (fi >= 0)
    hsp->coasc2 = Asc1 (th + 90, 90 - fi, sine, cose);
  else /* southern hemisphere */
    hsp->coasc2 = Asc1 (th + 90, -90 - fi, sine, cose);
  /* "polar ascendant" M. Munkasey */
  hsp->polasc = Asc1 (th - 90, fi, sine, cose);
  return retc;
} /* procedure houses */

/******************************/
static double Asc1 (double x1, double f, double sine, double cose) 
{ 
  int n;
  double ass;
  x1 = swe_degnorm(x1);
  n  = (int) ((x1 / 90) + 1);
  if (n == 1)
    ass = ( Asc2 (x1, f, sine, cose));
  else if (n == 2) 
    ass = (180 - Asc2 (180 - x1, - f, sine, cose));
  else if (n == 3)
    ass = (180 + Asc2 (x1 - 180, - f, sine, cose));
  else
    ass = (360 - Asc2 (360- x1,  f, sine, cose));
  ass = swe_degnorm(ass);
  if (fabs(ass - 90) < VERY_SMALL)	/* rounding, e.g.: if */
    ass = 90;				/* fi = 0 & st = 0, ac = 89.999... */
  if (fabs(ass - 180) < VERY_SMALL)
    ass = 180;
  if (fabs(ass - 270) < VERY_SMALL)	/* rounding, e.g.: if */
    ass = 270;				/* fi = 0 & st = 0, ac = 89.999... */
  if (fabs(ass - 360) < VERY_SMALL)
    ass = 0;
  return ass;
}  /* Asc1 */

static double Asc2 (double x, double f, double sine, double cose) 
{
  double ass, sinx;
  ass = - tand(f) * sine + cose * cosd(x);
  if (fabs(ass) < VERY_SMALL)
    ass = 0;
  sinx = sind(x);
  if (fabs(sinx) < VERY_SMALL)
    sinx = 0;
  if (sinx == 0) {
    if (ass < 0)
      ass = -VERY_SMALL;
    else
      ass = VERY_SMALL;
  } else if (ass == 0) {
    if (sinx < 0)
      ass = -90;
    else
      ass = 90;
  } else {
    ass = atand(sinx / ass);
  }
  if (ass < 0)
    ass = 180 + ass;
  return (ass);
} /* Asc2 */


/* computes the house position of a planet or another point,
 * in degrees: 0 - 30 = 1st house, 30 - 60 = 2nd house, etc.
 * armc 	sidereal time in degrees
 * geolat	geographic latitude
 * eps		true ecliptic obliquity
 * hsys		house system character
 * xpin		array of 6 doubles: 
 * 		only the first two of them are used: ecl. long., lat.
 * serr		error message area
 *
 * house position is returned by function.
 * 
 * sidereal house positions:
 *
 * tropical and sidereal house positions of planets are always identical 
 * if the traditional method of computing sidereal positions (subtracting
 * the ayanamsha from tropical in order to get sidereal positions) is used.
 *
 * if the sidereal plane is not identical to the ecliptic of date, 
 * sidereal and tropical house positions are identical with
 * house systems that are independent on the ecliptic such as:
 * - Campanus
 * - Regiomontanus
 * - Placidus
 * - Azimuth/Horizon
 * - Axial rotation system
 * - "topocentric" system
 * 
 * in all these cases call swe_house_pos() with TROPICAL planetary positions.
 *
 * but there are different house positions for ecliptic-dependent systems
 * such as:
 * - equal
 * - Porphyry
 * - Koch
 * 
 * for these cases there is no function. 
 */
double FAR PASCAL_CONV swe_house_pos(
	double armc, double geolat, double eps, int hsys, double *xpin, char *serr)
{
  double xp[6], xeq[6], ra, de, mdd, mdn, sad, san;
  double hpos, sinad, ad, a, admc, adp, samc, demc, asc, mc, acmc, tant;
  double fh, ra0, tanfi, fac;
  double sine = sind(eps);
  double cose = cosd(eps);
  AS_BOOL is_above_hor = FALSE;
  AS_BOOL clockwise = FALSE;
  hsys = toupper(hsys);
  xeq[0] = xpin[0];
  xeq[1] = xpin[1];
  xeq[2] = 1;
  swe_cotrans(xpin, xeq, -eps);	
  ra = xeq[0];
  de = xeq[1];
  mdd = swe_degnorm(ra - armc);
  mdn = swe_degnorm(mdd + 180);
  if (mdd >= 180)
    mdd -= 360;
  if (mdn >= 180)
    mdn -= 360;
  /* xp[0] will contain the house position, a value between 0 and 360 */
  switch(hsys) {
    case 'A':
    case 'E':
    case 'V':
      asc = Asc1 (swe_degnorm(armc + 90), geolat, sine, cose);
      demc = atand(sind(armc) * tand(eps));
      if (geolat >= 0 && 90 - geolat + demc < 0)
	asc = swe_degnorm(asc + 180);
      if (geolat < 0 && -90 - geolat + demc > 0)
	asc = swe_degnorm(asc + 180);
      xp[0] = swe_degnorm(xpin[0] - asc);
      if (hsys == 'V') 
	xp[0] = swe_degnorm(xp[0] + 15);
      /* to make sure that a call with a house cusp position returns
       * a value within the house, 0.001" is added */
      xp[0] = swe_degnorm(xp[0] + MILLIARCSEC);
      hpos = xp[0] / 30.0 + 1;
    break;
    case 'O':
      asc = Asc1 (swe_degnorm(armc + 90), geolat, sine, cose);
      demc = atand(sind(armc) * tand(eps));
      /* mc */
      if (fabs(armc - 90) > VERY_SMALL
	      && fabs(armc - 270) > VERY_SMALL) {
	tant = tand(armc);
	mc = swe_degnorm(atand(tant / cose));
	if (armc > 90 && armc <= 270) 
	  mc = swe_degnorm(mc + 180);
      } else {
	if (fabs(armc - 90) <= VERY_SMALL)
	  mc = 90;
	else 
	  mc = 270;
      }
      /* while MC is always south, 
       * Asc must always be in eastern hemisphere */
      if (geolat >= 0 && 90 - geolat + demc < 0) {
	asc = swe_degnorm(asc + 180);
      }
      if (geolat < 0 && -90 - geolat + demc > 0) {
	asc = swe_degnorm(asc + 180);
      }
      xp[0] = swe_degnorm(xpin[0] - asc);
      /* to make sure that a call with a house cusp position returns
       * a value within the house, 0.001" is added */
      xp[0] = swe_degnorm(xp[0] + MILLIARCSEC);
      if (xp[0] < 180)
	hpos = 1;
      else {
	hpos = 7;
	xp[0] -= 180;
      }
      acmc = swe_difdeg2n(asc, mc);
      if (xp[0] < 180 - acmc)
	hpos += xp[0] * 3 / (180 - acmc);
      else
	hpos += 3 + (xp[0] - 180 + acmc) * 3 / acmc;
    break;
    case 'X': /* Merdidian or axial rotation system */
      hpos = swe_degnorm(mdd - 90) / 30.0 + 1;
    break;
    case 'K':
     demc = atand(sind(armc) * tand(eps));
     /* if body is within circumpolar region, error */
     if (90 - fabs(geolat) <= fabs(de)) {
       if (serr != NULL)
         strcpy(serr, "no Koch house position, because planet is circumpolar.");
       xp[0] = 0;
       hpos = 0;	/* Error */
     } else if (90 - fabs(geolat) <= fabs(demc)) {
       if (serr != NULL)
         strcpy(serr, "no Koch house position, because mc is circumpolar.");
       xp[0] = 0;
       hpos = 0;	/* Error */
#if 0
     /* if geolat beyond polar circle, error */
       || (fabs(geolat) >= 90 - eps)) 
#endif
      } else {
        admc = asind(tand(eps) * tand(geolat) * sind(armc));
        adp = asind(tand(geolat) * tand(de));
          samc = 90 + admc;
        if (mdd >= 0) {	/* east */
          xp[0] = swe_degnorm(((mdd - adp + admc) / samc - 1) * 90);
        } else {
          xp[0] = swe_degnorm(((mdd + 180 + adp + admc) / samc + 1) * 90);
        }
	/* to make sure that a call with a house cusp position returns
	 * a value within the house, 0.001" is added */
	xp[0] = swe_degnorm(xp[0] + MILLIARCSEC);
	hpos = xp[0] / 30.0 + 1;
      }
      break;
    case 'C':
      xeq[0] = swe_degnorm(mdd - 90);
      swe_cotrans(xeq, xp, -geolat);
      /* to make sure that a call with a house cusp position returns
       * a value within the house, 0.001" is added */
      xp[0] = swe_degnorm(xp[0] + MILLIARCSEC);
      hpos = xp[0] / 30.0 + 1;
      break;
    case 'H':
      xeq[0] = swe_degnorm(mdd - 90);
      swe_cotrans(xeq, xp, 90 - geolat);
      /* to make sure that a call with a house cusp position returns
       * a value within the house, 0.001" is added */
      xp[0] = swe_degnorm(xp[0] + MILLIARCSEC);
      hpos = xp[0] / 30.0 + 1;
      break;
    case 'R':
      if (fabs(mdd) < VERY_SMALL)
        xp[0] = 270; 
      else if (180 - fabs(mdd) < VERY_SMALL)
        xp[0] = 90; 
      else {
        if (90 - fabs(geolat) < VERY_SMALL) {
          if (geolat > 0)
            geolat = 90 - VERY_SMALL;
          else
            geolat = -90 + VERY_SMALL;
        }
        if (90 - fabs(de) < VERY_SMALL) {
          if (de > 0)
            de = 90 - VERY_SMALL;
          else
            de = -90 + VERY_SMALL;
        }
        a = tand(geolat) * tand(de) + cosd(mdd);
        xp[0] = swe_degnorm(atand(-a / sind(mdd)));
        if (mdd < 0)
          xp[0] += 180;
        xp[0] = swe_degnorm(xp[0]);
	/* to make sure that a call with a house cusp position returns
	 * a value within the house, 0.001" is added */
        xp[0] = swe_degnorm(xp[0] + MILLIARCSEC);
      }  
      hpos = xp[0] / 30.0 + 1;
      break;
    case 'T':
	  mdd = swe_degnorm(mdd);
      if (de > 90 - VERY_SMALL)
	    de = 90 - VERY_SMALL;
	  if (de < -90 + VERY_SMALL)
	    de = -90 + VERY_SMALL;
      sinad = tand(de) * tand(geolat);
      ad = asind(sinad);
      a = sinad + cosd(mdd);
      if (a >= 0)
        is_above_hor = TRUE;
	  /* mirror everything below the horizon to the opposite point
	   * above the horizon */
	  if (!is_above_hor) {
	    ra = swe_degnorm(ra + 180);
	    de = -de;
	    mdd = swe_degnorm(mdd + 180);
	  }
      /* mirror everything on western hemisphere to eastern hemisphere */  
	  if (mdd > 180) {
		ra = swe_degnorm(armc - mdd);
	  }
	  /* binary search for "topocentric" position line of body */
      tanfi = tand(geolat);
	  fh = geolat;
	  ra0 = swe_degnorm(armc + 90);
	  xp[1] = 1;
	  xeq[1] = de;
	  fac = 2;
      while (fabs(xp[1]) > 0.000001) {
		if (xp[1] > 0) {
		  fh = atand(tand(fh) - tanfi / fac);
		  ra0 -= 90 / fac;
		} else {
		  fh = atand(tand(fh) + tanfi / fac);
		  ra0 += 90 / fac;
		}
		xeq[0] = swe_degnorm(ra - ra0);
        swe_cotrans(xeq, xp, 90 - fh);
		fac *= 2;
	  }
      hpos = swe_degnorm(ra0 - armc);
	  /* mirror back to west */
	  if (mdd > 180)
		hpos = swe_degnorm(-hpos);
	  /* mirror back to below horizon */
	  if (!is_above_hor)
	    hpos = swe_degnorm(hpos + 180);
	  hpos = swe_degnorm(hpos - 90) / 30 + 1;
	  break;
    case 'P':
    default:
       /* circumpolar region */
      if (90 - fabs(de) <= fabs(geolat)) {
        if (de * geolat < 0)  
          xp[0] = swe_degnorm(90 + mdn / 2);
        else
          xp[0] = swe_degnorm(270 + mdd / 2);
        if (serr != NULL)
          strcpy(serr, "Otto Ludwig procedure within circumpolar regions.");
      } else {
        sinad = tand(de) * tand(geolat);
        ad = asind(sinad);
        a = sinad + cosd(mdd);
        if (a >= 0)
          is_above_hor = TRUE;
        sad = 90 + ad;
        san = 90 - ad;
        if (is_above_hor)
          xp[0] =  (mdd / sad + 3) * 90;
        else
          xp[0] = (mdn / san + 1) * 90;
	/* to make sure that a call with a house cusp position returns
	 * a value within the house, 0.001" is added */
        xp[0] = swe_degnorm(xp[0] + MILLIARCSEC);
      }
      hpos = xp[0] / 30.0 + 1;
    break;
  }
  return hpos;
}
