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
#include "ConcFort.h"
#include "QuatParams.h"
#include "CALPHADFreeEnergyStrategyBinary.h"
#include "CALPHADFunctions.h"
#include "MolarVolumeStrategy.h"
#include "CALPHADFreeEnergyFunctionsBinary.h"
#include "CALPHADFreeEnergyFunctionsTernary.h"
#include "FuncFort.h"

#ifdef HAVE_THERMO4PFM
#include "CALPHADFreeEnergyFunctionsBinary2Ph1Sl.h"
#include "Database2JSON.h"
namespace pt = boost::property_tree;
#endif

#include "SAMRAI/tbox/InputManager.h"
#include "SAMRAI/pdat/CellData.h"
#include "SAMRAI/pdat/SideData.h"
#include "SAMRAI/hier/Index.h"
#include "SAMRAI/math/HierarchyCellDataOpsReal.h"

using namespace SAMRAI;


#include <cassert>

//=======================================================================

template <class FreeEnergyFunctionType>
CALPHADFreeEnergyStrategyBinary<FreeEnergyFunctionType>::
    CALPHADFreeEnergyStrategyBinary(
#ifdef HAVE_THERMO4PFM
        pt::ptree calphad_db,
#else
        std::shared_ptr<tbox::Database> calphad_db,
#endif
        std::shared_ptr<tbox::Database> newton_db,
        const EnergyInterpolationType energy_interp_func_type,
        const ConcInterpolationType conc_interp_func_type,
        MolarVolumeStrategy* mvstrategy, const int conc_l_id,
        const int conc_a_id, const int conc_b_id, const bool with_third_phase)
    : d_mv_strategy(mvstrategy),
      d_energy_interp_func_type(energy_interp_func_type),
      d_conc_interp_func_type(conc_interp_func_type),
      d_conc_l_id(conc_l_id),
      d_conc_a_id(conc_a_id),
      d_conc_b_id(conc_b_id),
      d_with_third_phase(with_third_phase)
{
   // conversion factor from [J/mol] to [pJ/(mu m)^3]
   // vm^-1 [mol/m^3] * 10e-18 [m^3/(mu m^3)] * 10e12 [pJ/J]
   // d_jpmol2pjpmumcube = 1.e-6 / d_vm;

   // R = 8.314472 J � K-1 � mol-1
   // tbox::plog << "CALPHADFreeEnergyStrategyBinary:" << std::endl;
   // tbox::plog << "Molar volume L =" << vml << std::endl;
   // tbox::plog << "Molar volume A =" << vma << std::endl;
   // tbox::plog << "jpmol2pjpmumcube=" << d_jpmol2pjpmumcube << std::endl;

   setup(calphad_db, newton_db);
}

//=======================================================================

template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<FreeEnergyFunctionType>::setup(
#ifdef HAVE_THERMO4PFM
    pt::ptree calphad_pt,
#else
    std::shared_ptr<tbox::Database> calphad_db,
#endif
    std::shared_ptr<tbox::Database> newton_db)
{
#ifdef HAVE_THERMO4PFM
   pt::ptree newton_pt;
   if (newton_db) copyDatabase(newton_db, newton_pt);
   d_calphad_fenergy.reset(new FreeEnergyFunctionType(calphad_pt, newton_pt,
                                                      d_energy_interp_func_type,
                                                      d_conc_interp_func_type));
#else
   d_calphad_fenergy.reset(new FreeEnergyFunctionType(calphad_db, newton_db,
                                                      d_energy_interp_func_type,
                                                      d_conc_interp_func_type,
                                                      d_with_third_phase));
#endif
}

//=======================================================================

template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<FreeEnergyFunctionType>::
    computeDerivFreeEnergyLiquid(hier::Patch& patch, const int temperature_id,
                                 const int dfl_id)
{
   assert(temperature_id >= 0);
   assert(dfl_id >= 0);

   computeDerivFreeEnergy(patch, temperature_id, dfl_id, d_conc_l_id,
                          PhaseIndex::phaseL);
}

//=======================================================================

