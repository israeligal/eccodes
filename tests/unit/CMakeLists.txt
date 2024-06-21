ecbuild_add_test(
    TARGET  eccodes_t_gribspec
    SOURCES gribspec.cc
    LIBS    eccodes
    ARGS    ${CMAKE_CURRENT_SOURCE_DIR}../../samples/reduced_gg_sfc_grib2.tmpl
            ${CMAKE_CURRENT_SOURCE_DIR}../reduced_gg_sfc_grib2.tmpl.json )

foreach( _test
        grid_healpix
        grid_lambert
        grid_polar_stereographic )
    ecbuild_add_test(TARGET eccodes_tests_unit_${_test} SOURCES ${_test}.cc LIBS eccodes )
endforeach()

