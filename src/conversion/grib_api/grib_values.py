# Convert C definitions (e.g. to enums)
import utils.debug as debug
import re

GribStatusTransforms = {
    "GRIB_SUCCESS": "GribStatus::SUCCESS", 
    "GRIB_END_OF_FILE": "GribStatus::END_OF_FILE", 
    "GRIB_INTERNAL_ERROR": "GribStatus::INTERNAL_ERROR", 
    "GRIB_BUFFER_TOO_SMALL": "GribStatus::BUFFER_TOO_SMALL", 
    "GRIB_NOT_IMPLEMENTED": "GribStatus::NOT_IMPLEMENTED", 
    "GRIB_7777_NOT_FOUND": "GribStatus::VALUE_7777_NOT_FOUND", 
    "GRIB_ARRAY_TOO_SMALL": "GribStatus::ARRAY_TOO_SMALL", 
    "GRIB_FILE_NOT_FOUND": "GribStatus::FILE_NOT_FOUND", 
    "GRIB_CODE_NOT_FOUND_IN_TABLE": "GribStatus::CODE_NOT_FOUND_IN_TABLE", 
    "GRIB_WRONG_ARRAY_SIZE": "GribStatus::WRONG_ARRAY_SIZE", 
    "GRIB_NOT_FOUND": "GribStatus::NOT_FOUND", 
    "GRIB_IO_PROBLEM": "GribStatus::IO_PROBLEM", 
    "GRIB_INVALID_MESSAGE": "GribStatus::INVALID_MESSAGE", 
    "GRIB_DECODING_ERROR": "GribStatus::DECODING_ERROR", 
    "GRIB_ENCODING_ERROR": "GribStatus::ENCODING_ERROR", 
    "GRIB_NO_MORE_IN_SET": "GribStatus::NO_MORE_IN_SET", 
    "GRIB_GEOCALCULUS_PROBLEM": "GribStatus::GEOCALCULUS_PROBLEM", 
    "GRIB_OUT_OF_MEMORY": "GribStatus::OUT_OF_MEMORY", 
    "GRIB_READ_ONLY": "GribStatus::READ_ONLY", 
    "GRIB_INVALID_ARGUMENT": "GribStatus::INVALID_ARGUMENT", 
    "GRIB_NULL_HANDLE": "GribStatus::NULL_HANDLE", 
    "GRIB_INVALID_SECTION_NUMBER": "GribStatus::INVALID_SECTION_NUMBER", 
    "GRIB_VALUE_CANNOT_BE_MISSING": "GribStatus::VALUE_CANNOT_BE_MISSING", 
    "GRIB_WRONG_LENGTH": "GribStatus::WRONG_LENGTH", 
    "GRIB_INVALID_TYPE": "GribStatus::INVALID_TYPE", 
    "GRIB_WRONG_STEP": "GribStatus::WRONG_STEP", 
    "GRIB_WRONG_STEP_UNIT": "GribStatus::WRONG_STEP_UNIT", 
    "GRIB_INVALID_FILE": "GribStatus::INVALID_FILE", 
    "GRIB_INVALID_GRIB": "GribStatus::INVALID_GRIB", 
    "GRIB_INVALID_INDEX": "GribStatus::INVALID_INDEX", 
    "GRIB_INVALID_ITERATOR": "GribStatus::INVALID_ITERATOR", 
    "GRIB_INVALID_KEYS_ITERATOR": "GribStatus::INVALID_KEYS_ITERATOR", 
    "GRIB_INVALID_NEAREST": "GribStatus::INVALID_NEAREST", 
    "GRIB_INVALID_ORDERBY": "GribStatus::INVALID_ORDERBY", 
    "GRIB_MISSING_KEY": "GribStatus::MISSING_KEY", 
    "GRIB_OUT_OF_AREA": "GribStatus::OUT_OF_AREA", 
    "GRIB_CONCEPT_NO_MATCH": "GribStatus::CONCEPT_NO_MATCH", 
    "GRIB_HASH_ARRAY_NO_MATCH": "GribStatus::HASH_ARRAY_NO_MATCH", 
    "GRIB_NO_DEFINITIONS": "GribStatus::NO_DEFINITIONS", 
    "GRIB_WRONG_TYPE": "GribStatus::WRONG_TYPE", 
    "GRIB_END": "GribStatus::END", 
    "GRIB_NO_VALUES": "GribStatus::NO_VALUES", 
    "GRIB_WRONG_GRID": "GribStatus::WRONG_GRID", 
    "GRIB_END_OF_INDEX": "GribStatus::END_OF_INDEX", 
    "GRIB_NULL_INDEX": "GribStatus::NULL_INDEX", 
    "GRIB_PREMATURE_END_OF_FILE": "GribStatus::PREMATURE_END_OF_FILE", 
    "GRIB_INTERNAL_ARRAY_TOO_SMALL": "GribStatus::INTERNAL_ARRAY_TOO_SMALL", 
    "GRIB_MESSAGE_TOO_LARGE": "GribStatus::MESSAGE_TOO_LARGE", 
    "GRIB_CONSTANT_FIELD": "GribStatus::CONSTANT_FIELD", 
    "GRIB_SWITCH_NO_MATCH": "GribStatus::SWITCH_NO_MATCH", 
    "GRIB_UNDERFLOW": "GribStatus::UNDERFLOW", 
    "GRIB_MESSAGE_MALFORMED": "GribStatus::MESSAGE_MALFORMED", 
    "GRIB_CORRUPTED_INDEX": "GribStatus::CORRUPTED_INDEX", 
    "GRIB_INVALID_BPV": "GribStatus::INVALID_BPV", 
    "GRIB_DIFFERENT_EDITION": "GribStatus::DIFFERENT_EDITION", 
    "GRIB_VALUE_DIFFERENT": "GribStatus::VALUE_DIFFERENT", 
    "GRIB_INVALID_KEY_VALUE": "GribStatus::INVALID_KEY_VALUE", 
    "GRIB_STRING_TOO_SMALL": "GribStatus::STRING_TOO_SMALL", 
    "GRIB_WRONG_CONVERSION": "GribStatus::WRONG_CONVERSION", 
    "GRIB_MISSING_BUFR_ENTRY": "GribStatus::MISSING_BUFR_ENTRY", 
    "GRIB_NULL_POINTER": "GribStatus::NULL_POINTER", 
    "GRIB_ATTRIBUTE_CLASH": "GribStatus::ATTRIBUTE_CLASH", 
    "GRIB_TOO_MANY_ATTRIBUTES": "GribStatus::TOO_MANY_ATTRIBUTES", 
    "GRIB_ATTRIBUTE_NOT_FOUND": "GribStatus::ATTRIBUTE_NOT_FOUND", 
    "GRIB_UNSUPPORTED_EDITION": "GribStatus::UNSUPPORTED_EDITION", 
    "GRIB_OUT_OF_RANGE": "GribStatus::OUT_OF_RANGE", 
    "GRIB_WRONG_BITMAP_SIZE": "GribStatus::WRONG_BITMAP_SIZE", 
    "GRIB_FUNCTIONALITY_NOT_ENABLED": "GribStatus::FUNCTIONALITY_NOT_ENABLED", 
    "GRIB_VALUE_MISMATCH": "GribStatus::VALUE_MISMATCH", 
    "GRIB_DOUBLE_VALUE_MISMATCH": "GribStatus::DOUBLE_VALUE_MISMATCH", 
    "GRIB_LONG_VALUE_MISMATCH": "GribStatus::LONG_VALUE_MISMATCH", 
    "GRIB_BYTE_VALUE_MISMATCH": "GribStatus::BYTE_VALUE_MISMATCH", 
    "GRIB_STRING_VALUE_MISMATCH": "GribStatus::STRING_VALUE_MISMATCH", 
    "GRIB_OFFSET_MISMATCH": "GribStatus::OFFSET_MISMATCH", 
    "GRIB_COUNT_MISMATCH": "GribStatus::COUNT_MISMATCH", 
    "GRIB_NAME_MISMATCH": "GribStatus::NAME_MISMATCH", 
    "GRIB_TYPE_MISMATCH": "GribStatus::TYPE_MISMATCH", 
    "GRIB_TYPE_AND_VALUE_MISMATCH": "GribStatus::TYPE_AND_VALUE_MISMATCH", 
    "GRIB_UNABLE_TO_COMPARE_ACCESSORS": "GribStatus::UNABLE_TO_COMPARE_ACCESSORS", 
    "GRIB_ASSERTION_FAILURE": "GribStatus::ASSERTION_FAILURE", 
}

