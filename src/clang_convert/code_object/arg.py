
import code_object.code_interface as code_interface
import utils.debug as debug
from code_object.declaration_specifier import DeclSpec
import re
from code_object.code_interface import NONE_VALUE

# Create an arg using a decl_specifier_seq string (see DeclSpec class) and (optional) name
# name can be "" to represent a type only
class Arg(code_interface.CodeInterface):
    
    # Note: decl_spec can be a string or an instance of class DeclSpec
    def __init__(self, decl_spec, name="", is_func_arg=False) -> None:
        if isinstance(decl_spec, DeclSpec):
            self._decl_spec = DeclSpec.from_instance(decl_spec)
        elif isinstance(decl_spec, str):
            self._decl_spec = DeclSpec.from_decl_specifier_seq(decl_spec)
        else:
            assert False, f"Unexpected decl_spec=[{decl_spec}]"

        self._name = name
        self._is_func_arg = is_func_arg # Support for processing function sig args differently to other args

    # Create from an existing Arg
    @classmethod
    def from_instance(cls, instance):
        return cls(instance.decl_spec, instance.name, instance.is_func_arg)

    # Use decl_spec to get storage_class etc...
    @property
    def decl_spec(self):
        return self._decl_spec

    @property
    def name(self):
        return self._name

    @property
    def is_func_arg(self):
        return self._is_func_arg

    # ---------- Support for Arg as a dict key: Begin ----------
    def __hash__(self):
        return hash((self.decl_spec, self.name))

    def __eq__(self, other):
        if self is NONE_VALUE or other is NONE_VALUE:
            return self is other

        # Ensure the other object is an instance of Arg for comparison.
        if not isinstance(other, Arg):
            return False

        return self.decl_spec == other.decl_spec and self.name == other.name
    # ---------- Support for Arg as a dict key: End   ----------

    def as_lines(self):
        arg_string = self.decl_spec.as_string()
        if self.decl_spec.is_array_type():
            arg_string = arg_string.replace(self.decl_spec.pointer, " ") + self.name + self.decl_spec.pointer
        else:
            arg_string += " " + self.name
        return [arg_string]
