
import code_object.code_interface as code_interface
from utils.string_funcs import strip_semicolon

# Represents accessing an element in an array in the form: name[index]
#
# name and index must be a code_interface subclass
class ArrayAccess(code_interface.CodeInterface):
    def __init__(self, name, index) -> None:
        self._name = name
        self._index = index
        assert isinstance(self._name, code_interface.CodeInterface), f"Name must be a CodeInterface class, not [{type(self._name)}]"
        assert isinstance(self._index, code_interface.CodeInterface), f"Index must be a CodeInterface class, not [{type(self._index)}]"

    # TODO - Should this be an Arg in order to access the decl_spec interface?
    @property
    def name(self):
        return self._name

    @property
    def index(self):
        return self._index

    def as_lines(self):
        return [f"{strip_semicolon(self._name.as_string())}[{strip_semicolon(self._index.as_string())}]"]