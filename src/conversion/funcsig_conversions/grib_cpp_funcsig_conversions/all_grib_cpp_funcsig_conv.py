
import funcsig_conversions.grib_cpp_funcsig_conversions.grib_bits_funcsig_conv as grib_bits_funcsig_conv
import funcsig_conversions.grib_cpp_funcsig_conversions.grib_common_funcsig_conv as grib_common_funcsig_conv
import funcsig_conversions.grib_cpp_funcsig_conversions.grib_query_funcsig_conv as grib_query_funcsig_conv
import funcsig_conversions.grib_cpp_funcsig_conversions.grib_value_funcsig_conv as grib_value_funcsig_conv
import funcsig_conversions.grib_cpp_funcsig_conversions.string_util_funcsig_conv as string_util_funcsig_conv
import funcsig_conversions.grib_cpp_funcsig_conversions.grib_scaling_funcsig_conv as grib_scaling_funcsig_conv

grib_cpp_funcsig_conversions = [
    grib_bits_funcsig_conv.grib_bits_funcsig_conversions,
    grib_common_funcsig_conv.grib_common_funcsig_conversions,
    grib_query_funcsig_conv.grib_query_funcsig_conversions,
    grib_value_funcsig_conv.grib_value_funcsig_conversions,
    string_util_funcsig_conv.string_util_funcsig_conversions,
    grib_scaling_funcsig_conv.grib_scaling_funcsig_conversions,
]

# Return a list of all the funcsig conversions
def all_grib_cpp_funcsig_conversions():
    return grib_cpp_funcsig_conversions