template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<FreeEnergyFunctionType>::
    computeDerivFreeEnergySolidA(hier::Patch& patch, const int temperature_id,
                                 const int dfs_id)
{
   assert(temperature_id >= 0.);
   assert(dfs_id >= 0);

   computeDerivFreeEnergy(patch, temperature_id, dfs_id, d_conc_a_id,
                          PhaseIndex::phaseA);
}

//=======================================================================

template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<FreeEnergyFunctionType>::
    computeDerivFreeEnergySolidB(hier::Patch& patch, const int temperature_id,
                                 const int dfs_id)
{
   assert(temperature_id >= 0.);
   assert(dfs_id >= 0);

   computeDerivFreeEnergy(patch, temperature_id, dfs_id, d_conc_b_id,
                          PhaseIndex::phaseB);
}

//=======================================================================

template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<
    FreeEnergyFunctionType>::computeFreeEnergyLiquid(hier::Patch& patch,
                                                     const int temperature_id,
                                                     const int fl_id,
                                                     const bool gp)
{
   assert(temperature_id >= 0);
   assert(fl_id >= 0);

   assert(d_conc_l_id >= 0);

   computeFreeEnergy(patch, temperature_id, fl_id, d_conc_l_id,
                     PhaseIndex::phaseL, gp);
}

//=======================================================================

template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<
    FreeEnergyFunctionType>::computeFreeEnergySolidA(hier::Patch& patch,
                                                     const int temperature_id,
                                                     const int fs_id,
                                                     const bool gp)
{
   assert(temperature_id >= 0.);
   assert(fs_id >= 0);

   computeFreeEnergy(patch, temperature_id, fs_id, d_conc_a_id,
                     PhaseIndex::phaseA, gp);
}

//=======================================================================

template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<
    FreeEnergyFunctionType>::computeFreeEnergySolidB(hier::Patch& patch,
                                                     const int temperature_id,
                                                     const int fs_id,
                                                     const bool gp)
{
   assert(temperature_id >= 0.);
   assert(fs_id >= 0);

   computeFreeEnergy(patch, temperature_id, fs_id, d_conc_b_id,
                     PhaseIndex::phaseB, gp);
}

//=======================================================================

template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<FreeEnergyFunctionType>::computeFreeEnergy(
    hier::Patch& patch, const int temperature_id, const int f_id,
    const int conc_i_id, const PhaseIndex pi, const bool gp)
{
   assert(temperature_id >= 0);
   assert(f_id >= 0);
   assert(conc_i_id >= 0);

   const hier::Box& pbox = patch.getBox();

   std::shared_ptr<pdat::CellData<double> > temperature(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(temperature_id)));

   std::shared_ptr<pdat::CellData<double> > f(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(f_id)));

   std::shared_ptr<pdat::CellData<double> > c_i(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(conc_i_id)));

   computeFreeEnergy(pbox, temperature, f, c_i, pi, gp);
}

//=======================================================================

template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<
    FreeEnergyFunctionType>::computeDerivFreeEnergy(hier::Patch& patch,
                                                    const int temperature_id,
                                                    const int df_id,
                                                    const int conc_i_id,
                                                    const PhaseIndex pi)
{
   assert(temperature_id >= 0);
   assert(df_id >= 0);
   assert(conc_i_id >= 0);

   const hier::Box& pbox = patch.getBox();

   std::shared_ptr<pdat::CellData<double> > temperature(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(temperature_id)));

   std::shared_ptr<pdat::CellData<double> > df(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(df_id)));

   std::shared_ptr<pdat::CellData<double> > c_i(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(conc_i_id)));

   computeDerivFreeEnergy(pbox, temperature, df, c_i, pi);
}

//=======================================================================