GribTypeTransforms = {
    "GRIB_TYPE_UNDEFINED": "GribType::UNDEFINED",
    "GRIB_TYPE_LONG": "GribType::LONG",
    "GRIB_TYPE_DOUBLE": "GribType::DOUBLE",
    "GRIB_TYPE_STRING": "GribType::STRING",
    "GRIB_TYPE_BYTES": "GribType::BYTES",
    "GRIB_TYPE_SECTION": "GribType::SECTION",
    "GRIB_TYPE_LABEL": "GribType::LABEL",
    "GRIB_TYPE_MISSING": "GribType::MISSING",
}

GribAccessorFlagTransforms = {
    "GRIB_ACCESSOR_FLAG_READ_ONLY" : "GribAccessorFlag::READ_ONLY",
    "GRIB_ACCESSOR_FLAG_DUMP" : "GribAccessorFlag::DUMP",
    "GRIB_ACCESSOR_FLAG_EDITION_SPECIFIC" : "GribAccessorFlag::EDITION_SPECIFIC",
    "GRIB_ACCESSOR_FLAG_CAN_BE_MISSING" : "GribAccessorFlag::CAN_BE_MISSING",
    "GRIB_ACCESSOR_FLAG_HIDDEN" : "GribAccessorFlag::HIDDEN",
    "GRIB_ACCESSOR_FLAG_CONSTRAINT" : "GribAccessorFlag::CONSTRAINT",
    "GRIB_ACCESSOR_FLAG_BUFR_DATA" : "GribAccessorFlag::BUFR_DATA",
    "GRIB_ACCESSOR_FLAG_NO_COPY" : "GribAccessorFlag::NO_COPY",
    "GRIB_ACCESSOR_FLAG_COPY_OK" : "GribAccessorFlag::COPY_OK",
    "GRIB_ACCESSOR_FLAG_FUNCTION" : "GribAccessorFlag::FUNCTION",
    "GRIB_ACCESSOR_FLAG_DATA" : "GribAccessorFlag::DATA",
    "GRIB_ACCESSOR_FLAG_NO_FAIL" : "GribAccessorFlag::NO_FAIL",
    "GRIB_ACCESSOR_FLAG_TRANSIENT" : "GribAccessorFlag::TRANSIENT",
    "GRIB_ACCESSOR_FLAG_STRING_TYPE" : "GribAccessorFlag::STRING_TYPE",
    "GRIB_ACCESSOR_FLAG_LONG_TYPE" : "GribAccessorFlag::LONG_TYPE",
    "GRIB_ACCESSOR_FLAG_DOUBLE_TYPE" : "GribAccessorFlag::DOUBLE_TYPE",
    "GRIB_ACCESSOR_FLAG_LOWERCASE" : "GribAccessorFlag::LOWERCASE",
    "GRIB_ACCESSOR_FLAG_BUFR_COORD" : "GribAccessorFlag::BUFR_COORD",
    "GRIB_ACCESSOR_FLAG_COPY_IF_CHANGING_EDITION" : "GribAccessorFlag::COPY_IF_CHANGING_EDITION",
}

