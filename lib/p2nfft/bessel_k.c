/*
 * Copyright (C) 2013 Michael Pippig
 *
 * This file is part of ScaFaCoS.
 * 
 * ScaFaCoS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ScaFaCoS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *	
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/* Compute the incomplete Bessel-K function of order nu according to the paper
   Richard M. Slevinsky and Hassan Safouhi. 2010.
   A recursive algorithm for the G transformation and accurate computation of incomplete Bessel functions.
   Appl. Numer. Math. 60, 12 (December 2010), 1411-1417.
   http://dx.doi.org/10.1016/j.apnum.2010.04.005 
*/

#include <math.h>
#include <stdio.h>
#include <gsl/gsl_sf_bessel.h>
#include <gsl/gsl_sf_gamma.h>
#include <gsl/gsl_sf_lambert.h>

#include "bessel_k.h"

/**************************************************************************/
/* inc. upper Bessel_K function, variant 1, see [Slavinsky, Safouhi 2010] */
/**************************************************************************/

// /* General recursion formula for coefficients of the G transform.
//  * We use this function for debugging only. Otherwise we precompute all coefficients to circumvent
//  * repetitive calculation of the same coefficients. */
// static fcs_float A(
//     fcs_float nu, fcs_int i, fcs_int k
//     )
// {
//   if(k<i)
//     fprintf(stderr, "Error in computation of G transform coefficients A(i,k): k < i is not valid\n");
// 
//   if(i==k){
//     return 1;
//   } else if(i==0) {
//     return (-nu + k-1) * A(nu, 0, k-1);
//   } else {
//     return (-nu + i + k-1) * A(nu, i, k-1) + A(nu, i-1, k-1);
//   }
// }
// 
// static fcs_float D_tilde(
//     fcs_int n, fcs_float x, fcs_float y, fcs_float nu,
//     fcs_int N, fcs_float *pt, fcs_float *D_Aki
//     )
// {
//   fcs_float sum1, sum2;
// 
//   sum1 = 0.0;
//   for(fcs_int r=0; r<=n; r++){
//     sum2 = 0.0;
//     for(fcs_int i=0; i<=r; i++)
//       sum2 += D_Aki[i+(r*(r+1))/2] * fcs_pow(x,i);
//     sum1 += pt[r + (n*(n+1))/2] * fcs_pow(-y,-r) * sum2;
//   }
//   return fcs_pow(-x*y,n) * fcs_pow(x,nu+1) * fcs_exp(x+y) * sum1;
// }
// 
// static fcs_float N_tilde(
//     fcs_int n, fcs_float x, fcs_float y, fcs_float nu,
//     fcs_int N, fcs_float *pt, fcs_float *D_Aki, fcs_float *N_Aki
//     )
// {
//   fcs_float sum1, sum2, sum3;
// 
//   sum1 = 0.0;
//   for(fcs_int r=1; r<=n; r++){
//     sum2 = 0.0;
//     for(fcs_int s=0; s<=r-1; s++){
//       sum3 = 0.0;
//       for(fcs_int i=0; i<=s; i++)
//         sum3 += N_Aki[i+(s*(s+1))/2] * fcs_pow(-x,i);
//       sum2 += pt[s + ((r-1)*r)/2] * fcs_pow(y,-s) * sum3;
//     }
//     sum1 += pt[r+ (n*(n+1))/2] * D_tilde(n-r,x,y,nu,N,pt,D_Aki) * fcs_pow(x*y,r) * sum2;
//   }
// 
//   return fcs_exp(-x-y)/(fcs_pow(x,nu)*y) * sum1;
// }
// 
// static fcs_float G_tilde(
//     fcs_int n, fcs_float x, fcs_float y, fcs_float nu,
//     fcs_int N, fcs_float *pt, fcs_float *D_Aki, fcs_float *N_Aki
//     )
// {
//   return fcs_pow(x,nu) * (N_tilde(n,x,y,nu,N,pt,D_Aki,N_Aki) / D_tilde(n,x,y,nu,N,pt,D_Aki));
// } 
// 
// /* compute next line of recurrence coefficients */
// static void precompute_Aki(
//     fcs_float nu, fcs_int k, fcs_int N, fcs_float *Aki
//     )
// {
//   fcs_float *Ai_cur = &Aki[(k*(k+1))/2];
//   fcs_float *Ai_pre = &Aki[((k-1)*k)/2];
// 
//   Ai_cur[0] = (k-1-nu)*Ai_pre[0]; 
//   for(fcs_int i=1; i<k; i++)
//     Ai_cur[i] = (i+k-1-nu)*Ai_pre[i] + Ai_pre[i-1];
//   Ai_cur[k] = 1;
// }
// 
// /* compute next line of recurrence coefficients */
// static void precompute_pascals_triangle(
//     fcs_int n, fcs_int N, fcs_float *pt
//     )
// {
//   fcs_float *pt_cur = &pt[(n*(n+1))/2];
//   fcs_float *pt_pre = &pt[((n-1)*n)/2];
// 
//   pt_cur[0] = 1;
//   for(fcs_int k=1; k<n; k++)
//     pt_cur[k] = pt_pre[k-1] + pt_pre[k]; 
//   pt_cur[n] = 1;
// }
// 
// fcs_float ifcs_p2nfft_inc_upper_bessel_k(
//     fcs_float nu, fcs_float x, fcs_float y, fcs_float eps
//     )
// {
//   fcs_int n = 1, n_max = 127;
//   fcs_float err = 1.0, val_new, val_old;
//   fcs_float *pt, *D_Aki, *N_Aki;
//   fcs_int N = n_max+1;
// 
//   /* Fct. gsl_sf_gamma_inc aborts for large arguments due to underflows.
//    * Therefore, we return 0.0 if one of the following upper bounds is already very small. */
//   if(-21 <= nu && nu <= 21){
//     /* For -21 <= nu <= 21 we used Mathematica to find x large enough such that the upper bound
//      *   K_nu(x,y) <= x^nu * Gamma(-nu,x)
//      * is very small */
//     // if( x > 225 ) return 0.0; /* x^nu * Gamma(-nu,x) < 1e-100 */
//     if( x > 111 ) return 0.0; /* x^nu * Gamma(-nu,x) < 1e-50 */
//      
//     /* For -21 <= nu <= 21, x<=y we used Mathematica to find x large enough such that the upper bound
//      *   K_nu(x,y) <=  2*K_nu(2*Sqrt(xy))
//      * based on formula (4) of [Slevinsky-Safouhi 2010], is very small. */
//     // if(x<y) if( x*y > 115.0*115.0 ) return 0.0; /*  2*K_nu(2*Sqrt(xy)) < 1e-100 */
//     if(x<y) if( x*y > 58.0*58.0 ) return 0.0; /*  2*K_nu(2*Sqrt(xy)) < 1e-50 */
//   } else {
//     const fcs_float bound = 1e-50;
// 
//     if(nu >= -1){
//     /* For arbitrary nu > -1 we solve the very crude upper bound
//        inc_upper_bessel(nu,x,y) < Exp[-x]/x for x. */
//       if( x > gsl_sf_lambert_W0(1/bound) ) return 0.0;
//     } else {
//       /* For nu < -1 we use the bounds 
//          inc_upper_bessel(nu,x,y) <= exp(1-x)*(-nu-1)!*x^nu for x<=1
//          inc_upper_bessel(nu,x,y) <= exp(1-x)*(-nu-1)!*x^(-1) for x>1 */
//       fcs_float fak = 1.0;
//       for(fcs_int t=1; t<-nu; t++) 
//         fak *= t;
//       
//       if( fak*fcs_exp(1-x)*fcs_pow(x,-1) < bound ) return 0.0;
//     }
//   }
// 
//   /* for y==0 incompl. bessel_k can be computed using incompl. Gamma fct. */
//   if(fcs_float_is_zero(y))
//     return fcs_pow(x,nu) * gsl_sf_gamma_inc(-nu,x); 
// 
//   /* for x<y compute the faster convergent complement integral,
//    * see formula (4) of [Slevinsky-Safouhi 2010] */
//   if(x<y)
//     return 2 * fcs_pow(x/y, nu/2) * ifcs_p2nfft_bessel_k(nu, 2*fcs_sqrt(x*y)) - ifcs_p2nfft_inc_upper_bessel_k(-nu, y, x, eps);
//   
//   /* for nu=0 and x,y small use Taylor approximation */
//   if(fcs_float_is_zero(nu)){
//     if(fcs_pow(x,2) + fcs_pow(y,2) < fcs_pow(0.75,2)){
//       fcs_int k = 0;
//       fcs_float fak = 1.0;
//       fcs_float z = 0.0;
//       while( fcs_exp(-x)*fcs_pow(y,k+1)/(x*(k+1)*fak) > eps){
// 	z+=fcs_pow(-1,k)*fcs_pow(x*y,k)*gsl_sf_gamma_inc(-k,x)/fak;
// 	k+=1;
// 	fak*=k;
//       }
//       return z;
//     }
//   }
// 
//   /* init recurrence coefficients  and Pascal's triangle*/
//   D_Aki = malloc(sizeof(fcs_float)*(N*(N+1))/2);
//   N_Aki = malloc(sizeof(fcs_float)*(N*(N+1))/2);
//   pt = malloc(sizeof(fcs_float)*(N*(N+1))/2);
// 
//   /* compute first two lines of recurrence coeff. and Pascal's triangle */
//   for(fcs_int t=0; t<2; t++){
//     precompute_Aki(-nu-1, t, N, D_Aki);
//     precompute_Aki(nu-1, t, N, N_Aki);
//     precompute_pascals_triangle(t, N, pt);
//   }
// 
//   val_new = G_tilde(n,x,y,nu,N,pt,D_Aki,N_Aki);
//   while(err > eps){
// 
//     /* avoid overflow by division with very small numbers */
//     if(fabs(val_new) < eps)
//       break;
// 
//     if(n >= n_max){
//       //fprintf(stderr, "Inc_Bessel_K: Cannot reach accuracy within %" FCS_LMOD_INT "d iterations: val_new = %e, val_old = %e, err = %e, eps = %e, nu = %e, x = %e, y = %e.\n", n_max, val_new, val_old, err, eps, nu, x, y);
//       break;
//     }
//     n++;
// 
//     /* compute next line of recurrence coefficients */
//     precompute_Aki(-nu-1, n, N, D_Aki);
//     precompute_Aki(nu-1, n, N, N_Aki);
// 
//     /* compute next line of Pascal's triangle */
//     precompute_pascals_triangle(n, N, pt);
// 
//     val_old = val_new;
//     val_new = G_tilde(n,x,y,nu,N,pt,D_Aki,N_Aki);
//     err = fabs(val_new - val_old);
// 
//     if(isnan(val_new)){
//       //fprintf(stderr, "Inc_Bessel_K: NAN at iteration %" FCS_LMOD_INT "d: val_new = %e, val_old = %e, err = %e, eps = %e, nu = %e, x = %e, y = %e.\n", n, val_new, val_old, err, eps, nu, x, y);
//       val_new=val_old;
//       break;
//     }
// 
//     if(fcs_float_is_zero(val_new)){
//       //fprintf(stderr, "Inc_Bessel_K: value 0 at iteration %" FCS_LMOD_INT "d: val_new = %e, val_old = %e, err = %e, eps = %e, nu = %e, x = %e, y = %e.\n", n, val_new, val_old, err, eps, nu, x, y);
//       val_new=val_old;
//       break;
//     }
// 
//     if(isinf(val_new)){
//       //fprintf(stderr, "Inc_Bessel_K: Inf at iteration %" FCS_LMOD_INT "d: val_new = %e, val_old = %e, err = %e, eps = %e, nu = %e, x = %e, y = %e.\n", n, val_new, val_old, err, eps, nu, x, y);
//       val_new=val_old;
//       break;
//     }
//   }
// 
//   free(pt); free(D_Aki); free(N_Aki);
// //   fprintf(stderr, "n = %d, val = %e, err %e\n", n, val_new, err);
//   return val_new;
// }


