if(HAVE_GEOGRAPY)
    foreach( _test
            grid_FESOM
            grid_ICON
            grid_ORCA
            grid_healpix
            grid_lambert
            grid_polar_stereographic )
        ecbuild_add_test(
            TARGET  eccodes_tests_unit_${_test}
            SOURCES ${_test}.cc
            LIBS    eccodes eckit_geo )
    endforeach()
endif()

