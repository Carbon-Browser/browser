/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_INSTALLED_SUBSCRIPTION_H_
#define COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_INSTALLED_SUBSCRIPTION_H_

#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "absl/types/optional.h"

#include "base/strings/string_piece_forward.h"
#include "base/time/time.h"
#include "components/adblock/core/common/content_type.h"
#include "components/adblock/core/common/header_filter_data.h"
#include "components/adblock/core/common/sitekey.h"
#include "components/adblock/core/subscription/subscription.h"
#include "url/gurl.h"

namespace adblock {

enum class SpecialFilterType {
  // Allows all ads on the frame and its children, overrides any URL blocking
  // or element hiding:
  Document,
  // Disables element hiding on the frame and its children, URL blocking is
  // still allowed:
  Elemhide,
  // Only consider domain-specific URL filters on this frame and its children:
  Genericblock,
  // Only consider domain-specific element hiding selectors on this frame and
  // its children:
  Generichide,
};
enum class FilterCategory { Allowing, Blocking, DomainSpecificBlocking };

// Represents an installed subscription that can be queried for filters.
class InstalledSubscription : public Subscription {
 public:
  struct Selectors {
    Selectors();
    ~Selectors();
    Selectors(const Selectors&);
    Selectors(Selectors&&);
    Selectors& operator=(const Selectors&);
    Selectors& operator=(Selectors&&);
    // The final set of selectors to apply on a page is |elemhide_selectors|
    // minus |elemhide_exceptions|. This difference is not computed by this
    // Subscription because there may be multiple subscriptions and
    // |elemhide_exceptions| from one subscriptions may remove
    // |elemhide_selectors| from another.
    std::vector<base::StringPiece> elemhide_selectors;
    std::vector<base::StringPiece> elemhide_exceptions;
  };

  class Snippet {
   public:
    Snippet();
    Snippet(const Snippet&);
    Snippet(Snippet&&);
    ~Snippet();
    Snippet& operator=(const Snippet&);
    Snippet& operator=(Snippet&&);
    base::StringPiece command;
    std::vector<base::StringPiece> arguments;
  };

  virtual bool HasUrlFilter(const GURL& url,
                            const std::string& document_domain,
                            ContentType content_type,
                            const SiteKey& sitekey,
                            FilterCategory category) const = 0;
  virtual bool HasPopupFilter(const GURL& url,
                              const GURL& opener_url,
                              const SiteKey& sitekey,
                              FilterCategory category) const = 0;
  virtual bool HasSpecialFilter(SpecialFilterType type,
                                const GURL& url,
                                const std::string& document_domain,
                                const SiteKey& sitekey) const = 0;
  // CSP filters have a payload: a string that gets injected to a network
  // response's Content-Security-Policy header.
  // |nullopt| means no filter found. If a filter is found, the string will
  // be non-empty.
  virtual absl::optional<base::StringPiece> FindCspFilter(
      const GURL& url,
      const std::string& document_domain,
      FilterCategory category) const = 0;
  // Find an URL to rewrite if any for specific request
  virtual absl::optional<base::StringPiece> FindRewriteFilter(
      const GURL& url,
      const std::string& document_domain,
      FilterCategory category) const = 0;

  virtual void FindHeaderFilters(const GURL& url,
                                 ContentType content_type,
                                 const std::string& document_domain,
                                 FilterCategory category,
                                 std::set<HeaderFilterData>& results) const = 0;

  virtual std::vector<Snippet> MatchSnippets(
      const std::string& document_domain) const = 0;

  virtual Selectors GetElemhideSelectors(const GURL& url,
                                         bool domain_specfic) const = 0;
  // Note there's no "domain_specific". Emulation filters are always
  // domain-specific.
  virtual Selectors GetElemhideEmulationSelectors(const GURL& url) const = 0;

  // Instructs to remove the file which contains this subscription's data during
  // destruction. NOP if there is no backing file, when the subscription is
  // created in-memory.
  // Operation is atomic and thread-safe. Consecutive calls are NOPs.
  virtual void MarkForPermanentRemoval() = 0;

 protected:
  friend class base::RefCountedThreadSafe<InstalledSubscription>;
  ~InstalledSubscription() override;
};

}  // namespace adblock

#endif  // COMPONENTS_ADBLOCK_CORE_SUBSCRIPTION_INSTALLED_SUBSCRIPTION_H_