# Grib Transformers - return C++ string representing the transformed value, or None
#
# Note - may include extra processing, e.g. GribAccessorFlags are wrapped in a toInt() call...

def transform_grib_status(cgrib_status):
    for cstatus, cppstatus in GribStatusTransforms.items():
        if cstatus == cgrib_status:
            return cppstatus
        
    return None

def transform_grib_type(cgrib_type):
    for ctype, cpptype in GribTypeTransforms.items():
        if ctype == cgrib_type:
            return cpptype
        
    return None

def transform_grib_accessor_flag(cgrib_accessor_flag):
    for caccessor_flag, cppaccessor_flag in GribAccessorFlagTransforms.items():
        if caccessor_flag == cgrib_accessor_flag:
            return f"toInt({cppaccessor_flag})"
        
    return None

grib_transformers = [
    transform_grib_status,
    transform_grib_type,
    transform_grib_accessor_flag
]

# Find and replace any known GRIB_* values
# Calls itself recursively after each match, with the remainder, until all matches made
def convert_grib_values(line, depth=0):
    assert depth<5, f"Unexpected recursion depth [{depth}]"

    m = re.search(r"\b(GRIB_\w+)\b", line)
    if m:
        # Call recursively to process the remainder
        remainder = convert_grib_values(line[m.end():], depth+1)

        for grib_transformer in grib_transformers:
            transformed_value = grib_transformer(m.group(1))
            if transformed_value:
                line = line[:m.start()] + transformed_value + remainder
                debug.line("convert_grib_values", f"[{depth}] Replaced {m.group(1)} with {transformed_value}: {line}")
                return line
                
        # GRIB_ entry doesn't match
        return line[:m.end()] + remainder
    
    # No GRIB_ entries found
    return line
