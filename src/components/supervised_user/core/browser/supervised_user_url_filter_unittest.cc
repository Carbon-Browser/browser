// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/supervised_user/core/browser/supervised_user_url_filter.h"

#include <cstddef>
#include <map>
#include <memory>
#include <optional>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/run_loop.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "components/prefs/testing_pref_service.h"
#include "components/safe_search_api/fake_url_checker_client.h"
#include "components/supervised_user/core/browser/supervised_user_preferences.h"
#include "components/supervised_user/core/browser/supervised_user_utils.h"
#include "components/supervised_user/core/common/features.h"
#include "components/supervised_user/core/common/pref_names.h"
#include "components/supervised_user/test_support/supervised_user_url_filter_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace supervised_user {
namespace {

using safe_search_api::ClassificationDetails;

class SupervisedUserURLFilterTest : public ::testing::Test,
                                    public SupervisedUserURLFilter::Observer {
 public:
  SupervisedUserURLFilterTest() {
    PrefRegistrySimple* registry = pref_service_.registry();
    RegisterProfilePrefs(registry);
    filter_.SetURLCheckerClient(
        std::make_unique<safe_search_api::FakeURLCheckerClient>());
    filter_.SetDefaultFilteringBehavior(FilteringBehavior::kBlock);
    filter_.AddObserver(this);
  }

  ~SupervisedUserURLFilterTest() override { filter_.RemoveObserver(this); }

  // SupervisedUserURLFilter::Observer:
  void OnURLChecked(SupervisedUserURLFilter::Result result) override {
    behavior_ = result.behavior;
    reason_ = result.reason;
  }

 protected:
  bool IsURLAllowlisted(const std::string& url) {
    return filter_.GetFilteringBehavior(GURL(url)).IsAllowed();
  }

  void ExpectURLInDefaultAllowlist(const std::string& url) {
    ExpectURLCheckMatches(url, FilteringBehavior::kAllow,
                          FilteringBehaviorReason::DEFAULT);
  }

  void ExpectURLInDefaultDenylist(const std::string& url) {
    ExpectURLCheckMatches(url, FilteringBehavior::kBlock,
                          FilteringBehaviorReason::DEFAULT);
  }

  void ExpectURLInManualAllowlist(const std::string& url) {
    ExpectURLCheckMatches(url, FilteringBehavior::kAllow,
                          FilteringBehaviorReason::MANUAL);
  }

  void ExpectURLInManualDenylist(const std::string& url) {
    ExpectURLCheckMatches(url, FilteringBehavior::kBlock,
                          FilteringBehaviorReason::MANUAL);
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  SupervisedUserURLFilter filter_ =
      SupervisedUserURLFilter(pref_service_,
                              std::make_unique<FakeURLFilterDelegate>());
  FilteringBehavior behavior_;
  FilteringBehaviorReason reason_;

 private:
  void ExpectURLCheckMatches(const std::string& url,
                             FilteringBehavior expected_behavior,
                             FilteringBehaviorReason expected_reason,
                             bool skip_manual_parent_filter = false) {
    bool called_synchronously = filter_.GetFilteringBehaviorWithAsyncChecks(
        GURL(url), base::DoNothing(), skip_manual_parent_filter);
    ASSERT_TRUE(called_synchronously);

    EXPECT_EQ(behavior_, expected_behavior);
    EXPECT_EQ(reason_, expected_reason);
  }
};

TEST_F(SupervisedUserURLFilterTest, Basic) {
  std::map<std::string, bool> hosts;
  hosts["*.google.com"] = true;

  filter_.SetManualHosts(std::move(hosts));
  filter_.SetDefaultFilteringBehavior(FilteringBehavior::kBlock);

  EXPECT_TRUE(IsURLAllowlisted("http://google.com"));
  EXPECT_TRUE(IsURLAllowlisted("http://google.com/"));
  EXPECT_TRUE(IsURLAllowlisted("http://google.com/whatever"));
  EXPECT_TRUE(IsURLAllowlisted("https://google.com/"));
  EXPECT_FALSE(IsURLAllowlisted("http://notgoogle.com/"));
  EXPECT_TRUE(IsURLAllowlisted("http://mail.google.com"));
  EXPECT_TRUE(IsURLAllowlisted("http://x.mail.google.com"));
  EXPECT_TRUE(IsURLAllowlisted("https://x.mail.google.com/"));
  EXPECT_TRUE(IsURLAllowlisted("http://x.y.google.com/a/b"));
  EXPECT_FALSE(IsURLAllowlisted("http://youtube.com/"));

  EXPECT_TRUE(IsURLAllowlisted("bogus://youtube.com/"));
  EXPECT_TRUE(IsURLAllowlisted("chrome://youtube.com/"));
  EXPECT_TRUE(IsURLAllowlisted("chrome://extensions/"));
  EXPECT_TRUE(IsURLAllowlisted("chrome-extension://foo/main.html"));
  EXPECT_TRUE(
      IsURLAllowlisted("file:///home/chronos/user/MyFiles/Downloads/img.jpg"));
}

TEST_F(SupervisedUserURLFilterTest, EffectiveURL) {
  std::map<std::string, bool> hosts;
  hosts["example.com"] = true;

  filter_.SetManualHosts(std::move(hosts));
  filter_.SetDefaultFilteringBehavior(FilteringBehavior::kBlock);

  ASSERT_TRUE(IsURLAllowlisted("http://example.com"));
  ASSERT_TRUE(IsURLAllowlisted("https://example.com"));

  // AMP Cache URLs.
  EXPECT_FALSE(IsURLAllowlisted("https://cdn.ampproject.org"));
  EXPECT_TRUE(IsURLAllowlisted("https://cdn.ampproject.org/c/example.com"));
  EXPECT_TRUE(IsURLAllowlisted("https://cdn.ampproject.org/c/www.example.com"));
  EXPECT_TRUE(
      IsURLAllowlisted("https://cdn.ampproject.org/c/example.com/path"));
  EXPECT_TRUE(IsURLAllowlisted("https://cdn.ampproject.org/c/s/example.com"));
  EXPECT_FALSE(IsURLAllowlisted("https://cdn.ampproject.org/c/other.com"));

  EXPECT_FALSE(IsURLAllowlisted("https://sub.cdn.ampproject.org"));
  EXPECT_TRUE(IsURLAllowlisted("https://sub.cdn.ampproject.org/c/example.com"));
  EXPECT_TRUE(
      IsURLAllowlisted("https://sub.cdn.ampproject.org/c/www.example.com"));
  EXPECT_TRUE(
      IsURLAllowlisted("https://sub.cdn.ampproject.org/c/example.com/path"));
  EXPECT_TRUE(
      IsURLAllowlisted("https://sub.cdn.ampproject.org/c/s/example.com"));
  EXPECT_FALSE(IsURLAllowlisted("https://sub.cdn.ampproject.org/c/other.com"));

  // Google AMP viewer URLs.
  EXPECT_FALSE(IsURLAllowlisted("https://www.google.com"));
  EXPECT_FALSE(IsURLAllowlisted("https://www.google.com/amp/"));
  EXPECT_TRUE(IsURLAllowlisted("https://www.google.com/amp/example.com"));
  EXPECT_TRUE(IsURLAllowlisted("https://www.google.com/amp/www.example.com"));
  EXPECT_TRUE(IsURLAllowlisted("https://www.google.com/amp/s/example.com"));
  EXPECT_TRUE(
      IsURLAllowlisted("https://www.google.com/amp/s/example.com/path"));
  EXPECT_FALSE(IsURLAllowlisted("https://www.google.com/amp/other.com"));

  // Google web cache URLs.
  EXPECT_FALSE(IsURLAllowlisted("https://webcache.googleusercontent.com"));
  EXPECT_FALSE(
      IsURLAllowlisted("https://webcache.googleusercontent.com/search"));
  EXPECT_FALSE(IsURLAllowlisted(
      "https://webcache.googleusercontent.com/search?q=example.com"));
  EXPECT_TRUE(IsURLAllowlisted(
      "https://webcache.googleusercontent.com/search?q=cache:example.com"));
  EXPECT_TRUE(
      IsURLAllowlisted("https://webcache.googleusercontent.com/"
                       "search?q=cache:example.com+search_query"));
  EXPECT_TRUE(
      IsURLAllowlisted("https://webcache.googleusercontent.com/"
                       "search?q=cache:123456789-01:example.com+search_query"));
  EXPECT_FALSE(IsURLAllowlisted(
      "https://webcache.googleusercontent.com/search?q=cache:other.com"));
  EXPECT_FALSE(
      IsURLAllowlisted("https://webcache.googleusercontent.com/"
                       "search?q=cache:other.com+example.com"));
  EXPECT_FALSE(
      IsURLAllowlisted("https://webcache.googleusercontent.com/"
                       "search?q=cache:123456789-01:other.com+example.com"));

  // Google Translate URLs.
  EXPECT_FALSE(IsURLAllowlisted("https://translate.google.com"));
  EXPECT_FALSE(IsURLAllowlisted("https://translate.googleusercontent.com"));
  EXPECT_TRUE(
      IsURLAllowlisted("https://translate.google.com/translate?u=example.com"));
  EXPECT_TRUE(IsURLAllowlisted(
      "https://translate.googleusercontent.com/translate?u=example.com"));
  EXPECT_TRUE(IsURLAllowlisted(
      "https://translate.google.com/translate?u=www.example.com"));
  EXPECT_TRUE(IsURLAllowlisted(
      "https://translate.google.com/translate?u=https://example.com"));
  EXPECT_FALSE(
      IsURLAllowlisted("https://translate.google.com/translate?u=other.com"));
}

TEST_F(SupervisedUserURLFilterTest, Inactive) {
  filter_.SetDefaultFilteringBehavior(FilteringBehavior::kAllow);
  std::map<std::string, bool> hosts;
  hosts["google.com"] = true;

  filter_.SetManualHosts(std::move(hosts));

  // If the filter is inactive, every URL should be allowed.
  EXPECT_TRUE(IsURLAllowlisted("http://google.com"));
  EXPECT_TRUE(IsURLAllowlisted("https://www.example.com"));
}

TEST_F(SupervisedUserURLFilterTest, IPAddress) {
  std::map<std::string, bool> hosts;
  hosts["123.123.123.123"] = true;

  filter_.SetManualHosts(std::move(hosts));
  filter_.SetDefaultFilteringBehavior(FilteringBehavior::kBlock);

  EXPECT_TRUE(IsURLAllowlisted("http://123.123.123.123/"));
  EXPECT_FALSE(IsURLAllowlisted("http://123.123.123.124/"));
}

TEST_F(SupervisedUserURLFilterTest, Canonicalization) {
  // We assume that the hosts and URLs are already canonicalized.
  std::map<std::string, bool> hosts;
  hosts["www.moose.org"] = true;
  hosts["www.xn--n3h.net"] = true;
  std::map<GURL, bool> urls;
  urls[GURL("http://www.example.com/foo/")] = true;
  urls[GURL("http://www.example.com/%C3%85t%C3%B8mstr%C3%B6m")] = true;
  filter_.SetManualHosts(std::move(hosts));
  filter_.SetManualURLs(std::move(urls));

  // Base cases.
  EXPECT_TRUE(IsURLAllowlisted("http://www.example.com/foo/"));
  EXPECT_TRUE(
      IsURLAllowlisted("http://www.example.com/%C3%85t%C3%B8mstr%C3%B6m"));

  // Verify that non-URI characters are escaped.
  EXPECT_TRUE(IsURLAllowlisted(
      "http://www.example.com/\xc3\x85t\xc3\xb8mstr\xc3\xb6m"));

  // Verify that unnecessary URI escapes remain escaped.
  EXPECT_TRUE(!IsURLAllowlisted("http://www.example.com/%66%6F%6F/"));

  // Verify that the default port are removed.
  EXPECT_TRUE(IsURLAllowlisted("http://www.example.com:80/foo/"));

  // Verify that scheme and hostname are lowercased.
  EXPECT_TRUE(IsURLAllowlisted("htTp://wWw.eXamPle.com/foo/"));
  EXPECT_TRUE(IsURLAllowlisted("HttP://WwW.mOOsE.orG/blurp/"));

  // Verify that UTF-8 in hostnames are converted to punycode.
  EXPECT_TRUE(IsURLAllowlisted("http://www.\xe2\x98\x83\x0a.net/bla/"));

  // Verify that query and ref are stripped.
  EXPECT_TRUE(IsURLAllowlisted("http://www.example.com/foo/?bar=baz#ref"));
}

TEST_F(SupervisedUserURLFilterTest, UrlWithNonStandardUrlSchemeAllowed) {
  // Non-standard url scheme.
  EXPECT_TRUE(IsURLAllowlisted("file://example.com"));
  EXPECT_TRUE(IsURLAllowlisted("filesystem://80cols.com"));
  EXPECT_TRUE(IsURLAllowlisted("chrome://example.com"));
  EXPECT_TRUE(IsURLAllowlisted("wtf://example.com"));
  EXPECT_TRUE(IsURLAllowlisted("gopher://example.com"));

  // Standard url scheme.
  EXPECT_FALSE(IsURLAllowlisted(("http://example.com")));
  EXPECT_FALSE(IsURLAllowlisted("https://example.com"));
  EXPECT_FALSE(IsURLAllowlisted("ftp://example.com"));
  EXPECT_FALSE(IsURLAllowlisted("ws://example.com"));
  EXPECT_FALSE(IsURLAllowlisted("wss://example.com"));
}

TEST_F(SupervisedUserURLFilterTest, HostMatchesPattern) {
  EXPECT_TRUE(SupervisedUserURLFilter::HostMatchesPattern("www.google.com",
                                                          "google.com"));
  EXPECT_TRUE(SupervisedUserURLFilter::HostMatchesPattern("www.google.com",
                                                          "*.google.com"));
  EXPECT_TRUE(SupervisedUserURLFilter::HostMatchesPattern("google.com",
                                                          "*.google.com"));
  EXPECT_TRUE(SupervisedUserURLFilter::HostMatchesPattern("accounts.google.com",
                                                          "*.google.com"));
  EXPECT_FALSE(SupervisedUserURLFilter::HostMatchesPattern("www.google.de",
                                                           "*.google.com"));
  EXPECT_FALSE(SupervisedUserURLFilter::HostMatchesPattern("notgoogle.com",
                                                           "*.google.com"));

  EXPECT_TRUE(SupervisedUserURLFilter::HostMatchesPattern("www.google.com",
                                                          "www.google.*"));
  EXPECT_TRUE(SupervisedUserURLFilter::HostMatchesPattern("www.google.de",
                                                          "www.google.*"));
  EXPECT_TRUE(SupervisedUserURLFilter::HostMatchesPattern("www.google.co.uk",
                                                          "www.google.*"));
  EXPECT_FALSE(SupervisedUserURLFilter::HostMatchesPattern(
      "www.google.blogspot.com", "www.google.*"));
  EXPECT_FALSE(SupervisedUserURLFilter::HostMatchesPattern("www.google",
                                                           "www.google.*"));
  EXPECT_TRUE(SupervisedUserURLFilter::HostMatchesPattern("google.com",
                                                          "www.google.*"));
  EXPECT_FALSE(SupervisedUserURLFilter::HostMatchesPattern("mail.google.com",
                                                           "www.google.*"));
  EXPECT_FALSE(SupervisedUserURLFilter::HostMatchesPattern("www.googleplex.com",
                                                           "www.google.*"));
  EXPECT_FALSE(SupervisedUserURLFilter::HostMatchesPattern("www.googleco.uk",
                                                           "www.google.*"));

  EXPECT_TRUE(SupervisedUserURLFilter::HostMatchesPattern("www.google.com",
                                                          "*.google.*"));
  EXPECT_TRUE(
      SupervisedUserURLFilter::HostMatchesPattern("google.com", "*.google.*"));
  EXPECT_TRUE(SupervisedUserURLFilter::HostMatchesPattern("accounts.google.com",
                                                          "*.google.*"));
  EXPECT_TRUE(SupervisedUserURLFilter::HostMatchesPattern("mail.google.com",
                                                          "*.google.*"));
  EXPECT_TRUE(SupervisedUserURLFilter::HostMatchesPattern("www.google.de",
                                                          "*.google.*"));
  EXPECT_TRUE(
      SupervisedUserURLFilter::HostMatchesPattern("google.de", "*.google.*"));
  EXPECT_FALSE(SupervisedUserURLFilter::HostMatchesPattern(
      "google.blogspot.com", "*.google.*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("google", "*.google.*"));
  EXPECT_FALSE(SupervisedUserURLFilter::HostMatchesPattern("notgoogle.com",
                                                           "*.google.*"));
  EXPECT_FALSE(SupervisedUserURLFilter::HostMatchesPattern("www.googleplex.com",
                                                           "*.google.*"));

  // Now test a few invalid patterns. They should never match.
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com", ""));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com", "."));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com", "*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com", ".*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com", "*."));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com", "*.*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google..com", "*..*"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com", "*.*.com"));
  EXPECT_FALSE(
      SupervisedUserURLFilter::HostMatchesPattern("www.google.com", "www.*.*"));
  EXPECT_FALSE(SupervisedUserURLFilter::HostMatchesPattern("www.google.com",
                                                           "*.goo.*le.*"));
  EXPECT_FALSE(SupervisedUserURLFilter::HostMatchesPattern("www.google.com",
                                                           "*google*"));
  EXPECT_FALSE(SupervisedUserURLFilter::HostMatchesPattern("www.google.com",
                                                           "www.*.google.com"));
}

TEST_F(SupervisedUserURLFilterTest, PatternsWithoutConflicts) {
  std::map<std::string, bool> hosts;

  // The third rule is redundant with the first, but it's not a conflict
  // since they have the same value (allow).
  hosts["*.google.com"] = true;
  hosts["calendar.google.com"] = false;
  hosts["mail.google.com"] = true;

  filter_.SetManualHosts(std::move(hosts));
  filter_.SetDefaultFilteringBehavior(FilteringBehavior::kBlock);

  EXPECT_TRUE(IsURLAllowlisted("http://www.google.com/foo/"));
  EXPECT_FALSE(IsURLAllowlisted("http://calendar.google.com/bar/"));
  EXPECT_TRUE(IsURLAllowlisted("http://mail.google.com/moose/"));
  EXPECT_FALSE(IsURLAllowlisted("http://www.google.co.uk/blurp/"));

  filter_.SetDefaultFilteringBehavior(FilteringBehavior::kAllow);

  EXPECT_TRUE(IsURLAllowlisted("http://www.google.com/foo/"));
  EXPECT_FALSE(IsURLAllowlisted("http://calendar.google.com/bar/"));
  EXPECT_TRUE(IsURLAllowlisted("http://mail.google.com/moose/"));
  EXPECT_TRUE(IsURLAllowlisted("http://www.google.co.uk/blurp/"));
}

TEST_F(SupervisedUserURLFilterTest, PatternsWithConflicts) {
  std::map<std::string, bool> hosts;
  base::HistogramTester histogram_tester;

  // First and second rule always conflicting.
  // The fourth rule conflicts with the first for "www.google.com" host.
  // Blocking then takes precedence.
  hosts["*.google.com"] = true;
  hosts["calendar.google.com"] = false;
  hosts["mail.google.com"] = true;
  hosts["www.google.*"] = false;

  filter_.SetManualHosts(std::move(hosts));
  filter_.SetDefaultFilteringBehavior(FilteringBehavior::kBlock);

  EXPECT_FALSE(IsURLAllowlisted("http://www.google.com/foo/"));
  histogram_tester.ExpectBucketCount(
      SupervisedUserURLFilter::GetManagedSiteListConflictHistogramNameForTest(),
      1, 1);
  // Match with conflicting first and second rule.
  EXPECT_FALSE(IsURLAllowlisted("http://calendar.google.com/bar/"));
  histogram_tester.ExpectBucketCount(
      SupervisedUserURLFilter::GetManagedSiteListConflictHistogramNameForTest(),
      1, 2);

  // Match with first and third rule both allowed, no conflict.
  EXPECT_TRUE(IsURLAllowlisted("http://mail.google.com/moose/"));
  histogram_tester.ExpectBucketCount(
      SupervisedUserURLFilter::GetManagedSiteListConflictHistogramNameForTest(),
      0, 1);

  // Match with fourth rule.
  EXPECT_FALSE(IsURLAllowlisted("http://www.google.co.uk/blurp/"));
  histogram_tester.ExpectBucketCount(
      SupervisedUserURLFilter::GetManagedSiteListConflictHistogramNameForTest(),
      0, 2);

  filter_.SetDefaultFilteringBehavior(FilteringBehavior::kAllow);

  EXPECT_FALSE(IsURLAllowlisted("http://www.google.com/foo/"));
  EXPECT_FALSE(IsURLAllowlisted("http://calendar.google.com/bar/"));
  EXPECT_TRUE(IsURLAllowlisted("http://mail.google.com/moose/"));
  EXPECT_FALSE(IsURLAllowlisted("http://www.google.co.uk/blurp/"));
  histogram_tester.ExpectBucketCount(
      SupervisedUserURLFilter::GetManagedSiteListConflictHistogramNameForTest(),
      1, 4);
  histogram_tester.ExpectBucketCount(
      SupervisedUserURLFilter::GetManagedSiteListConflictHistogramNameForTest(),
      0, 4);

  // No known rule, the metric is not recorded.
  EXPECT_TRUE(IsURLAllowlisted("https://youtube.com"));
  histogram_tester.ExpectTotalCount(
      SupervisedUserURLFilter::GetManagedSiteListConflictHistogramNameForTest(),
      8);
}

TEST_F(SupervisedUserURLFilterTest, Reason) {
  std::map<std::string, bool> hosts;
  std::map<GURL, bool> urls;
  hosts["youtube.com"] = true;
  hosts["*.google.*"] = true;
  urls[GURL("https://youtube.com/robots.txt")] = false;
  urls[GURL("https://google.co.uk/robots.txt")] = false;

  filter_.SetManualHosts(std::move(hosts));
  filter_.SetManualURLs(std::move(urls));

  filter_.SetDefaultFilteringBehavior(FilteringBehavior::kBlock);

  ExpectURLInDefaultDenylist("https://m.youtube.com/feed/trending");
  ExpectURLInDefaultDenylist("https://com.google");
  ExpectURLInManualAllowlist("https://youtube.com/feed/trending");
  ExpectURLInManualAllowlist("https://google.com/humans.txt");
  ExpectURLInManualDenylist("https://youtube.com/robots.txt");
  ExpectURLInManualDenylist("https://google.co.uk/robots.txt");

  filter_.SetDefaultFilteringBehavior(FilteringBehavior::kAllow);

  ExpectURLInManualAllowlist("https://youtube.com/feed/trending");
  ExpectURLInManualAllowlist("https://google.com/humans.txt");
  ExpectURLInManualDenylist("https://youtube.com/robots.txt");
  ExpectURLInManualDenylist("https://google.co.uk/robots.txt");
}

TEST_F(SupervisedUserURLFilterTest, UrlsNotRequiringGuardianApprovalAllowed) {
  filter_.SetDefaultFilteringBehavior(FilteringBehavior::kBlock);
  EXPECT_TRUE(IsURLAllowlisted("https://families.google.com/"));
  EXPECT_TRUE(IsURLAllowlisted("https://families.google.com"));
  EXPECT_TRUE(IsURLAllowlisted("https://families.google.com/something"));
  EXPECT_TRUE(IsURLAllowlisted("http://families.google.com/"));
  EXPECT_FALSE(IsURLAllowlisted("https://subdomain.families.google.com/"));
  EXPECT_TRUE(IsURLAllowlisted("https://myaccount.google.com/"));
  EXPECT_TRUE(IsURLAllowlisted("https://accounts.google.com/"));
  EXPECT_TRUE(IsURLAllowlisted("https://familylink.google.com/"));
  EXPECT_TRUE(IsURLAllowlisted("https://policies.google.com/"));
  EXPECT_TRUE(IsURLAllowlisted("https://support.google.com/"));

  // Chrome sync dashboard URLs (base initial URL, plus the version with locale
  // appended, and the redirect URL with locale appended).
  EXPECT_TRUE(IsURLAllowlisted("https://www.google.com/settings/chrome/sync"));
  EXPECT_TRUE(
      IsURLAllowlisted("https://www.google.com/settings/chrome/sync?hl=en-US"));
  EXPECT_TRUE(IsURLAllowlisted("https://chrome.google.com/sync?hl=en-US"));
}

TEST_F(SupervisedUserURLFilterTest, PlayTermsAlwaysAllowed) {
  filter_.SetDefaultFilteringBehavior(FilteringBehavior::kBlock);
  EXPECT_TRUE(IsURLAllowlisted("https://play.google/play-terms"));
  EXPECT_FALSE(IsURLAllowlisted("https://play.google.com/about/play-terms"));
  EXPECT_TRUE(IsURLAllowlisted("https://play.google/play-terms/"));
  EXPECT_FALSE(IsURLAllowlisted("https://play.google.com/about/play-terms/"));
  EXPECT_TRUE(
      IsURLAllowlisted("https://play.google/intl/pt-BR_pt/play-terms/"));
  EXPECT_FALSE(IsURLAllowlisted(
      "https://play.google.com/intl/pt-BR_pt/about/play-terms/"));
  EXPECT_TRUE(IsURLAllowlisted("https://play.google/play-terms/index.html"));
  EXPECT_FALSE(
      IsURLAllowlisted("https://play.google.com/about/play-terms/index.html"));
  EXPECT_FALSE(IsURLAllowlisted("http://play.google/play-terms/"));
  EXPECT_FALSE(IsURLAllowlisted("http://play.google.com/about/play-terms/"));
  EXPECT_FALSE(IsURLAllowlisted("https://subdomain.play.google/play-terms/"));
  EXPECT_FALSE(
      IsURLAllowlisted("https://subdomain.play.google.com/about/play-terms/"));
  EXPECT_FALSE(IsURLAllowlisted("https://play.google/"));
  EXPECT_FALSE(IsURLAllowlisted("https://play.google.com/"));
  EXPECT_FALSE(IsURLAllowlisted("https://play.google/about"));
  EXPECT_FALSE(IsURLAllowlisted("https://play.google.com/about"));
}

TEST_F(SupervisedUserURLFilterTest, IsGwsRedirectorExemptedFromApprovals) {
  base::test::ScopedFeatureList feature_list{
      kExemptGuardianApprovalOnGwsRedirector};

  CHECK(filter_.GetDefaultFilteringBehavior() == FilteringBehavior::kBlock);

  EXPECT_TRUE(IsURLAllowlisted(
      "http://www.google.com/url?q=http://example.com&source=test"));
  EXPECT_FALSE(
      IsURLAllowlisted("http://www.google.com/url?q=http://example.com"))
      << "GWS urls without source are not exempted.";
  EXPECT_FALSE(IsURLAllowlisted("http://www.google.com/url?source=test"))
      << "GWS urls without destination are not exempted.";
  EXPECT_FALSE(IsURLAllowlisted("http://google.com/url?source=test"))
      << "GWS urls without www are not exempted.";
}

TEST_F(SupervisedUserURLFilterTest, IsGwsRedirectorDenied) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(kExemptGuardianApprovalOnGwsRedirector);

  CHECK(filter_.GetDefaultFilteringBehavior() == FilteringBehavior::kBlock);
  EXPECT_FALSE(IsURLAllowlisted(
      "http://www.google.com/url?q=http://example.com&source=test"));
}

class SupervisedUserURLFilteringWithConflictsTest
    : public testing::TestWithParam<std::tuple<
          std::map<std::string, bool>,
          std::optional<
              SupervisedUserURLFilter::FilteringSubdomainConflictType>>> {
 public:
  SupervisedUserURLFilteringWithConflictsTest() {
    PrefRegistrySimple* registry = pref_service_.registry();
    RegisterProfilePrefs(registry);
    filter_.SetURLCheckerClient(
        std::make_unique<safe_search_api::FakeURLCheckerClient>());
    filter_.SetDefaultFilteringBehavior(FilteringBehavior::kBlock);
  }

 protected:
  bool IsURLAllowlisted(const std::string& url) {
    GURL gurl = GURL(url);
    CHECK(gurl.is_valid());
    return filter_.GetFilteringBehavior(gurl).IsAllowed();
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  SupervisedUserURLFilter filter_ =
      SupervisedUserURLFilter(pref_service_,
                              std::make_unique<FakeURLFilterDelegate>());
};

// Tests that the new histogram that records www-subdomain conflicts
// increases only when the corresponding conflict types occurs.
TEST_P(SupervisedUserURLFilteringWithConflictsTest,
       PatternsWithSubdomainConflicts) {
  base::HistogramTester histogram_tester;

  auto host_map = std::get<0>(GetParam());
  auto conflict_type = std::get<1>(GetParam());
  filter_.SetManualHosts(std::move(host_map));

  EXPECT_FALSE(IsURLAllowlisted("https://www.google.com"));

  if (conflict_type.has_value()) {
    histogram_tester.ExpectBucketCount(
        SupervisedUserURLFilter::
            GetManagedSiteListConflictTypeHistogramNameForTest(),
        /*sample=*/conflict_type.value(), /*expected_count=*/1);
  } else {
    // When there is no conflict, no entries are recorded.
    histogram_tester.ExpectTotalCount(
        SupervisedUserURLFilter::
            GetManagedSiteListConflictTypeHistogramNameForTest(),
        /*expected_count=*/0);
  }
}

// Tests that conflict tracking histogram records a result for no conflicts
// even for paths that determine a result and exit early.
TEST_F(SupervisedUserURLFilteringWithConflictsTest,
       PatterWithoutConflictOnEarlyExit) {
  base::HistogramTester histogram_tester;
  // The host map is empty but the url map contains an exact match.
  std::map<std::string, bool> host_map;
  std::map<GURL, bool> url_map =
      std::map<GURL, bool>({{GURL("https://www.google.com"), true}});
  filter_.SetManualHosts(std::move(host_map));
  filter_.SetManualURLs(std::move(url_map));

  EXPECT_TRUE(IsURLAllowlisted("https://www.google.com"));

  // When there is no conflict, no entries as recorded in the conflict type
  // histogram. A non-conflict entry is recorded on the conflict tracking histogram.
  histogram_tester.ExpectTotalCount(
      SupervisedUserURLFilter::
          GetManagedSiteListConflictTypeHistogramNameForTest(),
      /*expected_count=*/0);
  histogram_tester.ExpectBucketCount(
      SupervisedUserURLFilter::GetManagedSiteListConflictHistogramNameForTest(),
      /*sample=*/0, /*expected_count=*/1);
}

INSTANTIATE_TEST_SUITE_P(
    SubdomainConflicts,
    SupervisedUserURLFilteringWithConflictsTest,
    testing::Values(
        /* Only trivial subdomain conflicts: */
        std::make_tuple(
            /* host_map= */ std::map<std::string, bool>(
                {{"www.google.com", true}, {"https://google.com", false}}),
            SupervisedUserURLFilter::FilteringSubdomainConflictType::
                kTrivialSubdomainConflictOnly),
        std::make_tuple(
            /* host_map= */ std::map<std::string, bool>(
                {{"www.google.com", false}, {"https://google.com", true}}),
            SupervisedUserURLFilter::FilteringSubdomainConflictType::
                kTrivialSubdomainConflictOnly),
        std::make_tuple(
            /* host_map= */ std::map<std::string, bool>(
                {{"http://www.google.*", false}, {"google.*", true}}),
            SupervisedUserURLFilter::FilteringSubdomainConflictType::
                kTrivialSubdomainConflictOnly),
        std::make_tuple(
            // The collision happens because of the trivial subdomain collision
            // between google.com and other entries.
            /* host_map= */ std::map<std::string, bool>(
                {{"https://www.google.com", false},
                 {"google.com", true},
                 {"www.google.com", false},
                 {"http://www.google.com", false}}),
            SupervisedUserURLFilter::FilteringSubdomainConflictType::
                kTrivialSubdomainConflictOnly),
        std::make_tuple(
            // The collision happens because of the trivial subdomain collision
            // between https://google.com and www.google.com.
            /* host_map= */ std::map<std::string, bool>(
                {{"https://google.com", false},
                 {"www.google.com", true},
                 {"*.google.*", false}}),
            SupervisedUserURLFilter::FilteringSubdomainConflictType::
                kTrivialSubdomainConflictOnly),
        std::make_tuple(
            // The collision happens because of the trivial subdomain collision
            // between https://google.com and www.google.com.
            std::map<std::string, bool>({{"https://www.google.com", false},
                                         {"www.google.*", false},
                                         {"google.com", true}}),
            SupervisedUserURLFilter::FilteringSubdomainConflictType::
                kTrivialSubdomainConflictOnly),
        /* Only other conflicts: */
        std::make_tuple(
            /* host_map= */ std::map<std::string, bool>(
                {{"http://www.google.com", false}, {"*.google.*", true}}),
            SupervisedUserURLFilter::FilteringSubdomainConflictType::
                kOtherConflictOnly),
        std::make_tuple(
            /* host_map= */ std::map<std::string, bool>(
                {{"*.google.com", false}, {"www.google.com", true}}),
            SupervisedUserURLFilter::FilteringSubdomainConflictType::
                kOtherConflictOnly),
        std::make_tuple(
            /* host_map= */ std::map<std::string, bool>(
                {{"http://www.google.com", false}, {"www.google.*", true}}),
            SupervisedUserURLFilter::FilteringSubdomainConflictType::
                kOtherConflictOnly),
        std::make_tuple(
            /* host_map= */ std::map<std::string, bool>(
                {{"http://google.com", false},
                 {"https://google.com", true},
                 {"*.google.com", true}}),
            SupervisedUserURLFilter::FilteringSubdomainConflictType::
                kOtherConflictOnly),
        /* No conflicts: */
        std::make_tuple(
            /* host_map= */ std::map<std::string, bool>(
                {{"http://google.com", false}, {"www.google.com", false}}),
            std::nullopt),
        /* Mix of www-subdomain conflicts and other conflicts */
        std::make_tuple(
            /* host_map= */ std::map<std::string, bool>(
                {{"https://google.com", false},
                 {"www.google.com", true},
                 {"*.google.com", true}}),  // Other conflict entry
            SupervisedUserURLFilter::FilteringSubdomainConflictType::
                kTrivialSubdomainConflictAndOtherConflict),
        std::make_tuple(
            /* host_map= */ std::map<std::string, bool>(
                {{"https://google.com", true},
                 {"www.google.com", false},
                 {"*.google.*", true}}),  // Other conflict entry
            SupervisedUserURLFilter::FilteringSubdomainConflictType::
                kTrivialSubdomainConflictAndOtherConflict),
        std::make_tuple(
            /* host_map= */ std::map<std::string, bool>(
                {{"https://www.google.com", true},
                 {"google.com", false},
                 {"google.*", true},  // Other conflict entry
                 {"*.google.*", false}}),
            SupervisedUserURLFilter::FilteringSubdomainConflictType::
                kTrivialSubdomainConflictAndOtherConflict)));

struct MetricTestParam {
  // Context of filtering
  FilteringContext context;

  // Name of the histogram to emit that is specific for context (alongside
  // aggregated and legacy histograms).
  std::string context_specific_histogram;

  // Human-readable label of test case.
  std::string label;
};

class SupervisedUserURLFilterMetricsTest
    : public ::testing::TestWithParam<MetricTestParam> {
 protected:
  void EnableSafeSites() {
    RegisterProfilePrefs(pref_service_.registry());
    pref_service_.SetBoolean(prefs::kSupervisedUserSafeSites, true);
    pref_service_.SetString(prefs::kSupervisedUserId, kChildAccountSUID);
  }

  base::HistogramTester histogram_tester_;
  TestingPrefServiceSimple pref_service_;
  SupervisedUserURLFilter filter_{pref_service_,
                                  std::make_unique<FakeURLFilterDelegate>()};
};

TEST_P(SupervisedUserURLFilterMetricsTest,
       RecordsTopLevelMetricsForBlockNotInAllowlist) {
  filter_.SetDefaultFilteringBehavior(FilteringBehavior::kBlock);

  ASSERT_TRUE(filter_.GetFilteringBehaviorWithAsyncChecks(
      GURL("http://example.com"), base::DoNothing(), false,
      GetParam().context));

  if (GetParam().context == FilteringContext::kNavigationThrottle) {
    histogram_tester_.ExpectBucketCount(
        "ManagedUsers.TopLevelFilteringResult",
        SupervisedUserFilterTopLevelResult::kBlockNotInAllowlist, 1);
  }
  histogram_tester_.ExpectBucketCount(
      "ManagedUsers.TopLevelFilteringResult2",
      SupervisedUserFilterTopLevelResult::kBlockNotInAllowlist, 1);
  histogram_tester_.ExpectBucketCount(
      GetParam().context_specific_histogram,
      SupervisedUserFilterTopLevelResult::kBlockNotInAllowlist, 1);
}

TEST_P(SupervisedUserURLFilterMetricsTest, RecordsTopLevelMetricsForAllow) {
  filter_.SetManualHosts({{"http://example.com", true}});
  filter_.SetDefaultFilteringBehavior(FilteringBehavior::kBlock);

  ASSERT_TRUE(filter_.GetFilteringBehaviorWithAsyncChecks(
      GURL("http://example.com"), base::DoNothing(), false,
      GetParam().context));

  if (GetParam().context == FilteringContext::kNavigationThrottle) {
    histogram_tester_.ExpectBucketCount(
        "ManagedUsers.TopLevelFilteringResult",
        SupervisedUserFilterTopLevelResult::kAllow, 1);
  }
  histogram_tester_.ExpectBucketCount(
      "ManagedUsers.TopLevelFilteringResult2",
      SupervisedUserFilterTopLevelResult::kAllow, 1);
  histogram_tester_.ExpectBucketCount(
      GetParam().context_specific_histogram,
      SupervisedUserFilterTopLevelResult::kAllow, 1);
}

TEST_P(SupervisedUserURLFilterMetricsTest,
       RecordsTopLevelMetricsForBlockManual) {
  filter_.SetManualHosts({{"http://example.com", false}});
  filter_.SetDefaultFilteringBehavior(FilteringBehavior::kAllow);

  ASSERT_TRUE(filter_.GetFilteringBehaviorWithAsyncChecks(
      GURL("http://example.com"), base::DoNothing(), false,
      GetParam().context));

  if (GetParam().context == FilteringContext::kNavigationThrottle) {
    histogram_tester_.ExpectBucketCount(
        "ManagedUsers.TopLevelFilteringResult",
        SupervisedUserFilterTopLevelResult::kBlockManual, 1);
  }
  histogram_tester_.ExpectBucketCount(
      "ManagedUsers.TopLevelFilteringResult2",
      SupervisedUserFilterTopLevelResult::kBlockManual, 1);
  histogram_tester_.ExpectBucketCount(
      GetParam().context_specific_histogram,
      SupervisedUserFilterTopLevelResult::kBlockManual, 1);
}

TEST_P(SupervisedUserURLFilterMetricsTest,
       RecordsTopLevelMetricsForAsyncBlock) {
  EnableSafeSites();
  std::unique_ptr<safe_search_api::FakeURLCheckerClient> client =
      std::make_unique<safe_search_api::FakeURLCheckerClient>();
  safe_search_api::FakeURLCheckerClient* client_ptr = client.get();
  filter_.SetURLCheckerClient(std::move(client));

  ASSERT_FALSE(filter_.GetFilteringBehaviorWithAsyncChecks(
      GURL("http://example.com"), base::DoNothing(), false,
      GetParam().context));
  client_ptr->RunCallback(safe_search_api::ClientClassification::kRestricted);

  if (GetParam().context == FilteringContext::kNavigationThrottle) {
    histogram_tester_.ExpectBucketCount(
        "ManagedUsers.TopLevelFilteringResult",
        SupervisedUserFilterTopLevelResult::kBlockSafeSites, 1);
  }
  histogram_tester_.ExpectBucketCount(
      "ManagedUsers.TopLevelFilteringResult2",
      SupervisedUserFilterTopLevelResult::kBlockSafeSites, 1);
  histogram_tester_.ExpectBucketCount(
      GetParam().context_specific_histogram,
      SupervisedUserFilterTopLevelResult::kBlockSafeSites, 1);
}

TEST_P(SupervisedUserURLFilterMetricsTest,
       RecordsTopLevelMetricsForAsyncAllow) {
  EnableSafeSites();
  std::unique_ptr<safe_search_api::FakeURLCheckerClient> client =
      std::make_unique<safe_search_api::FakeURLCheckerClient>();
  safe_search_api::FakeURLCheckerClient* client_ptr = client.get();
  filter_.SetURLCheckerClient(std::move(client));

  ASSERT_FALSE(filter_.GetFilteringBehaviorWithAsyncChecks(
      GURL("http://example.com"), base::DoNothing(), false,
      GetParam().context));
  client_ptr->RunCallback(safe_search_api::ClientClassification::kAllowed);

  if (GetParam().context == FilteringContext::kNavigationThrottle) {
    histogram_tester_.ExpectBucketCount(
        "ManagedUsers.TopLevelFilteringResult",
        SupervisedUserFilterTopLevelResult::kAllow, 1);
  }
  histogram_tester_.ExpectBucketCount(
      "ManagedUsers.TopLevelFilteringResult2",
      SupervisedUserFilterTopLevelResult::kAllow, 1);
  histogram_tester_.ExpectBucketCount(
      GetParam().context_specific_histogram,
      SupervisedUserFilterTopLevelResult::kAllow, 1);
}

const MetricTestParam kMetricTestParams[] = {
    {.context = FilteringContext::kNavigationThrottle,
     .context_specific_histogram =
         "ManagedUsers.TopLevelFilteringResult2.NavigationThrottle",
     .label = "NavigationThrottleContext"},
    {.context = FilteringContext::kDefault,
     .context_specific_histogram =
         "ManagedUsers.TopLevelFilteringResult2.Default",
     .label = "DefaultContext"},
    {.context = FilteringContext::kNavigationObserver,
     .context_specific_histogram =
         "ManagedUsers.TopLevelFilteringResult2.NavigationObserver",
     .label = "NavigationObserverContext"},
    {.context = FilteringContext::kFamilyLinkSettingsUpdated,
     .context_specific_histogram =
         "ManagedUsers.TopLevelFilteringResult2.FamilyLinkSettingsUpdated",
     .label = "FamilyLinkSettingsUpdated"},
};

INSTANTIATE_TEST_SUITE_P(,
                         SupervisedUserURLFilterMetricsTest,
                         testing::ValuesIn(kMetricTestParams),
                         [](const auto& info) { return info.param.label; });

TEST(SupervisedUserURLFilterResultTest, IsFromManualList) {
  SupervisedUserURLFilter::Result allow{GURL("http://example.com"),
                                        FilteringBehavior::kAllow,
                                        FilteringBehaviorReason::MANUAL};
  SupervisedUserURLFilter::Result block{GURL("http://example.com"),
                                        FilteringBehavior::kBlock,
                                        FilteringBehaviorReason::MANUAL};
  SupervisedUserURLFilter::Result invalid{GURL("http://example.com"),
                                          FilteringBehavior::kInvalid,
                                          FilteringBehaviorReason::MANUAL};

  EXPECT_TRUE(allow.IsFromManualList());
  EXPECT_TRUE(block.IsFromManualList());
  EXPECT_TRUE(invalid.IsFromManualList());

  EXPECT_FALSE(allow.IsFromDefaultSetting());
  EXPECT_FALSE(block.IsFromDefaultSetting());
  EXPECT_FALSE(invalid.IsFromDefaultSetting());
}

TEST(SupervisedUserURLFilterResultTest, IsFromDefaultSetting) {
  SupervisedUserURLFilter::Result allow{GURL("http://example.com"),
                                        FilteringBehavior::kAllow,
                                        FilteringBehaviorReason::DEFAULT};
  SupervisedUserURLFilter::Result block{GURL("http://example.com"),
                                        FilteringBehavior::kBlock,
                                        FilteringBehaviorReason::DEFAULT};
  SupervisedUserURLFilter::Result invalid{GURL("http://example.com"),
                                          FilteringBehavior::kInvalid,
                                          FilteringBehaviorReason::DEFAULT};

  EXPECT_TRUE(allow.IsFromDefaultSetting());
  EXPECT_TRUE(block.IsFromDefaultSetting());
  EXPECT_TRUE(invalid.IsFromDefaultSetting());

  EXPECT_FALSE(allow.IsFromManualList());
  EXPECT_FALSE(block.IsFromManualList());
  EXPECT_FALSE(invalid.IsFromManualList());
}

TEST(SupervisedUserURLFilterResultTest, IsClassificationSuccessful) {
  SupervisedUserURLFilter::Result allow_from_list{
      GURL("http://example.com"), FilteringBehavior::kAllow,
      FilteringBehaviorReason::MANUAL};
  SupervisedUserURLFilter::Result allow_from_settings{
      GURL("http://example.com"), FilteringBehavior::kAllow,
      FilteringBehaviorReason::DEFAULT};
  SupervisedUserURLFilter::Result allow_from_server{
      GURL("http://example.com"), FilteringBehavior::kAllow,
      FilteringBehaviorReason::ASYNC_CHECKER,
      std::optional<ClassificationDetails>(
          {.reason = ClassificationDetails::Reason::kFreshServerResponse})};
  SupervisedUserURLFilter::Result allow_from_cache{
      GURL("http://example.com"), FilteringBehavior::kAllow,
      FilteringBehaviorReason::ASYNC_CHECKER,
      std::optional<ClassificationDetails>(
          {.reason = ClassificationDetails::Reason::kCachedResponse})};

  SupervisedUserURLFilter::Result block_from_list{
      GURL("http://example.com"), FilteringBehavior::kBlock,
      FilteringBehaviorReason::MANUAL};
  SupervisedUserURLFilter::Result block_from_settings{
      GURL("http://example.com"), FilteringBehavior::kBlock,
      FilteringBehaviorReason::DEFAULT};
  SupervisedUserURLFilter::Result block_from_server{
      GURL("http://example.com"), FilteringBehavior::kBlock,
      FilteringBehaviorReason::ASYNC_CHECKER,
      std::optional<ClassificationDetails>(
          {.reason = ClassificationDetails::Reason::kFreshServerResponse})};
  SupervisedUserURLFilter::Result block_from_cache{
      GURL("http://example.com"), FilteringBehavior::kBlock,
      FilteringBehaviorReason::ASYNC_CHECKER,
      std::optional<ClassificationDetails>(
          {.reason = ClassificationDetails::Reason::kCachedResponse})};

  EXPECT_TRUE(allow_from_list.IsClassificationSuccessful());
  EXPECT_TRUE(allow_from_settings.IsClassificationSuccessful());
  EXPECT_TRUE(allow_from_server.IsClassificationSuccessful());
  EXPECT_TRUE(allow_from_cache.IsClassificationSuccessful());
  EXPECT_TRUE(block_from_list.IsClassificationSuccessful());
  EXPECT_TRUE(block_from_settings.IsClassificationSuccessful());
  EXPECT_TRUE(block_from_server.IsClassificationSuccessful());
  EXPECT_TRUE(block_from_cache.IsClassificationSuccessful());
}

TEST(SupervisedUserURLFilterResultTest, IsClassificationNotSuccessful) {
  SupervisedUserURLFilter::Result allow_from_server{
      GURL("http://example.com"), FilteringBehavior::kAllow,
      FilteringBehaviorReason::ASYNC_CHECKER,
      std::optional<ClassificationDetails>(
          {.reason = ClassificationDetails::Reason::kFailedUseDefault})};
  SupervisedUserURLFilter::Result allow_from_cache{
      GURL("http://example.com"), FilteringBehavior::kAllow,
      FilteringBehaviorReason::ASYNC_CHECKER,
      std::optional<ClassificationDetails>(
          {.reason = ClassificationDetails::Reason::kFailedUseDefault})};

  SupervisedUserURLFilter::Result block_from_server{
      GURL("http://example.com"), FilteringBehavior::kBlock,
      FilteringBehaviorReason::ASYNC_CHECKER,
      std::optional<ClassificationDetails>(
          {.reason = ClassificationDetails::Reason::kFailedUseDefault})};
  SupervisedUserURLFilter::Result block_from_cache{
      GURL("http://example.com"), FilteringBehavior::kBlock,
      FilteringBehaviorReason::ASYNC_CHECKER,
      std::optional<ClassificationDetails>(
          {.reason = ClassificationDetails::Reason::kFailedUseDefault})};

  EXPECT_FALSE(allow_from_server.IsClassificationSuccessful());
  EXPECT_FALSE(allow_from_cache.IsClassificationSuccessful());
  EXPECT_FALSE(block_from_server.IsClassificationSuccessful());
  EXPECT_FALSE(block_from_cache.IsClassificationSuccessful());
}

}  // namespace
}  // namespace supervised_user
