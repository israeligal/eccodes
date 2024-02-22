
import utils.debug as debug
import grib_accessor.grib_accessor_conversion_pack.conversion_pack_updates.base_conversion_pack_updates as base_conversion_pack_updates
from code_object.arg import Arg
from code_object_converter.conversion_pack.funcsig_mapping import FuncSigMapping
from code_object_converter.conversion_pack.arg_indexes import ArgIndexes
from code_object.funcsig import FuncSig
from code_object.code_interface import NONE_VALUE

class StepHumanReadableConversionPackUpdates(base_conversion_pack_updates.BaseConversionPackUpdates):
    def __init__(self) -> None:
        super().__init__()

        self._funcsig_mappings.extend([
            # template<typename T> static int unpack_helper(grib_accessor* a, T* val, size_t* len)
            FuncSigMapping(FuncSig("int", "get_step_human_readable", [Arg("grib_handle*", "h"), Arg("char*", "result"), Arg("size_t*", "length")]),
                        FuncSig("GribStatus", "getStepHumanReadable", [NONE_VALUE, Arg("std::string&", "result"),  NONE_VALUE]),
                        ArgIndexes(cbuffer=1, clength=2, cpp_container=1)),
        ])