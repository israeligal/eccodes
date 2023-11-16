/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */


#pragma once

#include "eccodes.h"

#include <string>
#include <vector>

#include "eckit/config/MappedConfiguration.h"
#include "eckit/config/Configuration.h"


namespace eccodes::geo {


bool codes_check_error(int e, const char* call);


class GribConfiguration final : public eckit::Configuration {
public:
    explicit GribConfiguration(codes_handle*);

private:
    // -- Methods

    bool has(const std::string& name) const override;

    bool get(const std::string& name, std::string& value) const override;
    bool get(const std::string& name, bool& value) const override;
    bool get(const std::string& name, int& value) const override;
    bool get(const std::string& name, long& value) const override;
    bool get(const std::string& name, long long& value) const override;
    bool get(const std::string& name, std::size_t& value) const override;
    bool get(const std::string& name, float& value) const override;
    bool get(const std::string& name, double& value) const override;
    bool get(const std::string& name, std::vector<int>& value) const override;
    bool get(const std::string& name, std::vector<long>& value) const override;
    bool get(const std::string& name, std::vector<long long>& value) const override;
    bool get(const std::string& name, std::vector<std::size_t>& value) const override;
    bool get(const std::string& name, std::vector<float>& value) const override;
    bool get(const std::string& name, std::vector<double>& value) const override;
    bool get(const std::string& name, std::vector<std::string>& value) const override;

    void print(std::ostream&) const override;

    // -- Members

    mutable eckit::MappedConfiguration cache_;
    codes_handle* handle_;
};


}  // namespace eccodes::geo
