configure_file(spe3/SPE3CASE1.DATA spe3/SPE3CASE1.DATA COPYONLY)
configure_file(data/CORNERPOINT_ACTNUM.DATA data/CORNERPOINT_ACTNUM.DATA COPYONLY)
configure_file(data/JFUNC.DATA data/JFUNC.DATA COPYONLY)


foreach(prog completions deck group_tree grupnet parse_deck parse state props schedule wells)

    add_python_test(${prog} ${prog}.py)
endforeach()