template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<FreeEnergyFunctionType>::computeFreeEnergy(
    const hier::Box& pbox, std::shared_ptr<pdat::CellData<double> > cd_temp,
    std::shared_ptr<pdat::CellData<double> > cd_free_energy,
    std::shared_ptr<pdat::CellData<double> > cd_conc_i, const PhaseIndex pi,
    const bool gp)
{
   double* ptr_temp = cd_temp->getPointer();
   double* ptr_f = cd_free_energy->getPointer();
   double* ptr_c_i = cd_conc_i->getPointer();

   const hier::Box& temp_gbox = cd_temp->getGhostBox();
   int imin_temp = temp_gbox.lower(0);
   int jmin_temp = temp_gbox.lower(1);
   int jp_temp = temp_gbox.numberCells(0);
   int kmin_temp = 0;
   int kp_temp = 0;
#if (NDIM == 3)
   kmin_temp = temp_gbox.lower(2);
   kp_temp = jp_temp * temp_gbox.numberCells(1);
#endif

   const hier::Box& f_gbox = cd_free_energy->getGhostBox();
   int imin_f = f_gbox.lower(0);
   int jmin_f = f_gbox.lower(1);
   int jp_f = f_gbox.numberCells(0);
   int kmin_f = 0;
   int kp_f = 0;
#if (NDIM == 3)
   kmin_f = f_gbox.lower(2);
   kp_f = jp_f * f_gbox.numberCells(1);
#endif

   const hier::Box& c_i_gbox = cd_conc_i->getGhostBox();
   int imin_c_i = c_i_gbox.lower(0);
   int jmin_c_i = c_i_gbox.lower(1);
   int jp_c_i = c_i_gbox.numberCells(0);
   int kmin_c_i = 0;
   int kp_c_i = 0;
#if (NDIM == 3)
   kmin_c_i = c_i_gbox.lower(2);
   kp_c_i = jp_c_i * c_i_gbox.numberCells(1);
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

   for (int kk = kmin; kk <= kmax; kk++) {
      for (int jj = jmin; jj <= jmax; jj++) {
         for (int ii = imin; ii <= imax; ii++) {

            const int idx_temp = (ii - imin_temp) + (jj - jmin_temp) * jp_temp +
                                 (kk - kmin_temp) * kp_temp;

            const int idx_f =
                (ii - imin_f) + (jj - jmin_f) * jp_f + (kk - kmin_f) * kp_f;

            const int idx_c_i = (ii - imin_c_i) + (jj - jmin_c_i) * jp_c_i +
                                (kk - kmin_c_i) * kp_c_i;

            double t = ptr_temp[idx_temp];
            double c_i = ptr_c_i[idx_c_i];

            ptr_f[idx_f] =
                d_calphad_fenergy->computeFreeEnergy(t, &c_i, pi, gp);
            ptr_f[idx_f] *= d_mv_strategy->computeInvMolarVolume(t, &c_i, pi);
         }
      }
   }
}

//=======================================================================

template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<FreeEnergyFunctionType>::
    computeDerivFreeEnergy(
        const hier::Box& pbox, std::shared_ptr<pdat::CellData<double> > cd_temp,
        std::shared_ptr<pdat::CellData<double> > cd_free_energy,
        std::shared_ptr<pdat::CellData<double> > cd_conc_i, const PhaseIndex pi)
{
   double* ptr_temp = cd_temp->getPointer();
   double* ptr_f = cd_free_energy->getPointer();
   double* ptr_c_i = cd_conc_i->getPointer();

   const hier::Box& temp_gbox = cd_temp->getGhostBox();
   int imin_temp = temp_gbox.lower(0);
   int jmin_temp = temp_gbox.lower(1);
   int jp_temp = temp_gbox.numberCells(0);
   int kmin_temp = 0;
   int kp_temp = 0;
#if (NDIM == 3)
   kmin_temp = temp_gbox.lower(2);
   kp_temp = jp_temp * temp_gbox.numberCells(1);
#endif

   const hier::Box& f_gbox = cd_free_energy->getGhostBox();
   int imin_f = f_gbox.lower(0);
   int jmin_f = f_gbox.lower(1);
   int jp_f = f_gbox.numberCells(0);
   int kmin_f = 0;
   int kp_f = 0;
#if (NDIM == 3)
   kmin_f = f_gbox.lower(2);
   kp_f = jp_f * f_gbox.numberCells(1);
#endif

   const hier::Box& c_i_gbox = cd_conc_i->getGhostBox();
   int imin_c_i = c_i_gbox.lower(0);
   int jmin_c_i = c_i_gbox.lower(1);
   int jp_c_i = c_i_gbox.numberCells(0);
   int kmin_c_i = 0;
   int kp_c_i = 0;
#if (NDIM == 3)
   kmin_c_i = c_i_gbox.lower(2);
   kp_c_i = jp_c_i * c_i_gbox.numberCells(1);
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

   for (int kk = kmin; kk <= kmax; kk++) {
      for (int jj = jmin; jj <= jmax; jj++) {
         for (int ii = imin; ii <= imax; ii++) {

            const int idx_temp = (ii - imin_temp) + (jj - jmin_temp) * jp_temp +
                                 (kk - kmin_temp) * kp_temp;

            const int idx_f =
                (ii - imin_f) + (jj - jmin_f) * jp_f + (kk - kmin_f) * kp_f;

            const int idx_c_i = (ii - imin_c_i) + (jj - jmin_c_i) * jp_c_i +
                                (kk - kmin_c_i) * kp_c_i;

            double t = ptr_temp[idx_temp];
            double c_i = ptr_c_i[idx_c_i];

            d_calphad_fenergy->computeDerivFreeEnergy(t, &c_i, pi,
                                                      &ptr_f[idx_f]);
            ptr_f[idx_f] *= d_mv_strategy->computeInvMolarVolume(t, &c_i, pi);
         }
      }
   }
}

