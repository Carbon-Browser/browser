// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DIPS_DIPS_UTILS_H_
#define CONTENT_BROWSER_DIPS_DIPS_UTILS_H_

#include <optional>
#include <ostream>
#include <string_view>

#include "base/files/file_path.h"
#include "base/strings/cstring_view.h"
#include "base/time/time.h"
#include "content/common/content_export.h"
#include "content/public/browser/cookie_access_details.h"
#include "content/public/browser/dips_redirect_info.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page.h"
#include "content/public/browser/render_frame_host.h"
#include "services/network/public/mojom/cookie_access_observer.mojom.h"
#include "url/gurl.h"

namespace base {
class TimeDelta;
}

namespace content {
class BrowserContext;
}

namespace url {
class Origin;
}

// For use in tests/debugging.
CONTENT_EXPORT base::cstring_view DIPSCookieModeToString(DIPSCookieMode mode);
CONTENT_EXPORT base::cstring_view DIPSRedirectTypeToString(
    DIPSRedirectType type);
CONTENT_EXPORT base::cstring_view DIPSDataAccessTypeToString(
    DIPSDataAccessType type);

// A single cookie-accessing operation (either read or write). Not to be
// confused with DIPSDataAccessType, which can also represent no access or both
// read+write.
using CookieOperation = network::mojom::CookieAccessDetails::Type;

// The filename for the DIPS database.
const base::FilePath::CharType kDIPSFilename[] = FILE_PATH_LITERAL("DIPS");

// The FilePath for the ON-DISK DIPSDatabase associated with a BrowserContext,
// if one exists.
// NOTE: This returns the same value regardless of if there is actually a
// persisted DIPSDatabase for the BrowserContext or not.
CONTENT_EXPORT base::FilePath GetDIPSFilePath(content::BrowserContext* context);

inline DIPSDataAccessType ToDIPSDataAccessType(CookieOperation op) {
  return (op == CookieOperation::kChange ? DIPSDataAccessType::kWrite
                                         : DIPSDataAccessType::kRead);
}
CONTENT_EXPORT std::ostream& operator<<(std::ostream& os,
                                        DIPSDataAccessType access_type);

constexpr DIPSDataAccessType operator|(DIPSDataAccessType lhs,
                                       DIPSDataAccessType rhs) {
  return static_cast<DIPSDataAccessType>(static_cast<int>(lhs) |
                                         static_cast<int>(rhs));
}
inline DIPSDataAccessType& operator|=(DIPSDataAccessType& lhs,
                                      DIPSDataAccessType rhs) {
  return (lhs = lhs | rhs);
}

DIPSCookieMode GetDIPSCookieMode(bool is_otr);
std::string_view GetHistogramSuffix(DIPSCookieMode mode);
std::ostream& operator<<(std::ostream& os, DIPSCookieMode mode);

// DIPSEventRemovalType:
// NOTE: We use this type as a bitfield don't change existing values other than
// kAll, which should be updated to include any new fields.
enum class DIPSEventRemovalType {
  kNone = 0,
  kHistory = 1 << 0,
  kStorage = 1 << 1,
  // kAll is intended to cover all the above fields.
  kAll = kHistory | kStorage
};

constexpr DIPSEventRemovalType operator|(DIPSEventRemovalType lhs,
                                         DIPSEventRemovalType rhs) {
  return static_cast<DIPSEventRemovalType>(static_cast<int>(lhs) |
                                           static_cast<int>(rhs));
}

constexpr DIPSEventRemovalType operator&(DIPSEventRemovalType lhs,
                                         DIPSEventRemovalType rhs) {
  return static_cast<DIPSEventRemovalType>(static_cast<int>(lhs) &
                                           static_cast<int>(rhs));
}

constexpr DIPSEventRemovalType& operator|=(DIPSEventRemovalType& lhs,
                                           DIPSEventRemovalType rhs) {
  return lhs = lhs | rhs;
}

constexpr DIPSEventRemovalType& operator&=(DIPSEventRemovalType& lhs,
                                           DIPSEventRemovalType rhs) {
  return lhs = lhs & rhs;
}

std::string_view GetHistogramPiece(DIPSRedirectType type);
CONTENT_EXPORT std::ostream& operator<<(std::ostream& os,
                                        DIPSRedirectType type);

