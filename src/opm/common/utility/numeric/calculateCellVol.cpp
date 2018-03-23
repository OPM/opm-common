/*
  Copyright 2018 Statoil ASA.

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <algorithm>    
#include <opm/common/utility/numeric/calculateCellVol.hpp>


/* 
    Cell volume calculation based on following publication:
   
    D. K Pointing, Corner Point Geometry in Reservoir Simulation , 
    ECMOR I - 1st European Conference on the Mathematics of Oil Recovery, 
    1989 
*/
 

/*
    The expressions {C(0,0,0),..C(1,1,1)} have a nice interpretation in
    terms of a type of multipole expansion - the last four terms are differences
    in the lengths of cell face diagonals and of the diagonals across the
    cell. For a cubical block only the first four terms would exist.

*/

double C(const double* r, int i1, int i2, int i3){

   double temp;

   temp = 0.0;
   
   if ((i1 == 0) && (i2 == 0) && (i3 == 0)){
      temp = r[0];
   }

   if ((i1 == 1) && (i2 == 0) && (i3 == 0)){
      temp = r[1] - r[0];
   }

   if ((i1 == 0) && (i2 == 1) && (i3 == 0)){
      temp = r[2] - r[0];
   }

   if ((i1 == 0) && (i2 == 0) && (i3 == 1)){
      temp = r[4]-r[0];
   }

   if ((i1 == 1) && (i2 == 1) && (i3 == 0)){
      temp = r[3] + r[0] - r[2] - r[1];
   }

   if ((i1 == 0) && (i2 == 1) && (i3 == 1)){
      temp = r[6] + r[0] - r[4] - r[2];
   }

   if ((i1 == 1) && (i2 == 0) && (i3 == 1)){
      temp = r[5] + r[0] - r[4] - r[1];
   }

   if ((i1 == 1) && (i2 == 1) && (i3 == 1)){
      temp = r[7] + r[4] + r[2] + r[1] - r[6] - r[5] - r[3] - r[0];
   }

   return temp;

}


double perm123sign(int i1, int i2, int i3){

   double temp;
   
   if ((i1 ==1 ) && (i2==2) && (i3 == 3)){
      temp = 1.0;
   }
   
   if ((i1 == 1) && (i2==3) && (i3 == 2)){
      temp = -1.0;
   }
   
   if ((i1 == 2) && (i2 == 1) && (i3 == 3)){
      temp = -1.0;
   }
   
   if ((i1 == 2) && (i2 == 3) && (i3 == 1)){
      temp = 1.0;
   }
   
   if ((i1 == 3) && (i2 == 1) && (i3 == 2)){
      temp = 1.0;
   }
   
   if ((i1 == 3) && (i2 == 2) && (i3 == 1)){
      temp = -1.0;
   }
   
   return temp;
}

double calculateCellVol(const std::vector<double>& X, const std::vector<double>& Y, const std::vector<double>& Z){
      
  double volume = 0.0;
  const double* vect[3]; 
  int permutation[] = {1, 2, 3};

  do {
     for (int j = 0; j < 3; ++j){
        if (permutation[j] == 1){
	   vect[j] = X.data();
	} else if (permutation[j] == 2){
	   vect[j] = Y.data();
	} else if (permutation[j] == 3){
	   vect[j] = Z.data();
        } 
     }
    
     for (int pb = 0; pb < 2; ++pb){
       for (int pg = 0; pg < 2; ++pg){
   	 for (int qa = 0; qa < 2; ++qa){
   	   for (int qg = 0; qg < 2; ++qg){
   	     for (int ra = 0; ra < 2; ++ra){
   	       for (int rb = 0; rb < 2; ++rb){
                 const double cprod = C(vect[0], 1, pb, pg)*C(vect[1], qa, 1, qg)*C(vect[2], ra, rb, 1);
                 const double denom = (qa+ra+1) * (pb+rb+1) * (pg+qg+1);
                 volume += perm123sign(permutation[0], permutation[1], permutation[2]) * cprod / denom;
   	       }
   	     }
   	   }
   	 }
       }
     }  
   
  } while (std::next_permutation(std::begin(permutation), std::end(permutation)));

  return std::fabs(volume);
}



