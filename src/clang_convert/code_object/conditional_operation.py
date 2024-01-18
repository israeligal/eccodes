import utils.debug as debug
import code_object.code_interface as code_interface

# Represent a terniary operation of the form BOOL_EXPR ? TRUE_EXPR : FALSE_EXPR
#
# Models the CONDITIONAL_OPERATOR CursorKind in libclang
#
# expressionX must be a code_interface subclass (or None)
class ConditionalOperation(code_interface.CodeInterface):
    def __init__(self, bool_expression, true_expression, false_expression) -> None:
        self._bool_expression = bool_expression
        self._true_expression = true_expression
        self._false_expression = false_expression
        assert isinstance(self._bool_expression, code_interface.CodeInterface), f"Bool expression must be a CodeInterface class"
        assert isinstance(self._true_expression, code_interface.CodeInterface), f"True expression must be a CodeInterface class"
        assert isinstance(self._false_expression, code_interface.CodeInterface), f"False expression must be a CodeInterface class"

    @property
    def bool_expression(self):
        return self._bool_expression

    @property
    def true_expression(self):
        return self._true_expression

    @property
    def false_expression(self):
        return self._false_expression

    def as_lines(self):
        conditional_lines = []
        conditional_lines.extend(self._bool_expression.as_lines())
        conditional_lines.append("?")
        conditional_lines.extend(self._true_expression.as_lines())
        conditional_lines.append(":")
        conditional_lines.extend(self._false_expression.as_lines())

        return conditional_lines