//=======================================================================

template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<FreeEnergyFunctionType>::
    computeDrivingForce(const double time, hier::Patch& patch,
                        const int temperature_id, const int phase_id,
                        const int eta_id, const int conc_id, const int f_l_id,
                        const int f_a_id, const int f_b_id, const int rhs_id)
{
   EnergyInterpolationType d_energy_interp_func_type_saved(
       d_energy_interp_func_type);
   // use linear interpolation function to get driving force
   // without polynomial of phi factor
   d_energy_interp_func_type = EnergyInterpolationType::LINEAR,

   FreeEnergyStrategy::computeDrivingForce(time, patch, temperature_id,
                                           phase_id, eta_id, conc_id, f_l_id,
                                           f_a_id, f_b_id, rhs_id);

   d_energy_interp_func_type = d_energy_interp_func_type_saved;
};

//=======================================================================

template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<FreeEnergyFunctionType>::addDrivingForce(
    const double time, hier::Patch& patch, const int temperature_id,
    const int phase_id, const int eta_id, const int conc_id, const int f_l_id,
    const int f_a_id, const int f_b_id, const int rhs_id)
{
   (void)time;

   assert(conc_id >= 0);
   assert(phase_id >= 0);
   assert(f_l_id >= 0);
   assert(f_a_id >= 0);
   assert(rhs_id >= 0);
   assert(temperature_id >= 0);
   if (d_with_third_phase) {
      assert(eta_id >= 0);
      assert(f_b_id >= 0);
   }
   assert(d_conc_l_id >= 0);
   assert(d_conc_a_id >= 0);

   std::shared_ptr<pdat::CellData<double> > phase(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(phase_id)));
   assert(phase);

   std::shared_ptr<pdat::CellData<double> > t(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(temperature_id)));
   assert(t);

   std::shared_ptr<pdat::CellData<double> > fl(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(f_l_id)));
   assert(fl);

   std::shared_ptr<pdat::CellData<double> > fa(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(f_a_id)));
   assert(fa);

   std::shared_ptr<pdat::CellData<double> > c_l(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(d_conc_l_id)));
   assert(c_l);

   std::shared_ptr<pdat::CellData<double> > c_a(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(d_conc_a_id)));
   assert(c_a);

   std::shared_ptr<pdat::CellData<double> > rhs(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(rhs_id)));

   assert(rhs);
   assert(rhs->getGhostCellWidth() ==
          hier::IntVector(tbox::Dimension(NDIM), 0));

   std::shared_ptr<pdat::CellData<double> > eta;
   std::shared_ptr<pdat::CellData<double> > fb;
   std::shared_ptr<pdat::CellData<double> > c_b;
   if (d_with_third_phase) {
      eta = SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
          patch.getPatchData(eta_id));
      assert(eta);
      fb = SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
          patch.getPatchData(f_b_id));
      assert(fb);

      c_b = SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
          patch.getPatchData(d_conc_b_id));
      assert(c_b);
   }

   const hier::Box& pbox(patch.getBox());

   addDrivingForce(rhs, t, phase, eta, fl, fa, fb, c_l, c_a, c_b, pbox);
}

