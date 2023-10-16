#include "GribUtils.h"
#include "ConversionHelper.h"

namespace eccodes::accessor {

bool gribIsEarthOblate()
{
    long oblate{};
    GribStatus ret = unpackLongHelper(AccessorName("earthIsOblate"), oblate);

    return (ret == GribStatus::SUCCESS && oblate == 1);
}

}