/****************************************************************************************/
/* inc. upper Bessel_K function, variant 2, see [Slavinsky, Weniger 2015] Algorithm 3.4 */
/****************************************************************************************/

fcs_float ifcs_p2nfft_inc_upper_bessel_k(
    fcs_float nu, fcs_float x, fcs_float y, fcs_float eps
    )
{
  fcs_int n = 2, n_max = 127;
  fcs_float err = 1.0, val_new, val_old;
  fcs_float N[4], D[4];

  /* Fct. gsl_sf_gamma_inc aborts for large arguments due to underflows.
   * Therefore, we return 0.0 if one of the following upper bounds is already very small. */
  if(-21 <= nu && nu <= 21){
    /* For -21 <= nu <= 21 we used Mathematica to find x large enough such that the upper bound
     *   K_nu(x,y) <= x^nu * Gamma(-nu,x)
     * is very small */
    // if( x > 225 ) return 0.0; /* x^nu * Gamma(-nu,x) < 1e-100 */
    if( x > 111 ) return 0.0; /* x^nu * Gamma(-nu,x) < 1e-50 */
     
    /* For -21 <= nu <= 21, x<=y we used Mathematica to find x large enough such that the upper bound
     *   K_nu(x,y) <=  2*K_nu(2*Sqrt(xy))
     * based on formula (4) of [Slevinsky-Safouhi 2010], is very small. */
    // if(x<y) if( x*y > 115.0*115.0 ) return 0.0; /*  2*K_nu(2*Sqrt(xy)) < 1e-100 */
    if(x<y) if( x*y > 58.0*58.0 ) return 0.0; /*  2*K_nu(2*Sqrt(xy)) < 1e-50 */
  } else {
    const fcs_float bound = 1e-50;

    if(nu >= -1){
    /* For arbitrary nu > -1 we solve the very crude upper bound
       inc_upper_bessel(nu,x,y) < Exp[-x]/x for x. */
      if( x > gsl_sf_lambert_W0(1/bound) ) return 0.0;
    } else {
      /* For nu < -1 we use the bounds 
         inc_upper_bessel(nu,x,y) <= exp(1-x)*(-nu-1)!*x^nu for x<=1
         inc_upper_bessel(nu,x,y) <= exp(1-x)*(-nu-1)!*x^(-1) for x>1 */
      fcs_float fak = 1.0;
      for(fcs_int t=1; t<-nu; t++) 
        fak *= t;
      
      if( fak*fcs_exp(1-x)*fcs_pow(x,-1) < bound ) return 0.0;
    }
  }

  /* for y==0 incompl. bessel_k can be computed using incompl. Gamma fct. */
  if(fcs_float_is_zero(y))
    return fcs_pow(x,nu) * gsl_sf_gamma_inc(-nu,x); 

  /* for x<y compute the faster convergent complement integral,
   * see formula (4) of [Slevinsky-Safouhi 2010] */
  if(x<y)
    return 2 * fcs_pow(x/y, nu/2) * ifcs_p2nfft_bessel_k(nu, 2*fcs_sqrt(x*y)) - ifcs_p2nfft_inc_upper_bessel_k(-nu, y, x, eps);
  
  /* for nu=0 and x,y small use Taylor approximation */
  if(fcs_float_is_zero(nu)){
    if(fcs_pow(x,2) + fcs_pow(y,2) < fcs_pow(0.75,2)){
      fcs_int k = 0;
      fcs_float fak = 1.0;
      fcs_float z = 0.0;
      while( fcs_exp(-x)*fcs_pow(y,k+1)/(x*(k+1)*fak) > eps){
	z+=fcs_pow(-1,k)*fcs_pow(x*y,k)*gsl_sf_gamma_inc(-k,x)/fak;
	k+=1;
	fak*=k;
      }
      return z;
    }
  }

  N[0] = 0.0;
  N[1] = 1.0;
  N[2] = 0.5*(x+nu+3.0-y)*N[1];
  N[3] = (x+nu+5.0-y)*N[2] + (2.0*y-nu-2.0)*N[1];
  N[3] = N[3]/3.0;

  D[0] = exp(x+y);
  D[1] = (x+nu+1.0-y)*D[0];
  D[2] = 0.5*(x+nu+3.0-y)*D[1] + 0.5*(2.0*y-nu-1.0)*D[0];
  D[3] = (x+nu+5.0-y)*D[2] + (2.0*y-nu-2.0)*D[1] - y*D[0];
  D[3] = D[3]/3.0;
  
  val_old = N[2]/D[2];
  val_new = N[3]/D[3];
  
  /* compute current abs. error*/
  err = fabs(val_new - val_old);
  
  while(err > eps){

    /* avoid overflow by division with very small numbers */
    if(fabs(val_new) < eps)
      break;

    if(n >= n_max){
      //fprintf(stderr, "Inc_Bessel_K: Cannot reach accuracy within %" FCS_LMOD_INT "d iterations: val_new = %e, val_old = %e, err = %e, eps = %e, nu = %e, x = %e, y = %e.\n", n_max, val_new, val_old, err, eps, nu, x, y);
      break;
    }
    n++;

    /* safe last value as val_old */
    val_old = val_new;
    
    /* compute new value */
    /* we only need the last 3 terms */
    N[0]=N[1];
    N[1]=N[2];
    N[2]=N[3];
    
    D[0]=D[1];
    D[1]=D[2];
    D[2]=D[3];
    
    N[3] = (x+nu+1+2*n-y)*N[2] + (2*y-nu-n)*N[1] - y*N[0];
    N[3] = N[3]/(n+1);
    
    D[3] = (x+nu+1+2*n-y)*D[2] + (2*y-nu-n)*D[1] - y*D[0];
    D[3] = D[3]/(n+1);
    
    val_new = N[3]/D[3];

    if(isnan(val_new)){
      //fprintf(stderr, "Inc_Bessel_K: NAN at iteration %" FCS_LMOD_INT "d: val_new = %e, val_old = %e, err = %e, eps = %e, nu = %e, x = %e, y = %e.\n", n, val_new, val_old, err, eps, nu, x, y);
      val_new=val_old;
      break;
    }

    if(fcs_float_is_zero(val_new)){
      //fprintf(stderr, "Inc_Bessel_K: value 0 at iteration %" FCS_LMOD_INT "d: val_new = %e, val_old = %e, err = %e, eps = %e, nu = %e, x = %e, y = %e.\n", n, val_new, val_old, err, eps, nu, x, y);
      val_new=val_old;
      break;
    }

    if(isinf(val_new)){
      //fprintf(stderr, "Inc_Bessel_K: Inf at iteration %" FCS_LMOD_INT "d: val_new = %e, val_old = %e, err = %e, eps = %e, nu = %e, x = %e, y = %e.\n", n, val_new, val_old, err, eps, nu, x, y);
      val_new=val_old;
      break;
    }
    
    /* compute current abs. error*/
    err = fabs(val_new - val_old);
  }

//   fprintf(stderr, "n = %d, val = %e, err %e\n", n, val_new, err);
  return val_new;
}