//=======================================================================

template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<FreeEnergyFunctionType>::addDrivingForce(
    std::shared_ptr<pdat::CellData<double> > cd_rhs,
    std::shared_ptr<pdat::CellData<double> > cd_temperature,
    std::shared_ptr<pdat::CellData<double> > cd_phi,
    std::shared_ptr<pdat::CellData<double> > cd_eta,
    std::shared_ptr<pdat::CellData<double> > cd_f_l,
    std::shared_ptr<pdat::CellData<double> > cd_f_a,
    std::shared_ptr<pdat::CellData<double> > cd_f_b,
    std::shared_ptr<pdat::CellData<double> > cd_c_l,
    std::shared_ptr<pdat::CellData<double> > cd_c_a,
    std::shared_ptr<pdat::CellData<double> > cd_c_b, const hier::Box& pbox)
{
   double* ptr_rhs = cd_rhs->getPointer();
   double* ptr_temp = cd_temperature->getPointer();
   double* ptr_phi = cd_phi->getPointer();
   double* ptr_eta = NULL;
   double* ptr_f_l = cd_f_l->getPointer();
   double* ptr_f_a = cd_f_a->getPointer();
   double* ptr_f_b = NULL;
   double* ptr_c_l = cd_c_l->getPointer();
   double* ptr_c_a = cd_c_a->getPointer();
   double* ptr_c_b = NULL;

   if (d_with_third_phase) {
      ptr_eta = cd_eta->getPointer();
      ptr_f_b = cd_f_b->getPointer();
      ptr_c_b = cd_c_b->getPointer();
   }

   const hier::Box& rhs_gbox = cd_rhs->getGhostBox();
   int imin_rhs = rhs_gbox.lower(0);
   int jmin_rhs = rhs_gbox.lower(1);
   int jp_rhs = rhs_gbox.numberCells(0);
   int kmin_rhs = 0;
   int kp_rhs = 0;
#if (NDIM == 3)
   kmin_rhs = rhs_gbox.lower(2);
   kp_rhs = jp_rhs * rhs_gbox.numberCells(1);
#endif

   const hier::Box& temp_gbox = cd_temperature->getGhostBox();
   int imin_temp = temp_gbox.lower(0);
   int jmin_temp = temp_gbox.lower(1);
   int jp_temp = temp_gbox.numberCells(0);
   int kmin_temp = 0;
   int kp_temp = 0;
#if (NDIM == 3)
   kmin_temp = temp_gbox.lower(2);
   kp_temp = jp_temp * temp_gbox.numberCells(1);
#endif

   // Assuming phi, eta, and concentration all have same box
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

   // Assuming f_l, f_a, and f_b all have same box
   const hier::Box& f_i_gbox = cd_f_l->getGhostBox();
   int imin_f_i = f_i_gbox.lower(0);
   int jmin_f_i = f_i_gbox.lower(1);
   int jp_f_i = f_i_gbox.numberCells(0);
   int kmin_f_i = 0;
   int kp_f_i = 0;
#if (NDIM == 3)
   kmin_f_i = f_i_gbox.lower(2);
   kp_f_i = jp_f_i * f_i_gbox.numberCells(1);
#endif

   // Assuming c_l, c_a, and c_b all have same box
   const hier::Box& c_i_gbox = cd_c_l->getGhostBox();
   int imin_c_i = c_i_gbox.lower(0);
   int jmin_c_i = c_i_gbox.lower(1);
   int jp_c_i = c_i_gbox.numberCells(0);
   int kmin_c_i = 0;
   int kp_c_i = 0;
#if (NDIM == 3)
   kmin_c_i = c_i_gbox.lower(2);
   kp_c_i = jp_c_i * c_i_gbox.numberCells(1);
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

   for (int kk = kmin; kk <= kmax; kk++) {
      for (int jj = jmin; jj <= jmax; jj++) {
         for (int ii = imin; ii <= imax; ii++) {

            const int idx_rhs = (ii - imin_rhs) + (jj - jmin_rhs) * jp_rhs +
                                (kk - kmin_rhs) * kp_rhs;

            const int idx_temp = (ii - imin_temp) + (jj - jmin_temp) * jp_temp +
                                 (kk - kmin_temp) * kp_temp;

            const int idx_pf = (ii - imin_pf) + (jj - jmin_pf) * jp_pf +
                               (kk - kmin_pf) * kp_pf;

            const int idx_f_i = (ii - imin_f_i) + (jj - jmin_f_i) * jp_f_i +
                                (kk - kmin_f_i) * kp_f_i;

            const int idx_c_i = (ii - imin_c_i) + (jj - jmin_c_i) * jp_c_i +
                                (kk - kmin_c_i) * kp_c_i;

            double t = ptr_temp[idx_temp];
            double phi = ptr_phi[idx_pf];
            double f_l = ptr_f_l[idx_f_i];
            double f_a = ptr_f_a[idx_f_i];
            double f_b = 0.0;
            double c_l = ptr_c_l[idx_c_i];
            double c_a = ptr_c_a[idx_c_i];
            double c_b = 0.0;

            double mu = computeMuA(t, c_a);

            double hphi_prime = hprime(phi);

            double heta = 0.0;

            if (d_with_third_phase) {
               double eta = ptr_eta[idx_pf];
               f_b = ptr_f_b[idx_f_i];
               c_b = ptr_c_b[idx_c_i];

               heta = hprime(eta);
            }

            ptr_rhs[idx_rhs] +=
                hphi_prime * ((f_l - (1.0 - heta) * f_a - heta * f_b) -
                              mu * (c_l - (1.0 - heta) * c_a - heta * c_b));
         }
      }
   }
}

