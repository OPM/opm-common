/*
   Copyright (C) 2011  Statoil ASA, Norway.
   The file 'view_summary.c' is part of ERT - Ensemble based Reservoir Tool.

   ERT is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ERT is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
   for more details.
*/

// This script is based on/inspired by  view_summay.c found on  

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <vector> 
#include <iostream>
#include <algorithm>

#include <ert/util/ert_api_config.h>

#ifdef ERT_HAVE_GETOPT
#include <getopt.h>
#endif

#include <ert/util/util.h>
#include <ert/util/stringlist.h>

#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/ecl_sum.h>

#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <locale.h>


#include <ert/util/hash.h>
#include <ert/util/set.h>
#include <ert/util/util.h>
#include <ert/util/vector.h>
#include <ert/util/int_vector.h>
#include <ert/util/bool_vector.h>
#include <ert/util/time_t_vector.h>
#include <ert/util/time_interval.h>

#include <ert/ecl/ecl_util.h>
#include <ert/ecl/ecl_smspec.h>
#include <ert/ecl/ecl_sum_data.h>
#include <ert/ecl/smspec_node.h>

#include <opm/common/ErrorMacros.hpp>


#define DATE_HEADER         "-- Days   dd/mm/yyyy   "
#define DATE_STRING_LENGTH 128

  struct ecl_sum_struct {
  UTIL_TYPE_ID_DECLARATION;
  ecl_smspec_type   * smspec;     /* Internalized version of the SMSPEC file. */
  ecl_sum_data_type * data;       /* The data - can be NULL. */


  bool                fmt_case;
  bool                unified;
  char              * key_join_string;
  char              * path;       /* The path - as given for the case input. Can be NULL for cwd. */
  char              * abs_path;   /* Absolute path. */
  char              * base;       /* Only the basename. */
  char              * ecl_case;   /* This is the current case, with optional path component. == path + base*/
  char              * ext;        /* Only to support selective loading of formatted|unformatted and unified|multiple. (can be NULL) */
};

static void __ecl_sum_get_value_line( const ecl_sum_type * ecl_sum , int internal_index , const bool_vector_type * has_var , const int_vector_type * var_index , const ecl_sum_fmt_type * fmt, std::vector<double> &res_vec, std::vector<double> &time_vec) {
   
  
  time_vec.push_back(ecl_sum_iget_sim_days(ecl_sum , internal_index ));
  
 

  
  {
    int ivar;
    for (ivar = 0; ivar < int_vector_size( var_index ); ivar++) {
      if (bool_vector_iget( has_var , ivar )) {
  	res_vec.push_back(ecl_sum_iget(ecl_sum , internal_index, int_vector_iget( var_index , ivar )));
      }
    }
  }

}




void ecl_sum_get_value(const ecl_sum_type * ecl_sum , const stringlist_type * var_list , bool report_only , const ecl_sum_fmt_type * fmt, std::vector<double> &res_vec, std::vector<double> &time_vec) {
  bool_vector_type  * has_var   = bool_vector_alloc( stringlist_get_size( var_list ), false );
  int_vector_type   * var_index = int_vector_alloc( stringlist_get_size( var_list ), -1 );
  char * current_locale = NULL;
  if (fmt->locale != NULL)
    current_locale = setlocale(LC_NUMERIC , fmt->locale);

  {
    if (ecl_sum_has_general_var( ecl_sum , stringlist_iget( var_list , 0) )) {
      bool_vector_iset( has_var , 0, true );
      int_vector_iset( var_index , 0 , ecl_sum_get_general_var_params_index( ecl_sum , stringlist_iget( var_list , 0) ));
    } else {
      OPM_THROW(std::invalid_argument, "Keyword not found in summary file.");
      fprintf(stderr,"** Warning: could not find variable: \'%s\' in summary file \n", stringlist_iget( var_list , 0));
      // bool_vector_iset( has_var , ivar , false );
    }
    
  }

    

  if (report_only) {
    int first_report = ecl_sum_get_first_report_step( ecl_sum );
    int last_report  = ecl_sum_get_last_report_step( ecl_sum );
    int report;
    for (report = first_report; report <= last_report; report++) {
 
      if (ecl_sum_data_has_report_step(ecl_sum->data , report)) {
        int time_index;
        time_index = ecl_sum_data_iget_report_end( ecl_sum->data , report );
        __ecl_sum_get_value_line( ecl_sum, time_index , has_var , var_index , fmt, res_vec, time_vec);
      }
    }
  } else {
    int time_index;
    for (time_index = 0; time_index < ecl_sum_get_data_length( ecl_sum ); time_index++){
      __ecl_sum_get_value_line( ecl_sum, time_index , has_var , var_index , fmt, res_vec, time_vec);
    }
  }

  int_vector_free( var_index );
  bool_vector_free( has_var );
  if (current_locale != NULL)
    setlocale( LC_NUMERIC , current_locale);
}
#undef DATE_STRING_LENGTH