/*************************************************/
/* wrappers for Bessel K and inc. lower Bessel K */
/*************************************************/

fcs_float ifcs_p2nfft_inc_lower_bessel_k(
    fcs_float nu, fcs_float x, fcs_float y, fcs_float eps
    )
{
  return ifcs_p2nfft_inc_upper_bessel_k(-nu, y, x, eps);
}

fcs_float ifcs_p2nfft_bessel_k(
    fcs_float nu, fcs_float x
    )
{
  return gsl_sf_bessel_Knu(nu,x);
}



/*****************************************************/
/* recreate the table from [Slevinsky, Safouhi 2010] */
/*****************************************************/

static void plot_value(
    fcs_float x,fcs_float y, fcs_float nu, fcs_float eps
    )
{
  fprintf(stderr, "x = %f, y = %f, nu = %f, inc_bessel_K = %.16e\n", x, y, nu, ifcs_p2nfft_inc_upper_bessel_k(nu, x, y, eps));
}

static void plot_table1(
    fcs_float eps
    )
{
  fprintf(stderr, "\nTable 1, eps = %e\n", eps);
  for(fcs_int nu=0; nu<=9; nu++)
    plot_value(0.01, 4, nu, eps);
  plot_value(4.95, 5, 2, eps);
  plot_value(10, 2, 6, eps);
  plot_value(3.1, 2.6, 5, eps);
}

static void plot_table2(
    fcs_float eps
    )
{
  fprintf(stderr, "\nTable 2, eps = %e\n", eps);
  plot_value(1, 1, 8, eps);
  plot_value(1, 1, 16, eps);
  plot_value(5, 5, 4, eps);
  plot_value(5, 5, 8, eps);
  plot_value(5, 5, 16, eps);
  plot_value(10, 1, 16, eps);
  plot_value(10, 5, 16, eps);
  plot_value(10, 10, 16, eps);
  plot_value(1, 5, 1.6, eps);
  plot_value(1, 10, 2.1, eps);
  plot_value(5, 10, 3.5, eps);
  plot_value(0.1, 0.1, 16, eps);
  plot_value(0.5, 0.5, 12, eps);
}

void ifcs_p2nfft_plot_slavinsky_safouhi_table(
    void
    )
{
  plot_table1(1e-10);
  plot_table1(1e-15);
  plot_table2(1e-10);
  plot_table2(1e-15);
}
