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
#include <iostream>

#include "NormalNoise.h"
#include "UniformNoise.h"


int main(int argc, char* argv[])
{
   std::cout << "Test noise generation." << std::endl;

   // 1st test
   {
      std::cout << "Normal distributed noise..." << std::endl;
      NormalNoise& noise(*(NormalNoise::instance()));

      int ntotal = 1000;
      int count = 0;
      double avg = 0.;
      for (int i = 0; i < ntotal; i++) {
         double val = noise.gen();
         // std::cout<<val<<endl;
         if (fabs(val) <= 1.) count++;
         avg += val;
      }
      avg /= (double)ntotal;

      std::cout << count << " out of " << ntotal
                << " values are between -1 and 1." << std::endl;
      std::cout << "Average value is " << avg << std::endl;

      // 68% of values should be between -1 and 1
      if ((double)count > 0.71 * (double)ntotal ||
          (double)count < 0.65 * (double)ntotal) {
         std::cerr << "TEST NormalNoise failed!!!" << std::endl;
         return 1;
      }

      if (fabs(avg) > 0.04) {
         std::cerr << "TEST average NormalNoise failed!!!" << std::endl;
         return 1;
      }
   }

   // 2nd test
   {
      std::cout << "Uniform distributed noise..." << std::endl;
      UniformNoise& noise(*(UniformNoise::instance(42u)));

      int ntotal = 1000;
      int count = 0;
      double avg = 0.;
      for (int i = 0; i < ntotal; i++) {
         double val = noise.gen();
         if (fabs(val) <= 0.5) count++;
         avg += val;
      }
      avg /= ntotal;

      std::cout << count << " out of " << ntotal
                << " values are between -0.5 and 0.5" << std::endl;
      std::cout << "Average value is " << avg << std::endl;

      if (count < ntotal) {
         std::cerr << "TEST UniformNoise failed!!!" << std::endl;
         return 1;
      }

      if (fabs(avg) > 0.02) {
         std::cerr << "TEST average UniformNoise failed!!!" << std::endl;
         return 1;
      }
   }

   std::cout << "TEST successful!" << std::endl;

   return 0;
}
