/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "components/adblock/mock_filter_impl.h"

#include "base/notreached.h"

namespace adblock {

MockFilterImpl::MockFilterImpl(const std::string& raw,
                               AdblockPlus::IFilterImplementation::Type type)
    : raw_(raw), type_(type) {}

MockFilterImpl::~MockFilterImpl() = default;

AdblockPlus::IFilterImplementation::Type MockFilterImpl::GetType() const {
  return type_;
}

std::string MockFilterImpl::GetRaw() const {
  return raw_;
}

bool MockFilterImpl::operator==(
    const AdblockPlus::IFilterImplementation& other) const {
  return GetRaw() == other.GetRaw();
}

std::unique_ptr<AdblockPlus::IFilterImplementation> MockFilterImpl::Clone()
    const {
  return std::make_unique<MockFilterImpl>(raw_, type_);
}

}  // namespace adblock
