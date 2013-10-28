function install_headers( header-list prefix ) 
   foreach (header in header-list)
       get_filename_component( path ${header} PATH )
       file( RELATIVE_PATH rel_path ${PROJECT_SOURCE_DIR} ${path})
       install ( FILES ${header}
	  DESTINATION ${prefix}/include/${rel_path} )
   endforeach()
endfunction()