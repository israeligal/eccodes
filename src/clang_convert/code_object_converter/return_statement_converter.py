
import utils.debug as debug
import code_object.return_statement as return_statement
import code_object_converter.code_interface_converter as code_interface_converter
import code_object_converter.conversion_funcs as conversion_funcs

class ReturnStatementConverter(code_interface_converter.CodeInterfaceConverter):
    def __init__(self, ccode_object) -> None:
        super().__init__(ccode_object)
        assert isinstance(ccode_object, return_statement.ReturnStatement), f"Expected ReturnStatement, got type=[{type(ccode_object)}]"

    def create_cpp_code_object(self, conversion_pack):
        cpp_expression = conversion_funcs.convert_ccode_object(self._ccode_object.expression, conversion_pack)

        cppreturn_statement = return_statement.ReturnStatement(cpp_expression)
        return conversion_pack.conversion_validation.validate_return_statement(self._ccode_object, cppreturn_statement)