void calculateDeviations(std::vector<double> *absdev_vec,std::vector<double> *reldev_vec, double val1, double val2){
  double abs_deviation, rel_deviation;
  
      if(val1 >= 0 && val2 >= 0){
	abs_deviation =  std::max(val1, val2) - std::min(val1, val2);
      
	absdev_vec->push_back(abs_deviation);
	if(val1 == 0 && val2 == 0){}
	else{
	  rel_deviation = abs_deviation/double(std::max(val1, val2));
	  reldev_vec->push_back(rel_deviation);
	}
      }
     
}

double linearPolation(double check_value, double check_value_prev , double time_array[3]){
  double time_check = time_array[0], time_check_prev = time_array[1], time_reference = time_array[2], sloap, factor, lp_value;
  sloap = (check_value - check_value_prev)/double(time_check - time_check_prev);
  factor = (time_reference - time_check_prev)/double(time_check - time_check_prev);
  lp_value = check_value_prev + factor*sloap; 
  return lp_value;
  
}

void compareValuesDeviations(std::vector<double>* data1, std::vector<double>* data2, std::vector<double>* absdev_vec,std::vector<double> *reldev_vec, std::vector<double> *time_vec1, std::vector<double> *time_vec2){


  std::vector<double> *referance_vector, *checking_vector, *ref_data_vec, *check_data_vec;
  // Create reference and checking vectors. Assumes that the time vectors start and stops at the same time.
  // Choose the time vector with less elements to be the reference vector. 
  if (time_vec1->size() <= time_vec2->size()){
    referance_vector = time_vec1; // time vector
    ref_data_vec = data1; //data vector
    checking_vector = time_vec2;
    check_data_vec = data2;
    
  }
  else{
    referance_vector = time_vec2;
    ref_data_vec = data2;
    checking_vector = time_vec1;
    check_data_vec = data1;
  }
 
  int jvar = 0 ;
  
  for (int ivar = 0; ivar < referance_vector->size(); ivar++){
    while (true){
      if((*referance_vector)[ivar] == (*checking_vector)[jvar]){
	//Check without linear interpolation
	calculateDeviations(absdev_vec, reldev_vec, (*ref_data_vec)[ivar], (*check_data_vec)[jvar]); 
	break;
      }
      else if((*referance_vector)[ivar]<(*checking_vector)[jvar]){
	// Check with Linear polized arguments, jvar
	double time_array[3];
	time_array[0]= (*checking_vector)[jvar]; 
	time_array[1]= (*checking_vector)[jvar -1];
	time_array[2]= (*referance_vector)[ivar];
	double lp_value = linearPolation((*checking_vector)[jvar], (*checking_vector)[jvar-1],time_array);
	calculateDeviations(absdev_vec, reldev_vec, (*ref_data_vec)[ivar], lp_value);
	break; 
      }
      else{
	
	jvar++;
      }
     
    }
  }
}


 
 double vecMed(std::vector<double> *vec) {
   if(vec->empty()){ 
     return 0;
   }
    else {
        std::sort(vec->begin(), vec->end());
        if(vec->size() % 2 == 0)
	  return ((*vec)[vec->size()/2 - 1] + (*vec)[vec->size()/2]) / 2;
        else
	  return (*vec)[vec->size()/2];
    }
}







void compareValuesPresentResult(std::vector<double> *data1, std::vector<double> *data2, std::vector<double> *time_vec1, std::vector<double> *time_vec2, double rel_tolerance_max, double rel_tolerance_median){
  
  std::vector<double> absdev_vec, reldev_vec, *absdev_vec_ptr, *reldev_vec_ptr;
  absdev_vec_ptr = &absdev_vec;
  reldev_vec_ptr = &reldev_vec;
  compareValuesDeviations(data1, data2, absdev_vec_ptr,reldev_vec_ptr, time_vec1, time_vec2);
  
  if(absdev_vec_ptr->size() == 0 || reldev_vec_ptr->size() == 0){
      OPM_THROW(std::invalid_argument, "Relative or absolute deviation vectors are empty, something went wrong or the datasets are equal.");
  }
  
  double sum_absdev = (*absdev_vec_ptr)[0]; 
  double sum_reldev = (*reldev_vec_ptr)[0];
for(int it =0; it < absdev_vec_ptr->size()-1; it++){
  sum_absdev += (*absdev_vec_ptr)[it+1];
  sum_reldev += (*reldev_vec_ptr)[it+1];  
 }
 


 double avr_absdev, avr_reldev; 
 avr_absdev = sum_absdev/double(absdev_vec_ptr->size());
 avr_reldev = sum_reldev/double(reldev_vec_ptr->size());

 double medianValueAbs = vecMed(absdev_vec_ptr);
 double medianValueRel = vecMed(reldev_vec_ptr);

 // Vectors get sorted in vecMed, thus max value at the end of the vectors
double max_absdev, max_reldev; 
 max_absdev =  absdev_vec_ptr->back();
 max_reldev =  reldev_vec_ptr->back();

 if(max_reldev > rel_tolerance_max || medianValueRel > rel_tolerance_median ){
   std::cout << "The maximum relative deviation is " << max_reldev << ".  The tolerance level is " << rel_tolerance_max << std::endl; 
   std::cout << "The median relative deviation is " << medianValueRel << ".  The tolerance level is " << rel_tolerance_median << std::endl; 
   OPM_THROW(std::invalid_argument, "The deviation is too large." );
 }

 // It is possible to put som limits on the absolute deviation as well, which is not done here. 


/* //Possible to print some of the results
 printf("The maximum absolute deviation is: %f \n", max_absdev);
 printf("The maximum relative deviation is: %f \n", max_reldev);
 printf("\n");

printf("The meadian absolute deviation is: %f \n",medianValueAbs) ;
 printf("The meadian relative deviation is: %f \n",medianValueRel);
 printf("\n");

   printf("The average absolute deviation is %f \n",avr_absdev);
printf("The average relative deviation is %f \n",avr_reldev);
 printf("\n\n");

 printf("From the original %i reference data, there was/were %i deviation/s", std::min(data1->size(), data2->size()),absdev_vec_ptr->size());
 if(absdev_vec_ptr->size()/double(std::min(data1->size(), data2->size())) < 0.1){
     printf("The deviation rate is %f \n", absdev_vec_ptr->size()/double(std::min(data1->size(), data2->size())));
     printf("You might want to look into the data vectors. \n ");
   }
   else {
     printf("The deviation rate is > 0.1, the data should be ok. \n");
   }
 */  
   
  
}
  

