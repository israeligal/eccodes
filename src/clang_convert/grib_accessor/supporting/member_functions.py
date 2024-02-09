
from code_object_converter.conversion_pack.funcsig_mapping import FuncSigMapping
from code_object.funcsig import FuncSig
from code_object.arg import Arg
from code_object_converter.conversion_pack.arg_indexes import ArgIndexes
from code_object.code_interface import NONE_VALUE

# NOTE: The C FuncSig variable names may be different so should only be used as a guide (some are missing)
grib_accessor_member_funcsig_mapping = [
    
    # Constructor: static void init(grib_accessor*, const long, grib_arguments*);
    FuncSigMapping( FuncSig("void", "init", [Arg("grib_context*", ""), Arg("const long", ""), Arg("grib_arguments*", "")]),
                    FuncSig("void", "Constructor",   [NONE_VALUE, NONE_VALUE, Arg("AccessorInitData const&", "initData")]),
                    ArgIndexes(cbuffer=2, clength=1, cpp_container=2)),

    # Destructor: static void destroy(grib_context*, grib_accessor*);
    FuncSigMapping( FuncSig("void", "destroy", [Arg("grib_context*", ""), Arg("grib_accessor*", "")]),
                    FuncSig("void", "Destructor", [NONE_VALUE, NONE_VALUE]) ),
]

member_function_names = [mapping.cfuncsig.name for mapping in grib_accessor_member_funcsig_mapping]