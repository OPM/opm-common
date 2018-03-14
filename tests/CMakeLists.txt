configure_file(spe3/SPE3CASE1.DATA spe3/SPE3CASE1.DATA COPYONLY)
configure_file(data/CORNERPOINT_ACTNUM.DATA data/CORNERPOINT_ACTNUM.DATA COPYONLY)
configure_file(data/JFUNC.DATA data/JFUNC.DATA COPYONLY)
configure_file(data/schedule/part1.sch data/schedule/part1.sch COPYONLY)
configure_file(data/schedule/part2.sch data/schedule/part2.sch COPYONLY)
configure_file(data/schedule/part3.sch data/schedule/part3.sch COPYONLY)
configure_file(data/schedule/fragment.sch data/schedule/fragment.sch COPYONLY)


foreach(prog time_vector completions deck group_tree grupnet parse_deck parse state props schedule wells)
    add_python_test(${prog} ${prog}.py)
endforeach()
