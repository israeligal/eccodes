from accessor_specific.default import AccessorSpecific
import arg

class Uint8DataAccessorSpecific(AccessorSpecific):
    def __init__(self) -> None:
        super().__init__()
   
        self._custom_arg_transforms["ALL"] = {
            arg.Arg("unsigned char*","data") : arg.Arg("AccessorDataPointer","data"),
            }
