// Copyright (c) 2018, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory
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
// LLC, THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
// 
#ifndef included_CALPHADequilibriumPhaseConcentrationsStrategy
#define included_CALPHADequilibriumPhaseConcentrationsStrategy

#include "PhaseConcentrationsStrategy.h"
#include "CALPHADFreeEnergyFunctionsBinary.h"

#include "SAMRAI/tbox/InputManager.h"

class CALPHADequilibriumPhaseConcentrationsStrategy:public PhaseConcentrationsStrategy
{
public:
   CALPHADequilibriumPhaseConcentrationsStrategy(
      const int conc_l_id,
      const int conc_a_id,
      const int conc_b_id,
      const int conc_l_ref_id,
      const int conc_a_ref_id,
      const int conc_b_ref_id,
      const int ncompositions,
      const std::string& phase_interp_func_type,
      const std::string& eta_interp_func_type,
      const std::string& avg_func_type,
      const bool with_third_phase,
      const double  phase_well_scale,
      const double eta_well_scale,
      const std::string& phase_well_func_type,
      const std::string& eta_well_func_type,
      boost::shared_ptr<tbox::Database> calphad_db,
      boost::shared_ptr<tbox::Database> newton_d);

   ~CALPHADequilibriumPhaseConcentrationsStrategy()
   {
      delete d_calphad_fenergy;
   }

   virtual void computePhaseConcentrationsOnPatch(
      boost::shared_ptr< pdat::CellData<double> > cd_temperature,
      boost::shared_ptr< pdat::CellData<double> > cd_phi,
      boost::shared_ptr< pdat::CellData<double> > cd_eta,
      boost::shared_ptr< pdat::CellData<double> > cd_concentration,
      boost::shared_ptr< pdat::CellData<double> > cd_c_l,
      boost::shared_ptr< pdat::CellData<double> > cd_c_a,
      boost::shared_ptr< pdat::CellData<double> > cd_c_b,
      boost::shared_ptr<hier::Patch > patch );

private:
   int d_conc_l_ref_id;
   int d_conc_a_ref_id;
   int d_conc_b_ref_id;
   
   int d_ncompositions;
   
   FreeEnergyFunctions* d_calphad_fenergy;
};

#endif