//=======================================================================

template <class FreeEnergyFunctionType>
double CALPHADFreeEnergyStrategyBinary<FreeEnergyFunctionType>::computeMuA(
    const double t, const double c)
{
   double mu;
   d_calphad_fenergy->computeDerivFreeEnergy(t, &c, PhaseIndex::phaseA, &mu);
   mu *= d_mv_strategy->computeInvMolarVolume(t, &c, PhaseIndex::phaseA);

   return mu;
}

//=======================================================================

template <class FreeEnergyFunctionType>
double CALPHADFreeEnergyStrategyBinary<FreeEnergyFunctionType>::computeMuL(
    const double t, const double c)
{
   double mu;
   d_calphad_fenergy->computeDerivFreeEnergy(t, &c, PhaseIndex::phaseL, &mu);
   mu *= d_mv_strategy->computeInvMolarVolume(t, &c, PhaseIndex::phaseL);

   return mu;
}

//=======================================================================

template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<FreeEnergyFunctionType>::
    addDrivingForceEta(const double time, hier::Patch& patch,
                       const int temperature_id, const int phase_id,
                       const int eta_id, const int conc_id, const int f_l_id,
                       const int f_a_id, const int f_b_id, const int rhs_id)
{
   (void)time;

   assert(conc_id >= 0);
   assert(phase_id >= 0);
   assert(f_l_id >= 0);
   assert(f_a_id >= 0);
   assert(rhs_id >= 0);
   assert(temperature_id >= 0);
   assert(eta_id >= 0);
   assert(f_b_id >= 0);

   std::shared_ptr<pdat::CellData<double> > phase(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(phase_id)));
   assert(phase);

   std::shared_ptr<pdat::CellData<double> > eta(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(eta_id)));
   assert(eta);

   std::shared_ptr<pdat::CellData<double> > t(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(temperature_id)));
   assert(t);

   std::shared_ptr<pdat::CellData<double> > f_l(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(f_l_id)));
   assert(f_l);

   std::shared_ptr<pdat::CellData<double> > f_a(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(f_a_id)));
   assert(f_a);

   std::shared_ptr<pdat::CellData<double> > f_b(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(f_b_id)));
   assert(f_b);

   std::shared_ptr<pdat::CellData<double> > c_l(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(d_conc_l_id)));
   assert(c_l);

   std::shared_ptr<pdat::CellData<double> > c_a(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(d_conc_a_id)));
   assert(c_a);

   std::shared_ptr<pdat::CellData<double> > c_b(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(d_conc_b_id)));
   assert(c_b);

   std::shared_ptr<pdat::CellData<double> > rhs(
       SAMRAI_SHARED_PTR_CAST<pdat::CellData<double>, hier::PatchData>(
           patch.getPatchData(rhs_id)));
   assert(rhs);

   const hier::Box& pbox = patch.getBox();

   addDrivingForceEta(rhs, t, phase, eta, f_l, f_a, f_b, c_l, c_a, c_b, pbox);
}

