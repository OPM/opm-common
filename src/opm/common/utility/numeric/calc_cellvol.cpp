#include <opm/common/utility/numeric/calc_cellvol.hpp>


/* 
    Cell volume calculation based on following publication:
   
    D. K Pointing, Corner Point Geometry in Reservoir Simulation , 
    ECMOR I - 1st European Conference on the Mathematics of Oil Recovery, 
    1989 
*/
 
using namespace std;




double C(const std::vector<double>& r,int i1,int i2,int i3){

   double temp;

   temp=0.0;
   
   if ((i1==0) && (i2==0) && (i3==0)){
      temp=r[0];
   }

   if ((i1==1) && (i2==0) && (i3==0)){
      temp=r[1]-r[0];
   }

   if ((i1==0) && (i2==1) && (i3==0)){
      temp=r[2]-r[0];
   }

   if ((i1==0) && (i2==0) && (i3==1)){
      temp=r[4]-r[0];
   }

   if ((i1==1) && (i2==1) && (i3==0)){
      temp=r[3]+r[0]-r[2]-r[1];
   }

   if ((i1==0) && (i2==1) && (i3==1)){
      temp=r[6]+r[0]-r[4]-r[2];
   }

   if ((i1==1) && (i2==0) && (i3==1)){
      temp=r[5]+r[0]-r[4]-r[1];
   }

   if ((i1==1) && (i2==1) && (i3==1)){
      temp=r[7]+r[4]+r[2]+r[1]-r[6]-r[5]-r[3]-r[0];
   }

   return temp;

}

double perm123(int i1,int i2,int i3){

   int temp;
   
   if ((i1==1) && (i2==2) && (i3==3)){
      temp=0;
   }
   
   if ((i1==1) && (i2==3) && (i3==2)){
      temp=1;
   }
   
   if ((i1==2) && (i2==1) && (i3==3)){
      temp=1;
   }
   
   if ((i1==2) && (i2==3) && (i3==1)){
      temp=2;
   }
   
   if ((i1==3) && (i2==1) && (i3==2)){
      temp=2;
   }
   
   if ((i1==3) && (i2==2) && (i3==1)){
      temp=3;
   }

   
   return temp;

}



double calc_cell_volume(const std::vector<double>& X,const std::vector<double>& Y,const std::vector<double>& Z){
      
  int pb,pg,qa,qg,ra,rb;

  double volume=0.0;
   
  /* permutasjon (1,2,3)  */
    
  for (pb=0;pb<2;pb++){
    for (pg=0;pg<2;pg++){
      for (qa=0;qa<2;qa++){
  	for (qg=0;qg<2;qg++){
  	  for (ra=0;ra<2;ra++){
  	    for (rb=0;rb<2;rb++){
  	        volume=volume+(pow(-1.0,perm123(1,2,3))*(C(X,1,pb,pg)*C(Y,qa,1,qg)*C(Z,ra,rb,1))/((qa+ra+1)*(pb+rb+1)*(pg+qg+1)));
  	    }
          }
        }
      }
    }
  }  
  
  /* permutasjon (1,3,2) */
  
  for (pb=0;pb<2;pb++){
    for (pg=0;pg<2;pg++){
      for (qa=0;qa<2;qa++){
  	for (qg=0;qg<2;qg++){
  	  for (ra=0;ra<2;ra++){
  	    for (rb=0;rb<2;rb++){
        	 volume=volume+(pow(-1.0,perm123(1,3,2))*(C(X,1,pb,pg)*C(Z,qa,1,qg)*C(Y,ra,rb,1))/((qa+ra+1)*(pb+rb+1)*(pg+qg+1)));
  	    }
          }
        }
      }
    }
  }  
  
  /* permutasjon (2,1,3) */  
  
  for (pb=0;pb<2;pb++){
    for (pg=0;pg<2;pg++){
      for (qa=0;qa<2;qa++){
  	for (qg=0;qg<2;qg++){
  	  for (ra=0;ra<2;ra++){
  	    for (rb=0;rb<2;rb++){
        	volume=volume+pow(-1.0,perm123(2,1,3))*(C(Y,1,pb,pg)*C(X,qa,1,qg)*C(Z,ra,rb,1))/((qa+ra+1)*(pb+rb+1)*(pg+qg+1));
  	    }
          }
        }
      }
    }
  }  

  /* permutasjon (2,3,1)  */ 
  
  for (pb=0;pb<2;pb++){
    for (pg=0;pg<2;pg++){
      for (qa=0;qa<2;qa++){
  	for (qg=0;qg<2;qg++){
  	  for (ra=0;ra<2;ra++){
  	    for (rb=0;rb<2;rb++){
        	volume=volume+pow(-1.0,perm123(2,3,1))*(C(Y,1,pb,pg)*C(Z,qa,1,qg)*C(X,ra,rb,1))/((qa+ra+1)*(pb+rb+1)*(pg+qg+1));
  	    }
          }
        }
      }
    }
  }  

  /* permutasjon (3,1,2) */ 
  
  for (pb=0;pb<2;pb++){
    for (pg=0;pg<2;pg++){
      for (qa=0;qa<2;qa++){
  	for (qg=0;qg<2;qg++){
  	  for (ra=0;ra<2;ra++){
  	    for (rb=0;rb<2;rb++){
        	volume=volume+pow(-1.0,perm123(3,1,2))*(C(Z,1,pb,pg)*C(X,qa,1,qg)*C(Y,ra,rb,1))/((qa+ra+1)*(pb+rb+1)*(pg+qg+1));
  	    }
          }
        }
      }
    }
  }  
  
  /* permutasjon (3,2,1) */ 
  
  for (pb=0;pb<2;pb++){
    for (pg=0;pg<2;pg++){
      for (qa=0;qa<2;qa++){
  	for (qg=0;qg<2;qg++){
  	  for (ra=0;ra<2;ra++){
  	    for (rb=0;rb<2;rb++){
        	volume=volume+pow(-1.0,perm123(3,2,1))*(C(Z,1,pb,pg)*C(Y,qa,1,qg)*C(X,ra,rb,1))/((qa+ra+1)*(pb+rb+1)*(pg+qg+1));
  	    }
          }
        }
      }
    }
  }  
  
  return fabs(volume);

}

