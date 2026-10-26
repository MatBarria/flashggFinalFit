#include "../interface/RooDYBernCore.h"

#include <cmath>



RooDYBernCore::RooDYBernCore(
    const char* name,
    const char* title,
    RooAbsReal& _m
) :

RooAbsPdf(name,title),

m("m","m",this,_m)

{
}



RooDYBernCore::RooDYBernCore(
    const RooDYBernCore& other,
    const char* name
) :

RooAbsPdf(other,name),

m("m",this,other.m)

{
}



Double_t RooDYBernCore::evaluate() const
{

    //------------------------------------------------------
    // observable
    //------------------------------------------------------

    double mass = m;


    //------------------------------------------------------
    // Combine Bernstein normalization
    //
    // x=(m-xmin)/(xmax-xmin)
    //------------------------------------------------------

    constexpr double xmin = 110.0;
    constexpr double xmax = 150.0;

    double x = (mass-xmin)/(xmax-xmin);

    if(x<0.) x=0.;
    if(x>1.) x=1.;


    //------------------------------------------------------
    // Fixed Bernstein parameters
    //
    // Replace these five numbers with the fitted
    // bern_p0 ... bern_p4 from fitBern5_Combine.C
    //------------------------------------------------------

    constexpr double p0 = 1.2031e+00;
    constexpr double p1 = 5.1379e-01;
    constexpr double p2 = 7.0655e-01;
    constexpr double p3 = 4.4930e-01;
    constexpr double p4 = 4.4733e-01;


    //------------------------------------------------------
    // Combine uses coefficient = parameter^2
    //------------------------------------------------------

    const double c0 = p0*p0;
    const double c1 = p1*p1;
    const double c2 = p2*p2;
    const double c3 = p3*p3;
    const double c4 = p4*p4;


    //------------------------------------------------------
    // Bernstein polynomial
    //
    // Equivalent to RooBernsteinFast<5>
    //------------------------------------------------------

    const double omx = 1.0-x;

    double val =

          c0*omx*omx*omx*omx

        + 4.0*c1*x*omx*omx*omx

        + 6.0*c2*x*x*omx*omx

        + 4.0*c3*x*x*x*omx

        + c4*x*x*x*x;


    //------------------------------------------------------
    // Numerical protection
    //------------------------------------------------------

    return (val>0.) ? val : 1e-12;

}



#include "TClass.h"

ClassImp(RooDYBernCore)