using TimestampRange = std::optional<std::pair<base::Time, base::Time>>;
// Expand the range to include `time` if necessary. Returns true iff the range
// was modified.
CONTENT_EXPORT bool UpdateTimestampRange(TimestampRange& range,
                                         base::Time time);
// Checks that `this` range is either null or falls within `other`.
CONTENT_EXPORT bool IsNullOrWithin(const TimestampRange& inner,
                                   const TimestampRange& outer);

std::ostream& operator<<(std::ostream& os, TimestampRange type);

// Values for a site in the `bounces` table.
struct StateValue {
  TimestampRange site_storage_times;
  TimestampRange user_interaction_times;
  TimestampRange stateful_bounce_times;
  TimestampRange bounce_times;
  TimestampRange web_authn_assertion_times;
};

// Values for a 1P/3P site pair in the `popups` table.
struct PopupsStateValue {
  uint64_t access_id;
  base::Time last_popup_time;
  bool is_current_interaction;
  bool is_authentication_interaction;
};

struct PopupWithTime {
  std::string opener_site;
  std::string popup_site;
  base::Time last_popup_time;
};

// These values are emitted in metrics and should not be renumbered. This one
// type is used for both of the IsAdTagged and HasSameSiteIframe UKM enums.
enum class OptionalBool {
  kUnknown = 0,
  kFalse = 1,
  kTrue = 2,
};

inline OptionalBool ToOptionalBool(bool b) {
  return b ? OptionalBool::kTrue : OptionalBool::kFalse;
}

inline bool operator==(const StateValue& lhs, const StateValue& rhs) {
  return std::tie(lhs.site_storage_times, lhs.user_interaction_times,
                  lhs.stateful_bounce_times, lhs.bounce_times,
                  lhs.web_authn_assertion_times) ==
         std::tie(rhs.site_storage_times, rhs.user_interaction_times,
                  rhs.stateful_bounce_times, rhs.bounce_times,
                  rhs.web_authn_assertion_times);
}

// Return the number of seconds in `delta`, clamped to [0, 10].
// i.e. 11 linearly-sized buckets.
CONTENT_EXPORT int64_t BucketizeDIPSBounceDelay(base::TimeDelta delta);

// Returns an opaque value representing the "privacy boundary" that the URL
// belongs to. Currently returns eTLD+1, but this is an implementation detail
// and may change.
CONTENT_EXPORT std::string GetSiteForDIPS(const GURL& url);
CONTENT_EXPORT std::string GetSiteForDIPS(const url::Origin& origin);

// Returns true iff `web_contents` contains an iframe whose committed URL
// belongs to the same site as `url`.
bool HasSameSiteIframe(content::WebContents* web_contents, const GURL& url);

// Returns whether the provided cookie access was ad-tagged, based on the cookie
// settings overrides. Returns Unknown if kSkipTpcdMitigationsForAdsHeuristics
// is false and the override is not set regardless.
CONTENT_EXPORT OptionalBool
IsAdTaggedCookieForHeuristics(const content::CookieAccessDetails& details);

CONTENT_EXPORT bool HasCHIPS(
    const net::CookieAccessResultList& cookie_access_result_list);

// Returns `True` iff the `navigation_handle` represents a navigation
// happening in an iframe of the primary frame tree.
inline bool IsInPrimaryPageIFrame(
    content::NavigationHandle* navigation_handle) {
  return navigation_handle && navigation_handle->GetParentFrame()
             ? navigation_handle->GetParentFrame()->GetPage().IsPrimary()
             : false;
}

// Returns `True` iff both urls return a similar outcome off of
// `GetSiteForDIPS()`.
inline bool IsSameSiteForDIPS(const GURL& url1, const GURL& url2) {
  return GetSiteForDIPS(url1) == GetSiteForDIPS(url2);
}

// Returns `True` iff the `navigation_handle` represents a navigation happening
// in any frame of the primary page.
// NOTE: This does not include fenced frames.
inline bool IsInPrimaryPage(content::NavigationHandle* navigation_handle) {
  return navigation_handle && navigation_handle->GetParentFrame()
             ? navigation_handle->GetParentFrame()->GetPage().IsPrimary()
             : navigation_handle->IsInPrimaryMainFrame();
}