//=======================================================================

template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<FreeEnergyFunctionType>::
    addDrivingForceEta(std::shared_ptr<pdat::CellData<double> > cd_rhs,
                       std::shared_ptr<pdat::CellData<double> > cd_temperature,
                       std::shared_ptr<pdat::CellData<double> > cd_phi,
                       std::shared_ptr<pdat::CellData<double> > cd_eta,
                       std::shared_ptr<pdat::CellData<double> > cd_f_l,
                       std::shared_ptr<pdat::CellData<double> > cd_f_a,
                       std::shared_ptr<pdat::CellData<double> > cd_f_b,
                       std::shared_ptr<pdat::CellData<double> > cd_c_l,
                       std::shared_ptr<pdat::CellData<double> > cd_c_a,
                       std::shared_ptr<pdat::CellData<double> > cd_c_b,
                       const hier::Box& pbox)
{
   double* ptr_rhs = cd_rhs->getPointer();

   const double* const ptr_temp = cd_temperature->getPointer();
   const double* const ptr_phi = cd_phi->getPointer();
   const double* const ptr_eta = cd_eta->getPointer();
   const double* const ptr_f_a = cd_f_a->getPointer();
   const double* const ptr_f_b = cd_f_b->getPointer();
   const double* const ptr_c_a = cd_c_a->getPointer();
   const double* const ptr_c_b = cd_c_b->getPointer();

   const hier::Box& rhs_gbox = cd_rhs->getGhostBox();
   int imin_rhs = rhs_gbox.lower(0);
   int jmin_rhs = rhs_gbox.lower(1);
   int jp_rhs = rhs_gbox.numberCells(0);
   int kmin_rhs = 0;
   int kp_rhs = 0;
#if (NDIM == 3)
   kmin_rhs = rhs_gbox.lower(2);
   kp_rhs = jp_rhs * rhs_gbox.numberCells(1);
#endif

   const hier::Box& temp_gbox = cd_temperature->getGhostBox();
   int imin_temp = temp_gbox.lower(0);
   int jmin_temp = temp_gbox.lower(1);
   int jp_temp = temp_gbox.numberCells(0);
   int kmin_temp = 0;
   int kp_temp = 0;
#if (NDIM == 3)
   kmin_temp = temp_gbox.lower(2);
   kp_temp = jp_temp * temp_gbox.numberCells(1);
#endif

   // Assuming phi, eta, and concentration all have same box
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

   // Assuming f_l, f_a, and f_b all have same box
   const hier::Box& f_i_gbox = cd_f_l->getGhostBox();
   int imin_f_i = f_i_gbox.lower(0);
   int jmin_f_i = f_i_gbox.lower(1);
   int jp_f_i = f_i_gbox.numberCells(0);
   int kmin_f_i = 0;
   int kp_f_i = 0;
#if (NDIM == 3)
   kmin_f_i = f_i_gbox.lower(2);
   kp_f_i = jp_f_i * f_i_gbox.numberCells(1);
#endif

   // Assuming c_l, c_a, and c_b all have same box
   const hier::Box& c_i_gbox = cd_c_l->getGhostBox();
   int imin_c_i = c_i_gbox.lower(0);
   int jmin_c_i = c_i_gbox.lower(1);
   int jp_c_i = c_i_gbox.numberCells(0);
   int kmin_c_i = 0;
   int kp_c_i = 0;
#if (NDIM == 3)
   kmin_c_i = c_i_gbox.lower(2);
   kp_c_i = jp_c_i * c_i_gbox.numberCells(1);
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
   const char interpf = energyInterpChar(d_energy_interp_func_type);

   for (int kk = kmin; kk <= kmax; kk++) {
      for (int jj = jmin; jj <= jmax; jj++) {
         for (int ii = imin; ii <= imax; ii++) {

            const int idx_rhs = (ii - imin_rhs) + (jj - jmin_rhs) * jp_rhs +
                                (kk - kmin_rhs) * kp_rhs;

            const int idx_temp = (ii - imin_temp) + (jj - jmin_temp) * jp_temp +
                                 (kk - kmin_temp) * kp_temp;

            const int idx_pf = (ii - imin_pf) + (jj - jmin_pf) * jp_pf +
                               (kk - kmin_pf) * kp_pf;

            const int idx_f_i = (ii - imin_f_i) + (jj - jmin_f_i) * jp_f_i +
                                (kk - kmin_f_i) * kp_f_i;

            const int idx_c_i = (ii - imin_c_i) + (jj - jmin_c_i) * jp_c_i +
                                (kk - kmin_c_i) * kp_c_i;

            const double t = ptr_temp[idx_temp];
            const double phi = ptr_phi[idx_pf];
            const double eta = ptr_eta[idx_pf];
            const double f_a = ptr_f_a[idx_f_i];
            const double f_b = ptr_f_b[idx_f_i];
            const double c_a = ptr_c_a[idx_c_i];
            const double c_b = ptr_c_b[idx_c_i];

            const double mu = computeMuA(t, c_a);
            // const double mu = 0.;

            const double hphi = INTERP_FUNC(phi, &interpf);
            const double heta_prime = DERIV_INTERP_FUNC(eta, &interpf);
            ptr_rhs[idx_rhs] +=
                hphi * heta_prime * ((f_a - f_b) - mu * (c_a - c_b));
         }
      }
   }
}


