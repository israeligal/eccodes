from accessor_specific.default import AccessorSpecific
import arg

class BitDataAccessorSpecific(AccessorSpecific):
    def __init__(self) -> None:
        super().__init__()
   
        self._custom_arg_transforms["ALL"] = {
            arg.Arg("unsigned char*","mdata") : arg.Arg("AccessorDataPointer","mdata"),
            }