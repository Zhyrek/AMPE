// Copyright (c) 2018, Lawrence Livermore National Security, LLC and
// UT-Battelle, LLC.
// Produced at the Lawrence Livermore National Laboratory and
// the Oak Ridge National Laboratory
// LLNL-CODE-747500
// All rights reserved.
// This file is part of AMPE.
// For details, see https://github.com/LLNL/AMPE
// Please also read AMPE/LICENSE.
//
#include "PhaseFACOps.h"

#include "SAMRAI/hier/Index.h"
#include "SAMRAI/hier/PatchData.h"
#include "SAMRAI/tbox/Utilities.h"
#include "FuncFort.h"

#include <cassert>


//======================================================================

PhaseFACOps::PhaseFACOps(const std::string& object_name,
                         std::shared_ptr<tbox::Database> database)
    : EllipticFACOps(tbox::Dimension(NDIM), object_name, database)
{
   t_setcoeffs_timer = tbox::TimerManager::getManager()->getTimer(
       "AMPE::PhaseFACOps::setOperatorCoefficients()");
}

//======================================================================

void PhaseFACOps::setOperatorCoefficients(
    const int phase_id, const int phase_mobility_id, const double epsilon_phase,
    const double gamma, const EnergyInterpolationType phase_interp_func_type,
    const double phase_well_scale, const std::string phase_well_func_type)
{
   assert(phase_mobility_id >= 0);

   t_setcoeffs_timer->start();

   setM(phase_mobility_id);

   // C to be set after M since it uses M
   setC(phase_id, gamma, phase_interp_func_type, phase_well_scale,
        phase_well_func_type);

   setDConstant(-gamma * epsilon_phase * epsilon_phase);

   t_setcoeffs_timer->stop();
}

//======================================================================

// C = 1 + gamma * phi_mobility * (
//       phi_well_scale * phi_well_func'' )

void PhaseFACOps::setC(const int phi_id, const double gamma,
                       const EnergyInterpolationType phi_interp_func_type,
                       const double phi_well_scale,
                       const std::string phi_well_func_type)
{
   assert(phi_id >= 0);
   assert(d_m_id >= 0);
   assert(d_c_id[0] >= 0);
   assert(d_M_is_set);

   // tbox::pout<<"PhaseFACOps::setC()..."<<endl;

   for (int ln = d_ln_min; ln <= d_ln_max; ++ln) {
      std::shared_ptr<hier::PatchLevel> level = d_hierarchy->getPatchLevel(ln);

      for (hier::PatchLevel::iterator pi(level->begin()); pi != level->end();
           ++pi) {

         std::shared_ptr<hier::Patch> patch = *pi;
         const hier::Box& patch_box = patch->getBox();

         std::shared_ptr<pdat::CellData<double> > phi_data(
             SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
                 patch->getPatchData(phi_id)));

         std::shared_ptr<pdat::CellData<double> > local_m_data(
             SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
                 patch->getPatchData(d_m_id)));

         std::shared_ptr<pdat::CellData<double> > cdata(
             SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
                 patch->getPatchData(d_c_id[0])));

         setCOnPatchPrivate(phi_data, local_m_data, cdata, gamma,
                            phi_interp_func_type, phi_well_scale,
                            phi_well_func_type.c_str(), patch_box);
      }
   }

   setCPatchDataId(d_c_id[0], 0);

   return;
}

void PhaseFACOps::setCOnPatchPrivate(
    std::shared_ptr<pdat::CellData<double> > cd_phi,
    std::shared_ptr<pdat::CellData<double> > cd_m,
    std::shared_ptr<pdat::CellData<double> > cd_c, const double gamma,
    const EnergyInterpolationType phi_interp_func_type,
    const double phi_well_scale, const char* phi_well_func_type,
    const hier::Box& pbox)
{
   double* ptr_phi = cd_phi->getPointer();
   double* ptr_m = cd_m->getPointer();
   double* ptr_c = cd_c->getPointer();

   const hier::Box& c_gbox = cd_c->getGhostBox();
   int imin_c = c_gbox.lower(0);
   int jmin_c = c_gbox.lower(1);
   int jp_c = c_gbox.numberCells(0);
   int kmin_c = 0;
   int kp_c = 0;
#if (NDIM == 3)
   kmin_c = c_gbox.lower(2);
   kp_c = jp_c * c_gbox.numberCells(1);
#endif

   const hier::Box& m_gbox = cd_m->getGhostBox();
   int imin_m = m_gbox.lower(0);
   int jmin_m = m_gbox.lower(1);
   int jp_m = m_gbox.numberCells(0);
   int kmin_m = 0;
   int kp_m = 0;
#if (NDIM == 3)
   kmin_m = m_gbox.lower(2);
   kp_m = jp_m * m_gbox.numberCells(1);
#endif

   const hier::Box& pf_gbox = cd_phi->getGhostBox();
   int imin_pf = pf_gbox.lower(0);
   int jmin_pf = pf_gbox.lower(1);
   int jp_pf = pf_gbox.numberCells(0);
   int kmin_pf = 0;
   int kp_pf = 0;
#if (NDIM == 3)
   kmin_pf = pf_gbox.lower(2);
   kp_pf = jp_pf * pf_gbox.numberCells(1);
#endif

   int imin = pbox.lower(0);
   int imax = pbox.upper(0);
   int jmin = pbox.lower(1);
   int jmax = pbox.upper(1);
   int kmin = 0;
   int kmax = 0;
#if (NDIM == 3)
   kmin = pbox.lower(2);
   kmax = pbox.upper(2);
#endif
   const char interp = energyInterpChar(phi_interp_func_type);

   for (int kk = kmin; kk <= kmax; kk++) {
      for (int jj = jmin; jj <= jmax; jj++) {
         for (int ii = imin; ii <= imax; ii++) {

            const int idx_c =
                (ii - imin_c) + (jj - jmin_c) * jp_c + (kk - kmin_c) * kp_c;

            const int idx_m =
                (ii - imin_m) + (jj - jmin_m) * jp_m + (kk - kmin_m) * kp_m;

            const int idx_pf = (ii - imin_pf) + (jj - jmin_pf) * jp_pf +
                               (kk - kmin_pf) * kp_pf;

            const double m = ptr_m[idx_m];
            const double phi = ptr_phi[idx_pf];

            const double g_phi_dbl_prime =
                SECOND_DERIV_WELL_FUNC(phi, phi_well_func_type);

            const double gamma_m = gamma * m;

            // C = 1 + gamma * phi_mobility * (
            //       phi_well_scale * phi_well_func'' )
            ptr_c[idx_c] = 1.0 + gamma_m * phi_well_scale * g_phi_dbl_prime;
         }
      }
   }
}