//=======================================================================

template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<FreeEnergyFunctionType>::
    defaultComputeSecondDerivativeEnergyPhaseL(const double temp,
                                               const std::vector<double>& c_l,
                                               std::vector<double>& d2fdc2,
                                               const bool use_internal_units)
{
   d_calphad_fenergy->computeSecondDerivativeFreeEnergy(temp, &c_l[0],
                                                        PhaseIndex::phaseL,
#ifdef HAVE_THERMO4PFM
                                                        d2fdc2.data()
#else
                                                        d2fdc2
#endif
   );

   if (use_internal_units)
      d2fdc2[0] *= d_mv_strategy->computeInvMolarVolume(temp, &c_l[0],
                                                        PhaseIndex::phaseL);
}

//=======================================================================

template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<FreeEnergyFunctionType>::
    defaultComputeSecondDerivativeEnergyPhaseA(const double temp,
                                               const std::vector<double>& c_a,
                                               std::vector<double>& d2fdc2,
                                               const bool use_internal_units)
{
   d_calphad_fenergy->computeSecondDerivativeFreeEnergy(temp, &c_a[0],
                                                        PhaseIndex::phaseA,
#ifdef HAVE_THERMO4PFM
                                                        d2fdc2.data()
#else
                                                        d2fdc2
#endif
   );

   if (use_internal_units)
      d2fdc2[0] *= d_mv_strategy->computeInvMolarVolume(temp, &c_a[0],
                                                        PhaseIndex::phaseA);
}

//=======================================================================
#ifndef HAVE_THERMO4PFM
template <class FreeEnergyFunctionType>
void CALPHADFreeEnergyStrategyBinary<FreeEnergyFunctionType>::
    defaultComputeSecondDerivativeEnergyPhaseB(const double temp,
                                               const std::vector<double>& c_b,
                                               std::vector<double>& d2fdc2,
                                               const bool use_internal_units)
{
   d_calphad_fenergy->computeSecondDerivativeFreeEnergy(temp, &c_b[0],
                                                        PhaseIndex::phaseB,
                                                        d2fdc2);

   if (use_internal_units)
      d2fdc2[0] *= d_mv_strategy->computeInvMolarVolume(temp, &c_b[0],
                                                        PhaseIndex::phaseB);
}
#endif

template class CALPHADFreeEnergyStrategyBinary<
    CALPHADFreeEnergyFunctionsBinary>;
#ifdef HAVE_THERMO4PFM
template class CALPHADFreeEnergyStrategyBinary<
    CALPHADFreeEnergyFunctionsBinary2Ph1Sl>;
#endif
