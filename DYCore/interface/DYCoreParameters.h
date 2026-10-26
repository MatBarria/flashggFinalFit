#ifndef FLASHGGFINALFIT_DYCORE_PARAMETERS_H
#define FLASHGGFINALFIT_DYCORE_PARAMETERS_H

#include <string>
#include <vector>

struct DYExpParameters {
    double theta;
    double a1;
    double b1;
    double c1;
    double a2;
};

struct DYPowParameters {
    double frac;
    double a1;
    double b1;
    double c1;
    double a2;
};


// Per-category third order Chebyshev correction applied on top of
// the fixed DYCore / DYSplineCore shapes (see RooDYCoreMod and
// RooDYSplineCoreMod).  These are the parameters fitted on the
// MC in Background/test/fitDYshapes.cpp.
struct DYChebModParameters {
    double c1;
    double c2;
    double c3;
};

DYExpParameters loadDYExpParameters(const std::string& cat);
DYPowParameters loadDYPowParameters(const std::string& cat);
DYChebModParameters loadDYCoreModParameters(const std::string& cat);
DYChebModParameters loadDYSplineCoreModParameters(const std::string& cat);

#endif
