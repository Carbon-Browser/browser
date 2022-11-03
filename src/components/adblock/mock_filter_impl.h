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

#ifndef COMPONENTS_ADBLOCK_MOCK_FILTER_IMPL_H_
#define COMPONENTS_ADBLOCK_MOCK_FILTER_IMPL_H_

#include <memory>
#include <string>

#include "third_party/libadblockplus/src/include/AdblockPlus/IFilterImplementation.h"

namespace adblock {

class MockFilterImpl : public AdblockPlus::IFilterImplementation {
 public:
  MockFilterImpl(const std::string& raw, Type type);
  ~MockFilterImpl() override;

  AdblockPlus::IFilterImplementation::Type GetType() const override;
  std::string GetRaw() const override;
  std::unique_ptr<AdblockPlus::IFilterImplementation> Clone() const override;
  bool operator==(const AdblockPlus::IFilterImplementation&) const override;

 private:
  std::string raw_;
  Type type_;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_MOCK_FILTER_IMPL_H_
