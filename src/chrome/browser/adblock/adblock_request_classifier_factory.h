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

#ifndef CHROME_BROWSER_ADBLOCK_ADBLOCK_REQUEST_CLASSIFIER_FACTORY_H_
#define CHROME_BROWSER_ADBLOCK_ADBLOCK_REQUEST_CLASSIFIER_FACTORY_H_

#include "base/no_destructor.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "content/public/browser/browser_context.h"

namespace adblock {

class AdblockRequestClassifier;
class AdblockRequestClassifierFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static AdblockRequestClassifier* GetForBrowserContext(
      content::BrowserContext* context);
  static AdblockRequestClassifierFactory* GetInstance();

 private:
  friend class base::NoDestructor<AdblockRequestClassifierFactory>;
  AdblockRequestClassifierFactory();
  ~AdblockRequestClassifierFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // namespace adblock

#endif  // CHROME_BROWSER_ADBLOCK_ADBLOCK_REQUEST_CLASSIFIER_FACTORY_H_
