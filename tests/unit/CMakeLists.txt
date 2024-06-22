foreach( _test
        grid_healpix
        grid_lambert
        grid_polar_stereographic )
    ecbuild_add_test(
        TARGET  eccodes_tests_unit_${_test}
        SOURCES ${_test}.cc
        LIBS    eccodes )
endforeach()

