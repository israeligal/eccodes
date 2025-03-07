# data/bufr/CMakeLists.txt
# Download all the BUFR data and reference files from website
#

file(READ "bufr_data_files.txt" bufr_files_to_download)
# Convert file contents into a CMake list (where each element in the list
# is one line of the file)
string(REGEX REPLACE "\n" ";" bufr_files_to_download "${bufr_files_to_download}")


file(READ "bufr_ref_files.txt" bufr_refs_to_download)
string(REGEX REPLACE "\n" ";" bufr_refs_to_download "${bufr_refs_to_download}")

# Exceptional case: download bufr files which have to be treated specially
list(APPEND bufr_files_to_download "vos308014_v3_26.bufr")  # See test ecc-197
list(APPEND bufr_files_to_download "bad.bufr")  # See ECC-1938

if( ENABLE_EXTRA_TESTS )
    ecbuild_get_test_multidata(
        TARGET eccodes_download_bufrs
        NOCHECK
        NAMES ${bufr_files_to_download}
    )
    ecbuild_get_test_multidata(
        TARGET eccodes_download_bufr_refs
        NOCHECK
        NAMES ${bufr_refs_to_download}
    )
endif()

# Copy other files - e.g. text files etc from the source data/bufr dir
list(APPEND other_files
    bufr_data_files.txt
    bufr_ref_files.txt
)

foreach( file ${other_files} )
   execute_process( COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${file} ${CMAKE_CURRENT_BINARY_DIR} )
endforeach()
