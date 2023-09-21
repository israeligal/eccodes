#include "AccessorStore.h"
#include "Accessor.h"
#include <algorithm>

namespace {
   static std::size_t bufferOffset{0};
}

namespace eccodes::accessor {  

AccessorStore& AccessorStore::instance() {
    static AccessorStore theOne;
    return theOne;
}

bool AccessorStore::addAccessor(AccessorPtr accessor)
{
   std::lock_guard<std::recursive_mutex> guard(mutex_);

   // TODO: Handle duplicates ("same")

   store_.push_back({accessor->name(), accessor});

   return true;
}

AccessorPtr AccessorStore::getAccessor(AccessorName const& name)
{
   std::lock_guard<std::recursive_mutex> guard(mutex_);

   if(auto it = std::find_if(std::begin(store_), std::end(store_),
                             [&name](AccessorEntry const& entry) { return name.get() == entry.first.get(); });
      it != store_.end())
   {
      return it->second;
   }

   return {};
}

bool AccessorStore::destroyAccessor(AccessorName const& name)
{
   std::lock_guard<std::recursive_mutex> guard(mutex_);

   if(auto it = std::find_if(std::begin(store_), std::end(store_),
                            [&name](AccessorEntry const& entry) { return name.get() == entry.first.get(); });
      it != store_.end())
   {
      store_.erase(it);
      return true;
   }

   return false;
}

AccessorPtr get(AccessorName const& name)
{
   return AccessorStore::instance().getAccessor(name);
}

// C-Support
void add_grib_accessor(AccessorName const& name, grib_accessor* a)
{
   AccessorStore::instance().grib_accessors_[name] = a;
}

grib_accessor* get_grib_accessor(AccessorName const& name)
{
   // Will throw if accessor not found...
   return AccessorStore::instance().grib_accessors_.at(name);
}

}