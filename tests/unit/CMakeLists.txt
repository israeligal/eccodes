ecbuild_add_test(
    TARGET  eccodes_tests_unit_gribspec_N32
    COMMAND grib_get_gridspec
    ARGS    ${CMAKE_CURRENT_SOURCE_DIR}/../../samples/reduced_gg_sfc_grib2.tmpl )

set_tests_properties(
    eccodes_tests_unit_gribspec_N32
    PROPERTIES
    PASS_REGULAR_EXPRESSION [=[{"grid":"N32"}]=] )

foreach( _test
        grid_healpix
        grid_lambert
        grid_polar_stereographic )
    ecbuild_add_test(
        TARGET  eccodes_tests_unit_${_test}
        SOURCES ${_test}.cc
        LIBS    eccodes )
endforeach()