// Returns `True` iff the 'rfh' exists and represents a frame in the primary
// page.
inline bool IsInPrimaryPage(content::RenderFrameHost* rfh) {
  return rfh && rfh->GetPage().IsPrimary();
}

// Returns the last committed or the to be committed url of the main frame of
// the page containing the `navigation_handle`.
inline std::optional<GURL> GetFirstPartyURL(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle) {
    return std::nullopt;
  }
  return navigation_handle->GetParentFrame()
             ? navigation_handle->GetParentFrame()
                   ->GetMainFrame()
                   ->GetLastCommittedURL()
             : navigation_handle->GetURL();
}

// Returns an optional last committed url of the main frame of the page
// containing the `rfh`.
inline std::optional<GURL> GetFirstPartyURL(content::RenderFrameHost* rfh) {
  return rfh ? std::optional<GURL>(rfh->GetMainFrame()->GetLastCommittedURL())
             : std::nullopt;
}

// The amount of time since a page last received user interaction before a
// subsequent user interaction event may be recorded to DIPS Storage for the
// same page.
inline constexpr base::TimeDelta kDIPSTimestampUpdateInterval =
    base::Minutes(1);

[[nodiscard]] CONTENT_EXPORT bool UpdateTimestamp(
    std::optional<base::Time>& last_time,
    base::Time now);

// DIPSInteractionType is used in UKM to record the way the user interacted with
// the site. It should match CookieHeuristicInteractionType in
// tools/metrics/ukm/ukm.xml
enum class DIPSInteractionType {
  Authentication = 0,
  UserActivation = 1,
  NoInteraction = 2,
};

enum class DIPSRecordedEvent {
  kStorage,
  kInteraction,
  kWebAuthnAssertion,
};

// DIPSRedirectCategory is basically the cross-product of DIPSDataAccessType and
// a boolean value indicating site engagement. It's used in UMA enum histograms.
//
// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class DIPSRedirectCategory {
  kNoCookies_NoEngagement = 0,
  kReadCookies_NoEngagement = 1,
  kWriteCookies_NoEngagement = 2,
  kReadWriteCookies_NoEngagement = 3,
  kNoCookies_HasEngagement = 4,
  kReadCookies_HasEngagement = 5,
  kWriteCookies_HasEngagement = 6,
  kReadWriteCookies_HasEngagement = 7,
  kUnknownCookies_NoEngagement = 8,
  kUnknownCookies_HasEngagement = 9,
  kMaxValue = kUnknownCookies_HasEngagement,
};

// DIPSErrorCode is used in UMA enum histograms to monitor certain errors and
// verify that they are being fixed.
//
// When adding an error to this enum, update the DIPSErrorCode enum in
// tools/metrics/histograms/enums.xml as well.
//
// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class DIPSErrorCode {
  kRead_None = 0,
  kRead_OpenEndedRange_NullStart = 1,
  kRead_OpenEndedRange_NullEnd = 2,
  kRead_BounceTimesIsntSupersetOfStatefulBounces = 3,
  kRead_EmptySite_InDb = 4,
  kRead_EmptySite_NotInDb = 5,
  kWrite_None = 6,
  kWrite_EmptySite = 7,
  kMaxValue = kWrite_EmptySite,
};

// DIPSDeletionAction is used in UMA enum histograms to record the actual
// deletion action taken on DIPS-eligible (incidental) site.
//
// When adding an action to this enum, update the DIPSDeletionAction enum in
// tools/metrics/histograms/enums.xml as well.
//
// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class DIPSDeletionAction {
  kDisallowed = 0,
  kExceptedAs1p = 1,  // No longer used - merged into 'kExcepted' below.
  kExceptedAs3p = 2,  // No longer used - merged into 'kExcepted' below.
  kEnforced = 3,
  kIgnored = 4,
  kExcepted = 5,
  kMaxValue = kExcepted,
};

enum class DIPSDatabaseTable {
  kBounces = 1,
  kPopups = 2,
  kMaxValue = kPopups,
};

#endif  // CONTENT_BROWSER_DIPS_DIPS_UTILS_H_
