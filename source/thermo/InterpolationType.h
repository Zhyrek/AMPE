// Copyright (c) 2018, Lawrence Livermore National Security, LLC and
// UT-Battelle, LLC.
// Produced at the Lawrence Livermore National Laboratory and
// the Oak Ridge National Laboratory
// Written by M.R. Dorr, J.-L. Fattebert and M.E. Wickett
// LLNL-CODE-747500
// All rights reserved.
// This file is part of AMPE.
// For details, see https://github.com/LLNL/AMPE
// Please also read AMPE/LICENSE.
//
#ifndef InterpolationType_H
#define InterpolationType_H

namespace ampe_thermo
{

enum class ConcInterpolationType { LINEAR, PBG, HARMONIC, UNDEFINED };

enum class EnergyInterpolationType { LINEAR, PBG, HARMONIC, UNDEFINED };


char energyInterpChar(EnergyInterpolationType interp_func_type);
char concInterpChar(ConcInterpolationType interp_func_type);

}  // namespace ampe_thermo

#endif
