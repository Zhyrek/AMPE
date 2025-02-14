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
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the disclaimer below.
// - Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the disclaimer (as noted below) in the
//   documentation and/or other materials provided with the distribution.
// - Neither the name of the LLNS/LLNL nor the names of its contributors may be
//   used to endorse or promote products derived from this software without
//   specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL LAWRENCE LIVERMORE NATIONAL SECURITY,
// LLC, UT BATTELLE, LLC,
// THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#include "CALPHADConcSolverTernary.h"
#include "CALPHADFunctions.h"

#include <iostream>
#include <cmath>
#include <cassert>

namespace ampe_thermo
{

//=======================================================================

CALPHADConcSolverTernary::CALPHADConcSolverTernary() {}

//=======================================================================

// solve for c=(c_L, c_A)
void CALPHADConcSolverTernary::RHS(const double* const c, double* const fvec)
{
   assert(d_fC[0] == d_fC[0]);

   const double* const cL = &c[0];
   const double* const cS = &c[2];

   /*
    * derivatives in liquid
    */
   double derivIdealMixL[2];
   CALPHADcomputeFIdealMix_derivTernary(d_RT, cL[0], cL[1], derivIdealMixL);

   double derivFMixL[2];
   CALPHADcomputeFMix_derivTernary(d_L_AB_L, d_L_AC_L, d_L_BC_L, d_L_ABC_L,
                                   cL[0], cL[1], derivFMixL);

   double dfLdciL[2];
   // 1st species
   dfLdciL[0] = d_fA[0] - d_fC[0] + derivFMixL[0] + derivIdealMixL[0];

   // 2nd species
   dfLdciL[1] = d_fB[0] - d_fC[0] + derivFMixL[1] + derivIdealMixL[1];

   /*
    * derivatives in solid
    */
   double derivIdealMixS[2];
   CALPHADcomputeFIdealMix_derivTernary(d_RT, cS[0], cS[1], derivIdealMixS);

   double derivFMixS[2];
   CALPHADcomputeFMix_derivTernary(d_L_AB_S, d_L_AC_S, d_L_BC_S, d_L_ABC_S,
                                   cS[0], cS[1], derivFMixS);

   double dfSdciS[2];
   // 1st species
   dfSdciS[0] = d_fA[1] - d_fC[1] + derivFMixS[0] + derivIdealMixS[0];

   // 2nd species
   dfSdciS[1] = d_fB[1] - d_fC[1] + derivFMixS[1] + derivIdealMixS[1];

   /*
    * system of 4 equations
    */

   // equation for 1st species
   fvec[0] = (1.0 - d_hphi) * cL[0] + d_hphi * cS[0] - d_c0[0];

   // equation for 2nd species
   fvec[1] = (1.0 - d_hphi) * cL[1] + d_hphi * cS[1] - d_c0[1];

   // equal chemical potential equation for 1st species
   fvec[2] = dfLdciL[0] - dfSdciS[0];

   // equal chemical potential equation for 2nd species
   fvec[3] = dfLdciL[1] - dfSdciS[1];

   assert(fvec[0] == fvec[0]);
   assert(fvec[1] == fvec[1]);
   assert(fvec[2] == fvec[2]);
   assert(fvec[3] == fvec[3]);
}

//=======================================================================

void CALPHADConcSolverTernary::Jacobian(const double* const c,
                                        double** const fjac)
{
   const double* const cL = &c[0];
   const double* const cS = &c[2];

   double deriv2IdealMixL[4];
   CALPHADcomputeFIdealMix_deriv2Ternary(d_RT, cL[0], cL[1], deriv2IdealMixL);

   double deriv2FMixL[4];
   CALPHADcomputeFMix_deriv2Ternary(d_L_AB_L, d_L_AC_L, d_L_BC_L, d_L_ABC_L,
                                    cL[0], cL[1], deriv2FMixL);

   double d2fLdciL2[3];  // include only one cross term (other one equal by
                         // symmetry)
   d2fLdciL2[0] = deriv2FMixL[0] + deriv2IdealMixL[0];
   d2fLdciL2[1] = deriv2FMixL[1] + deriv2IdealMixL[1];
   d2fLdciL2[2] = deriv2FMixL[3] + deriv2IdealMixL[3];

   double deriv2IdealMixS[4];
   CALPHADcomputeFIdealMix_deriv2Ternary(d_RT, cS[0], cS[1], deriv2IdealMixS);

   double deriv2FMixS[4];
   CALPHADcomputeFMix_deriv2Ternary(d_L_AB_S, d_L_AC_S, d_L_BC_S, d_L_ABC_S,
                                    cS[0], cS[1], deriv2FMixS);

   double d2fSdciS2[3];
   d2fSdciS2[0] = deriv2FMixS[0] + deriv2IdealMixS[0];
   d2fSdciS2[1] = deriv2FMixS[1] + deriv2IdealMixS[1];
   d2fSdciS2[2] = deriv2FMixS[3] + deriv2IdealMixS[3];

   /*
    * Jacobian:
    * f[i][j]=df[i]/dc[j]
    */
   fjac[0][0] = (1.0 - d_hphi);
   fjac[0][1] = 0.;
   fjac[0][2] = d_hphi;
   fjac[0][3] = 0.;

   fjac[1][0] = 0.;
   fjac[1][1] = (1.0 - d_hphi);
   fjac[1][2] = 0.;
   fjac[1][3] = d_hphi;

   fjac[2][0] = d2fLdciL2[0];
   fjac[2][1] = d2fLdciL2[1];
   fjac[2][2] = -d2fSdciS2[0];
   fjac[2][3] = -d2fSdciS2[1];

   fjac[3][0] = d2fLdciL2[1];
   fjac[3][1] = d2fLdciL2[2];
   fjac[3][2] = -d2fSdciS2[1];
   fjac[3][3] = -d2fSdciS2[2];
}

//=======================================================================

void CALPHADConcSolverTernary::setup(
    const double c0, const double c1, const double hphi, const double RTinv,
    const double* const L_AB_L, const double* const L_AC_L,
    const double* const L_BC_L, const double* const L_AB_S,
    const double* const L_AC_S, const double* const L_BC_S,
    const double* const L_ABC_L, const double* const L_ABC_S,
    const double* const fA, const double* const fB, const double* const fC)
{
   assert(fC[0] == fC[0]);

   // std::cout<<"CALPHADConcSolverTernary::ComputeConcentration()"<<endl;
   d_c0[0] = c0;
   d_c0[1] = c1;
   d_hphi = hphi;
   d_RT = 1. / RTinv;

   for (int ii = 0; ii < 4; ii++)
      d_L_AB_L[ii] = L_AB_L[ii];
   for (int ii = 0; ii < 4; ii++)
      d_L_AC_L[ii] = L_AC_L[ii];
   for (int ii = 0; ii < 4; ii++)
      d_L_BC_L[ii] = L_BC_L[ii];

   for (int ii = 0; ii < 4; ii++)
      d_L_AB_S[ii] = L_AB_S[ii];
   for (int ii = 0; ii < 4; ii++)
      d_L_AC_S[ii] = L_AC_S[ii];
   for (int ii = 0; ii < 4; ii++)
      d_L_BC_S[ii] = L_BC_S[ii];

   for (int ii = 0; ii < 3; ii++)
      d_L_ABC_S[ii] = L_ABC_S[ii];
   for (int ii = 0; ii < 3; ii++)
      d_L_ABC_L[ii] = L_ABC_L[ii];

   // loop over phases
   for (int ii = 0; ii < 2; ii++)
      d_fA[ii] = fA[ii];
   for (int ii = 0; ii < 2; ii++)
      d_fB[ii] = fB[ii];
   for (int ii = 0; ii < 2; ii++)
      d_fC[ii] = fC[ii];
}

int CALPHADConcSolverTernary::ComputeConcentration(
    double* const conc, const double c0, const double c1, const double hphi,
    const double RTinv, const double* const L_AB_L, const double* const L_AC_L,
    const double* const L_BC_L, const double* const L_AB_S,
    const double* const L_AC_S, const double* const L_BC_S,
    const double* const L_ABC_L, const double* const L_ABC_S,
    const double* const fA, const double* const fB, const double* const fC)
{
   setup(c0, c1, hphi, RTinv, L_AB_L, L_AC_L, L_BC_L, L_AB_S, L_AC_S, L_BC_S,
         L_ABC_L, L_ABC_S, fA, fB, fC);

   int ret = NewtonSolver::ComputeSolution(conc, 4);

   return ret;
}

}  // namespace ampe_thermo