int  main(int argc , char ** argv) {

  std::vector<double> res_vec1, res_vec2, time_vec1, time_vec2;
  std::vector<double> *res_vec1_ptr, *res_vec2_ptr, *time_vec1_ptr, *time_vec2_ptr;
  {
    bool           report_only     = false;
    bool           include_restart = true;
    int            arg_offset      = 1;
    


    {
      char         * data_file1 = argv[arg_offset];
      char         * data_file2 = argv[arg_offset + 1];
      ecl_sum_type * ecl_sum1;
      ecl_sum_type * ecl_sum2;


      ecl_sum1 = ecl_sum_fread_alloc_case__( data_file1 , ":" , include_restart);
      ecl_sum2 = ecl_sum_fread_alloc_case__( data_file2 , ":" , include_restart);
      /** If no keys have been presented the function will list available keys. */
     
      if (ecl_sum1 != NULL && ecl_sum2 != NULL) {
        
	  stringlist_type * keys = stringlist_alloc_new( );
	  ecl_sum_select_matching_general_var_list( ecl_sum1 , "*" , keys);
	  stringlist_sort(keys , NULL );
	  ecl_sum_fmt_type fmt;
	 
	  
	  //set tolerance level for relative tolerance max and the median relative tolereance 
	  double rel_tolerance_max = 0.5;
	  double rel_tolerance_median = 0.05;



	  for (int it = 0; it < stringlist_get_size(keys); it++){
	    stringlist_type * key_ = stringlist_alloc_new( );
	    stringlist_append_copy( key_ , stringlist_iget( keys , it )); 

	    ecl_sum_get_value(ecl_sum1, key_, report_only , &fmt, res_vec1, time_vec1);
	    ecl_sum_get_value(ecl_sum2, key_ , report_only , &fmt, res_vec2, time_vec2);
	    res_vec1_ptr = &res_vec1; 
	    res_vec2_ptr = &res_vec2; 
	    time_vec1_ptr = &time_vec1;
	    time_vec2_ptr = &time_vec2;
	    
     
	   

	    std::cout << std::endl << std::endl << std::endl; 
	    std::cout << "KEYWORD: " << stringlist_iget( keys , it ) << std::endl; 
	    compareValuesPresentResult(res_vec1_ptr, res_vec2_ptr, time_vec1_ptr, time_vec2_ptr, rel_tolerance_max, rel_tolerance_median);
	    res_vec1_ptr->clear();
	    res_vec2_ptr->clear();
	    time_vec1_ptr->clear();
	    time_vec2_ptr->clear();
	    stringlist_free(key_);
	  }
      
	  stringlist_free(keys);
	  

	   
 	
	//std::cout << "KEYWORD: " <<  arg_list[0] << std::endl; 
	//compareValuesPresentResult(res_vec1_ptr, res_vec2_ptr, time_vec1_ptr, time_vec2_ptr, 1000000000000000, 1);
	/*printf("\n\n");
	  printf("Quantity Q1:\n\n ");
	  // compareValuesPresentResult(q1vec1, q1vec2, time_vec1, time_vec2, 100000, 1);
	  printf("\n\n\n");
	  printf("Quantity Q2: ");
	  //  compareValuesPresentResult(q2vec1, q2vec2, time_vec1, time_vec2, 100000, 1);
	  printf("\n\n\n");
	  printf("Quantity Q3: ");
	  // compareValuesPresentResult(q3vec1, q3vec2, time_vec1, time_vec2, 100000, 1);	
	  printf("\n\n\n");
	*/
	  
	  
        }
        ecl_sum_free(ecl_sum1);
	ecl_sum_free(ecl_sum2);
      } 
        
  }
    
}

