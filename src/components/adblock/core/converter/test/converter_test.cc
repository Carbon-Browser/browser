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

#include <set>
#include <sstream>

#include "base/memory/scoped_refptr.h"
#include "base/rand_util.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/content_type.h"
#include "components/adblock/core/common/header_filter_data.h"
#include "components/adblock/core/common/sitekey.h"
#include "components/adblock/core/converter/converter.h"
#include "components/adblock/core/converter/test/mock_snippet_tokenizer.h"
#include "components/adblock/core/schema/filter_list_schema_generated.h"
#include "components/adblock/core/subscription/installed_subscription_impl.h"
#include "components/adblock/core/subscription/subscription.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/url_constants.h"

namespace adblock {

static ContentType all_content_types[]{
    ContentType::Image,       ContentType::Script, ContentType::Stylesheet,
    ContentType::Subdocument, ContentType::Ping,   ContentType::Xmlhttprequest,
    ContentType::Websocket,   ContentType::Webrtc, ContentType::Media,
    ContentType::Font,        ContentType::Object, ContentType::Other,
};

std::string RandomAsciiString(size_t length) {
  std::string result(length, ' ');
  for (auto& c : result)
    c = base::RandInt('a', 'z');
  return result;
}
struct FlatIndex {
  explicit FlatIndex(std::unique_ptr<FlatbufferData> data)
      : buffer_(std::move(data)),
        index_(flat::GetSubscription(buffer_->data())) {}
  const std::unique_ptr<FlatbufferData> buffer_;
  const flat::Subscription* index_;
};

class AdblockConverterTest : public testing::Test {
 public:
  std::set<base::StringPiece> FilterSelectors(
      InstalledSubscription::Selectors selectors) {
    // Remove exceptions.
    selectors.elemhide_selectors.erase(
        std::remove_if(selectors.elemhide_selectors.begin(),
                       selectors.elemhide_selectors.end(),
                       [&](const auto& selector) {
                         return std::find(selectors.elemhide_exceptions.begin(),
                                          selectors.elemhide_exceptions.end(),
                                          selector) !=
                                selectors.elemhide_exceptions.end();
                       }),
        selectors.elemhide_selectors.end());
    std::set<base::StringPiece> filtered_selectors(
        selectors.elemhide_selectors.begin(),
        selectors.elemhide_selectors.end());
    return filtered_selectors;
  }

  scoped_refptr<InstalledSubscription> ConvertAndLoadRules(
      std::string rules,
      GURL url = GURL(),
      bool allow_privileged = false) {
    // Without [Adblock Plus 2.0], the file is not a valid filter list.
    rules = "[Adblock Plus 2.0]\n! Title: TestingList\n" + rules;
    std::stringstream input(std::move(rules));
    auto converter_result = Converter().Convert(input, {url, allow_privileged});
    return base::MakeRefCounted<InstalledSubscriptionImpl>(
        std::move(converter_result.data),
        Subscription::InstallationState::Installed, base::Time());
  }

  void ExpectSnippetTokenizerWill(const base::StringPiece input,
                                  SnippetTokenizer::SnippetScript result) {
    tokenizer = std::make_unique<MockSnippetTokenizer>();
    EXPECT_CALL(*tokenizer, Tokenize(input))
        .WillRepeatedly(testing::Return(result));
  }

  std::unique_ptr<FlatIndex> ConvertAndLoadRulesToIndex(
      std::string rules,
      GURL url = GURL(),
      bool allow_privileged = false) {
    // Without [Adblock Plus 2.0], the file is not a valid filter list.
    rules = "[Adblock Plus 2.0]\n! Title: TestingList\n" + rules;
    std::stringstream input(std::move(rules));
    if (tokenizer) {
      return std::make_unique<FlatIndex>(
          Converter(std::move(tokenizer))
              .Convert(input, {url, allow_privileged})
              .data);
    }

    return std::make_unique<FlatIndex>(
        Converter().Convert(input, {url, allow_privileged}).data);
  }

  enum ContentTypePolicy { ContentTypeIsSpecific, ContentTypeIsGeneric };
  bool VerifyRequestBlocked(scoped_refptr<InstalledSubscription> subscription,
                            GURL url,
                            std::string domain,
                            ContentType content_type,
                            ContentTypePolicy policy) {
    const bool blocks_request = subscription->HasUrlFilter(
        url, domain, content_type, SiteKey(), FilterCategory::Blocking);
    const bool has_allow_filter = subscription->HasUrlFilter(
        url, domain, content_type, SiteKey(), FilterCategory::Allowing);

    if (policy == ContentTypeIsSpecific) {
      bool blocks_other_types = false;
      for (auto other_type : all_content_types) {
        if (other_type == content_type)
          continue;
        blocks_other_types &= subscription->HasUrlFilter(
            url, domain, other_type, SiteKey(), FilterCategory::Blocking);
      }
      return blocks_request && !blocks_other_types && !has_allow_filter;
    }
    return blocks_request && !has_allow_filter;
  }

  bool VerifyRequestAllowed(scoped_refptr<InstalledSubscription> subscription,
                            GURL url,
                            std::string domain,
                            ContentType content_type) {
    const bool blocks_request = subscription->HasUrlFilter(
        url, domain, content_type, SiteKey(), FilterCategory::Blocking);
    const bool has_allow_filter = subscription->HasUrlFilter(
        url, domain, content_type, SiteKey(), FilterCategory::Allowing);

    bool allows_other_types = false;
    for (auto other_type : all_content_types) {
      if (other_type == content_type)
        continue;
      allows_other_types &= subscription->HasUrlFilter(
          url, domain, other_type, SiteKey(), FilterCategory::Allowing);
    }
    return blocks_request && !allows_other_types && has_allow_filter;
  }
  std::unique_ptr<MockSnippetTokenizer> tokenizer;
};

TEST_F(AdblockConverterTest, SubscriptionUrlParsed) {
  const GURL kSubscriptionUrl{
      "https://testpages.adblockplus.org/en/abp-testcase-subscription.txt"};
  auto subscription = ConvertAndLoadRules("", kSubscriptionUrl);
  EXPECT_EQ(subscription->GetSourceUrl(), kSubscriptionUrl);
}

TEST_F(AdblockConverterTest, SubscriptionTitleParsed) {
  const GURL kSubscriptionUrl{
      "https://testpages.adblockplus.org/en/abp-testcase-subscription.txt"};
  auto subscription = ConvertAndLoadRules("", kSubscriptionUrl);
  EXPECT_EQ(subscription->GetTitle(), "TestingList");
}

TEST_F(AdblockConverterTest, CurrentVersionParsed) {
  const GURL kSubscriptionUrl{
      "https://testpages.adblockplus.org/en/abp-testcase-subscription.txt"};
  auto subscription = ConvertAndLoadRules(
      "! Version: 202108191113\n !Expires: 1 d", kSubscriptionUrl);
  EXPECT_EQ(subscription->GetCurrentVersion(), "202108191113");
  EXPECT_EQ(subscription->GetExpirationInterval(), base::Days(1));
}

TEST_F(AdblockConverterTest, UrlFilter_NoRules) {
  auto subscription = ConvertAndLoadRules("");
  EXPECT_FALSE(subscription->HasUrlFilter(GURL("https://untracked.com/file"),
                                          "domain.com", ContentType::Stylesheet,
                                          SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_Stylesheet) {
  auto subscription = ConvertAndLoadRules(R"(
    testpages.adblockplus.org/testfiles/stylesheet/$stylesheet
  )");
  EXPECT_TRUE(VerifyRequestBlocked(
      subscription,
      GURL("https://testpages.adblockplus.org/testfiles/stylesheet/file.css"),
      "domain.com", ContentType::Stylesheet, ContentTypeIsSpecific));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_Image) {
  auto subscription = ConvertAndLoadRules(R"(
    ||testpages.adblockplus.org/testfiles/image/static/$image
  )");

  EXPECT_TRUE(VerifyRequestBlocked(
      subscription,
      GURL(
          "https://testpages.adblockplus.org/testfiles/image/static/image.png"),
      "domain.com", ContentType::Image, ContentTypeIsSpecific));
}

TEST_F(AdblockConverterTest,
       UrlFilterBlocking_HandlesLookaroundUrlBlockingFiltersNegativeLookahead) {
  // The filter below employs negative lookahead
  // it should match any url except the ones that are within
  // the (?!) group.
  auto subscription = ConvertAndLoadRules(R"(
    /^https?://(?![^\s]+\.streamvid\.club|api\.kinogram\.best\/embed\/|cdn\.jsdelivr\.net\/npm\/venom-player)/$third-party,xmlhttprequest,domain=kindkino.ru
    )");

  EXPECT_FALSE(VerifyRequestBlocked(
      subscription, GURL("https://something.streamvid.club"), "kindkino.ru",
      ContentType::Xmlhttprequest, ContentTypeIsGeneric));

  EXPECT_TRUE(VerifyRequestBlocked(subscription, GURL("https://foo.org"),
                                   "kindkino.ru", ContentType::Xmlhttprequest,
                                   ContentTypeIsGeneric));
}

TEST_F(AdblockConverterTest,
       UrlFilterBlocking_HandlesLookaroundUrlBlockingFiltersCaseSensetive) {
  auto subscription = ConvertAndLoadRules(R"(
    /^https?://(?![^\s]+\.streamvid\.club/case)/$third-party,match-case,xmlhttprequest,domain=kindkino.ru
    )");

  EXPECT_FALSE(VerifyRequestBlocked(
      subscription, GURL("https://something.streamvid.club/case"),
      "kindkino.ru", ContentType::Xmlhttprequest, ContentTypeIsGeneric));

  EXPECT_TRUE(VerifyRequestBlocked(
      subscription, GURL("https://something.streamvid.club/CASE"),
      "kindkino.ru", ContentType::Xmlhttprequest, ContentTypeIsGeneric));
}

TEST_F(AdblockConverterTest,
       UrlFilterBlocking_NotAffectedByRegexLookaroundFilter) {
  auto subscription = ConvertAndLoadRules(R"(
    ||testpages.adblockplus.org/testfiles/image/static/$image
    /^https?://(?![^\s]+\.streamvid\.club)/$third-party,match-case,xmlhttprequest,domain=kindkino.ru
  )");

  EXPECT_TRUE(VerifyRequestBlocked(
      subscription,
      GURL(
          "https://testpages.adblockplus.org/testfiles/image/static/image.png"),
      "domain.com", ContentType::Image, ContentTypeIsSpecific));
}

TEST_F(AdblockConverterTest,
       UrlFilterBlocking_NotAffectedByRegexLookaroundFilter_2) {
  auto subscription = ConvertAndLoadRules(R"(
    /^https?:\/\/.*\.(onlineee|online|)\/.*/$domain=hclips.com
    /^https?://(?![^\s]+\.streamvid\.club)/$third-party,match-case,xmlhttprequest,domain=kindkino.ru
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://moneypunchstep.online/saber/ball/nomad/"), "hclips.com",
      ContentType::Subdocument, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest,
       UrlFilterBlocking_NotAffectedByInvalidRegexFilter) {
  auto subscription = ConvertAndLoadRules(R"(
    ||testpages.adblockplus.org/testfiles/image/static/$image
    /[/
  )");

  EXPECT_TRUE(VerifyRequestBlocked(
      subscription,
      GURL(
          "https://testpages.adblockplus.org/testfiles/image/static/image.png"),
      "domain.com", ContentType::Image, ContentTypeIsSpecific));
}

TEST_F(AdblockConverterTest,
       UrlFilterBlocking_NotAffectedByInvalidRegexFilter_3) {
  auto subscription = ConvertAndLoadRules(R"(
    /^https?:\/\/.*\.(onlineee|online|)\/.*/$domain=hclips.com
    /[/
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://moneypunchstep.online/saber/ball/nomad/"), "hclips.com",
      ContentType::Subdocument, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest,
       UrlFilterBlocking_LookaroundRegexEnginesMaxUrlSizeStressTest) {
  auto subscription = ConvertAndLoadRules(R"(
    /^https?://(?![^\s]+\.streamvid\.club|api\.kinogram\.best\/embed\/|cdn\.jsdelivr\.net\/npm\/venom-player)/$third-party,xmlhttprequest,domain=kindkino.ru
    )");

  std::string url = "https://something.streamvid.club/";
  url.append(RandomAsciiString(url::kMaxURLChars - url.size()));
  const GURL big_url(url);

  EXPECT_EQ(big_url.spec().size(), url::kMaxURLChars);
  EXPECT_FALSE(VerifyRequestBlocked(subscription, big_url, "kindkino.ru",
                                    ContentType::Xmlhttprequest,
                                    ContentTypeIsGeneric));

  url.append(RandomAsciiString(url::kMaxURLChars));
  const GURL bigger_url(url);

  EXPECT_EQ(bigger_url.spec().size(), url::kMaxURLChars * 2);
  EXPECT_FALSE(VerifyRequestBlocked(subscription, bigger_url, "kindkino.ru",
                                    ContentType::Xmlhttprequest,
                                    ContentTypeIsGeneric));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_AcceptsNonNormalizedUrl) {
  auto subscription = ConvertAndLoadRules(R"(
    ||testpages.adblockplus.org/ad
  )");

  EXPECT_TRUE(VerifyRequestBlocked(
      subscription, GURL("https:testpages.adblockplus.org/ad:80"), "domain.com",
      ContentType::Stylesheet, ContentTypeIsSpecific));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_Script) {
  auto subscription = ConvertAndLoadRules(R"(
    testpages.adblockplus.org/testfiles/script/$script
  )");

  EXPECT_TRUE(VerifyRequestBlocked(
      subscription,
      GURL("https://testpages.adblockplus.org/testfiles/script/script.js"),
      "domain.com", ContentType::Script, ContentTypeIsSpecific));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_Subdocument) {
  auto subscription = ConvertAndLoadRules(R"(
    testpages.adblockplus.org/testfiles/subdocument/$subdocument
  )");
  EXPECT_TRUE(VerifyRequestBlocked(
      subscription,
      GURL(
          "https://testpages.adblockplus.org/testfiles/subdocument/index.html"),
      "domain.com", ContentType::Subdocument, ContentTypeIsSpecific));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_WebRTC) {
  auto subscription = ConvertAndLoadRules(R"(
    $webrtc,domain=testpages.adblockplus.org
  )");
  EXPECT_TRUE(VerifyRequestBlocked(
      subscription, GURL("https://testpages.adblockplus.org/webrtc"),
      "testpages.adblockplus.org", ContentType::Webrtc, ContentTypeIsSpecific));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_Wildcard) {
  auto subscription = ConvertAndLoadRules(R"(
    /testfiles/blocking/wildcard/*/wildcard.png
  )");
  EXPECT_TRUE(VerifyRequestBlocked(
      subscription,
      GURL("https://domain.com/testfiles/blocking/wildcard/path/component/"
           "wildcard.png"),
      "domain.com", ContentType::Image, ContentTypeIsGeneric));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_FullPath) {
  auto subscription = ConvertAndLoadRules(R"(
    ||testpages.adblockplus.org/testfiles/blocking/full-path.png
  )");
  EXPECT_TRUE(VerifyRequestBlocked(
      subscription,
      GURL(
          "https://testpages.adblockplus.org/testfiles/blocking/full-path.png"),
      "domain.com", ContentType::Image, ContentTypeIsGeneric));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_PartialPath) {
  auto subscription = ConvertAndLoadRules(R"(
    /testfiles/blocking/partial-path/
  )");
  EXPECT_TRUE(VerifyRequestBlocked(
      subscription,
      GURL("https://whatever.com/testfiles/blocking/partial-path/content.png"),
      "domain.com", ContentType::Image, ContentTypeIsGeneric));
}

TEST_F(AdblockConverterTest, UrlFilter_EndWithCarret) {
  auto subscription = ConvertAndLoadRules(R"(
    ads.example.com^
  )");
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("http://ads.example.com.ua"), "https://testpages.adblockplus.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_DomainWildcardMiddle1) {
  auto subscription = ConvertAndLoadRules(R"(
    ||a.*.b.com
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://a.c.b.com"), "https://testpages.adblockplus.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_DomainWildcardMiddle2) {
  auto subscription = ConvertAndLoadRules(R"(
    ||a*.b.com
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://a.c.b.com"), "https://testpages.adblockplus.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_DomainWildcardMiddle3) {
  auto subscription = ConvertAndLoadRules(R"(
    ||a.*b.com
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://a.b.com"), "https://testpages.adblockplus.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_WildcardDomainAndPath1) {
  auto subscription = ConvertAndLoadRules(R"(
    ||d*in.com/*/blocking
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://domain.com/testfiles/blocking/wildcard.png"), "domain.com",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_WildcardDomainAndPath2) {
  auto subscription = ConvertAndLoadRules(R"(
    ||a*.b.com/*.png
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://a.b.com/a.png"), "https://testpages.adblockplus.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://ac.b.com/image.png"), "https://testpages.adblockplus.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_DomainWildcardStart) {
  auto subscription = ConvertAndLoadRules(R"(
    ||*a.b.com
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://a.b.com"), "https://testpages.adblockplus.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://domena.b.com/path"), "https://testpages.adblockplus.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_DomainWildcardStartEndCarret) {
  auto subscription = ConvertAndLoadRules(R"(
    ||*a.b.com^
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://a.b.com"), "https://testpages.adblockplus.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://domena.b.com/path"), "https://testpages.adblockplus.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_DomainCarretWildcardEnd) {
  auto subscription = ConvertAndLoadRules(R"(
    ||example.com^*/path.js
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://connect.example.com/en_US/path.js"),
      "https://testpages.adblockplus.org", ContentType::Script, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_CarretEnd) {
  auto subscription = ConvertAndLoadRules(R"(
    ||example.com^
    @@||example.com/ddm^
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://ad.example.com/ddm/ad/infytghiuf/nmys/;ord=1596077603231?"),
      "https://testpages.adblockplus.org", ContentType::Script, SiteKey(),
      FilterCategory::Blocking));
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://ad.example.com/ddm/ad/infytghiuf/nmys/;ord=1596077603231?"),
      "https://testpages.adblockplus.org", ContentType::Script, SiteKey(),
      FilterCategory::Allowing));
}

TEST_F(AdblockConverterTest, UrlFilter_DomainInParamsNoMatch) {
  auto subscription = ConvertAndLoadRules(R"(
    ||domain.com^
  )");
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://example.com/path?domain=https://www.domain.com"),
      "https://testpages.adblockplus.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_SchemeDomainDot) {
  auto subscription = ConvertAndLoadRules(R"(
    ://ads.
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(GURL("https://ads.example.com/u?dp=1"),
                                         "https://testpages.adblockplus.org",
                                         ContentType::Script, SiteKey(),
                                         FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_PathWildcards) {
  auto subscription = ConvertAndLoadRules(R"(
    ||example.com/a/*/c/script.*.js
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://example.com/a/b/c/script.file.js"),
      "https://testpages.adblockplus.org", ContentType::Script, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_MultipleCarretAndWildcard) {
  auto subscription = ConvertAndLoadRules(R"(
    @@^path1/path2/*/path4/file*.js^
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://example.com/path1/path2/path3/path4/file1.2.3.js?v=1"),
      "https://testpages.adblockplus.org", ContentType::Script, SiteKey(),
      FilterCategory::Allowing));
}

TEST_F(AdblockConverterTest, UrlFilter_PartdomainNoMatch) {
  auto subscription = ConvertAndLoadRules(R"(
    ||art-domain.com^
  )");
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://sub.part-domain.com/path"),
      "https://testpages.adblockplus.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_DoubleSlash) {
  auto subscription = ConvertAndLoadRules(R"(
    ||example.com*/script.
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(GURL("https://example.com//script.js"),
                                         "https://testpages.adblockplus.org",
                                         ContentType::Image, SiteKey(),
                                         FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_Regex) {
  auto subscription = ConvertAndLoadRules(R"(
   @@/^https:\/\/www\.domain(?:\.[a-z]{2,3}){1,2}\/afs\//
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://www.domain.com/afs/iframe.html"),
      "https://testpages.adblockplus.org", ContentType::Script, SiteKey(),
      FilterCategory::Allowing));
}

TEST_F(AdblockConverterTest, UrlFilter_DomainMatchFilterWithoutDomain1) {
  auto subscription = ConvertAndLoadRules(R"(
   @@||*file_name.gif
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://example.com/path/file_name.gif"),
      "https://testpages.adblockplus.org", ContentType::Script, SiteKey(),
      FilterCategory::Allowing));
}

TEST_F(AdblockConverterTest, UrlFilter_DomainMatchFilterWithoutDomain2) {
  auto subscription = ConvertAndLoadRules(R"(
   @@||*/file_name.gif
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://example.com/path/file_name.gif"),
      "https://testpages.adblockplus.org", ContentType::Script, SiteKey(),
      FilterCategory::Allowing));
}

TEST_F(AdblockConverterTest, UrlFilter_DomainStart) {
  auto subscription = ConvertAndLoadRules(R"(
    ||example.co
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://example.com"), "https://testpages.adblockplus.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_StartMatchCompleteUrl) {
  auto subscription = ConvertAndLoadRules(R"(
    |https://domain.com/path/file.gif
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://domain.com/path/file.gif"),
      "https://testpages.adblockplus.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_DomainMatchDotWildcard) {
  auto subscription = ConvertAndLoadRules(R"(
    ||*.
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://example.com/path/file.gif"),
      "https://testpages.adblockplus.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_DomainWithPort) {
  auto subscription = ConvertAndLoadRules(R"(
    ||example.com:8888/js
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://wwww.example.com:8888/js"),
      "https://testpages.adblockplus.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_IPWithPortAndWildcard) {
  auto subscription = ConvertAndLoadRules(R"(
    ||1.2.3.4:8060/*/
  )");
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://1.2.3.4:8060/path"), "https://testpages.adblockplus.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://1.2.3.4:8060/path/file.js"),
      "https://testpages.adblockplus.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_DomainWithPortAndCarret) {
  auto subscription = ConvertAndLoadRules(R"(
    ||example.com:8862^
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://example.com:8862"), "https://testpages.adblockplus.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
  EXPECT_TRUE(subscription->HasUrlFilter(GURL("https://example.com:8862/path"),
                                         "https://testpages.adblockplus.org",
                                         ContentType::Image, SiteKey(),
                                         FilterCategory::Blocking));
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://example.com:886"), "https://testpages.adblockplus.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_SinglePipeCarret) {
  auto subscription = ConvertAndLoadRules(R"(
    |http://example.com^
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("http://example.com:8000/"), "https://testpages.adblockplus.org",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlAllowListDocument) {
  auto subscription = ConvertAndLoadRules(R"(
    @@^upapi=true$document,domain=thedirect.com|domain2.com
  )");
  EXPECT_FALSE(subscription->HasSpecialFilter(
      SpecialFilterType::Document,
      GURL("https://backend.upapi.net/pv?upapi=true"), "domain.com",
      SiteKey()));

  EXPECT_TRUE(subscription->HasSpecialFilter(
      SpecialFilterType::Document,
      GURL("https://backend.upapi.net/pv?upapi=true"), "thedirect.com",
      SiteKey()));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_DomainSpecific) {
  auto subscription = ConvertAndLoadRules(R"(
    /testfiles/domain/dynamic/*$domain=testpages.adblockplus.org
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://whatever.com/testfiles/domain/dynamic/content.png"),
      "testpages.adblockplus.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
  // Does not match the same url embedded in a different domain - the rule is
  // domain-specific.
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://whatever.com/testfiles/domain/dynamic/content.png"),
      "different.adblockplus.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_DomainSpecificIgnoresWWW) {
  auto subscription = ConvertAndLoadRules(R"(
    /testfiles/domain/dynamic/*$domain=testpages.adblockplus.org
  )");

  // The filter's domain is testpages.adblockplus.org and the request's domain
  // is www.testpages.adblockplus.org. The "www" prefix is ignored, the domains
  // are considered to match.
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://whatever.com/testfiles/domain/dynamic/content.png"),
      "www.testpages.adblockplus.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest,
       UrlFilterBlocking_DomainSpecificIsCaseInsensitive) {
  auto subscription = ConvertAndLoadRules(R"(
    /testfiles/domain/dynamic/*$domain=testpages.adblockplus.org
  )");

  // The filter's domain is testpages.adblockplus.org and the request's domain
  // is TESTPAGES.adblockplus.org. The domains are considered to match.
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://whatever.com/testfiles/domain/dynamic/content.png"),
      "TESTPAGES.adblockplus.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_Other) {
  auto subscription = ConvertAndLoadRules(R"(
    $other,domain=testpages.adblockplus.org
  )");
  EXPECT_TRUE(VerifyRequestBlocked(
      subscription, GURL("https://whatever.com/script.js"),
      "testpages.adblockplus.org", ContentType::Other, ContentTypeIsSpecific));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_Ping) {
  auto subscription = ConvertAndLoadRules(R"(
    testpages.adblockplus.org/*^$ping
  )");
  EXPECT_TRUE(VerifyRequestBlocked(
      subscription, GURL("https://testpages.adblockplus.org/ping"),
      "testpages.adblockplus.org", ContentType::Ping, ContentTypeIsSpecific));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_XmlHttpRequest) {
  auto subscription = ConvertAndLoadRules(R"(
    testpages.adblockplus.org/testfiles/xmlhttprequest/$xmlhttprequest
  )");
  EXPECT_TRUE(VerifyRequestBlocked(
      subscription,
      GURL("https://testpages.adblockplus.org/testfiles/xmlhttprequest/"
           "request.xml"),
      "testpages.adblockplus.org", ContentType::Xmlhttprequest,
      ContentTypeIsSpecific));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_ThirdParty) {
  auto subscription = ConvertAndLoadRules(R"(
    adblockplus-icon-colour-web.svg$third-party
  )");

  // $third-party means the rule applies if the domain is different than the
  // domain of the URL (actually, a bit more complicated than that)
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://testpages.adblockplus.org/adblockplus-icon-colour-web.svg"),
      "google.com", ContentType::Image, SiteKey(), FilterCategory::Blocking));

  // Does not apply on same domain.
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://testpages.adblockplus.org/adblockplus-icon-colour-web.svg"),
      "testpages.adblockplus.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_ThirdParty_2) {
  auto subscription = ConvertAndLoadRules(R"(
    ||adgrx.com^$third-party
    @@||fls.doubleclick.net^$subdocument,image
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://rtb.adgrx.com/segments/"
           "EEXrM7_yY0aoq_OtZxYHORtz55uneYk8VAiLioGMm14=/47510.gif"),
      "8879538.fls.doubleclick.net", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));

  EXPECT_FALSE(subscription->HasSpecialFilter(
      SpecialFilterType::Document,
      GURL("https://rtb.adgrx.com/segments/"
           "EEXrM7_yY0aoq_OtZxYHORtz55uneYk8VAiLioGMm14=/47510.gif"),
      "8879538.fls.doubleclick.net", SiteKey()));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_RegexEnd) {
  auto subscription = ConvertAndLoadRules(R"(
    ||popin.cc/popin_discovery
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://api.popin.cc/popin_discovery5-min.js"), "domain.com",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_NotThirdParty) {
  auto subscription = ConvertAndLoadRules(R"(
    abb-logo.png$~third-party
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://testpages.adblockplus.org/abb-logo.png"),
      "testpages.adblockplus.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
  // Does not apply on different domain.
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://testpages.adblockplus.org/abb-logo.png"), "domain.com",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_Popup) {
  auto subscription = ConvertAndLoadRules(R"(
    ||testpages.adblockplus.org/testfiles/popup/link.html^$popup
  )");

  EXPECT_TRUE(subscription->HasPopupFilter(
      GURL("https://testpages.adblockplus.org/testfiles/popup/link.html"),
      GURL("https://testpages.adblockplus.org"), SiteKey(),
      FilterCategory::Blocking));

  // No allowing filter:
  EXPECT_FALSE(subscription->HasPopupFilter(
      GURL("https://testpages.adblockplus.org/testfiles/popup/link.html"),
      GURL("https://testpages.adblockplus.org"), SiteKey(),
      FilterCategory::Allowing));

  // Does not match if the content type is different.
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://testpages.adblockplus.org/testfiles/popup/link.html"),
      "testpages.adblockplus.org", ContentType::Subdocument, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_MultipleTypes) {
  auto subscription = ConvertAndLoadRules(R"(
    ||testpages.adblockplus.org/testfiles/popup/link.html^$popup,image,script
  )");

  EXPECT_TRUE(subscription->HasPopupFilter(
      GURL("https://testpages.adblockplus.org/testfiles/popup/link.html"),
      GURL("https://testpages.adblockplus.org"), SiteKey(),
      FilterCategory::Blocking));

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://testpages.adblockplus.org/testfiles/popup/link.html"),
      "testpages.adblockplus.org", ContentType::Script, SiteKey(),
      FilterCategory::Blocking));

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://testpages.adblockplus.org/testfiles/popup/link.html"),
      "testpages.adblockplus.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_UppercaseDefinition) {
  // A filter with uppercase component should be matched by a lowercase URL,
  // this requires keywords to be converted to lowercase during filter parsing.
  auto subscription = ConvertAndLoadRules(R"(
    ||bttrack.com/Pixel/$image,third-party
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://bttrack.com/pixel/"
           "cookiesync?source=14b8c562-d12b-418b-b680-ad517d5839ec"),
      "super.abril.com.br", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilterBlocking_UppercaseURL) {
  // A filter with lowercase component should be matched by an uppercase URL,
  // this requires keywords to be converted to lowercase during URL matching.
  auto subscription = ConvertAndLoadRules(R"(
    ||bttrack.com/pixel/$image,third-party
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://bttrack.com/Pixel/"
           "cookiesync?source=14b8c562-d12b-418b-b680-ad517d5839ec"),
      "super.abril.com.br", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_CaseSensitiveMatch) {
  auto subscription = ConvertAndLoadRules(R"(
    /testfiles/match-case/static/*/abc.png$match-case
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://whatever.com/testfiles/match-case/static/path/abc.png"),
      "testpages.adblockplus.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
  // Does not match if the case is different
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://whatever.com/testfiles/match-case/static/path/ABC.png"),
      "testpages.adblockplus.org", ContentType::Image, SiteKey(),
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilter_AllowlistThirdPartyFilter) {
  auto subscription = ConvertAndLoadRules(R"(
    ||amazon-adsystem.com^$third-party
    @@||amazon-adsystem.com//ecm
    @@||amazon-adsystem.com/ecm
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://s.amazon-adsystem.com//"
           "ecm3?ex=visualiq&id=a78f7b3f-a525-45eb-a1ed-5eb5eab339af"),
      "www.wwe.com", ContentType::Image, SiteKey(), FilterCategory::Allowing));
}

TEST_F(AdblockConverterTest, UrlFilter_IgnoresInvalidFilterOption) {
  auto subscription = ConvertAndLoadRules(R"(
    @@||google.com/recaptcha/$csp,subdocument
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL(" https://www.google.com/recaptcha/api2/a"), "vidoza.net",
      ContentType::Subdocument, SiteKey(), FilterCategory::Allowing));
}

TEST_F(AdblockConverterTest, UrlFilterException_Sitekey) {
  auto subscription = ConvertAndLoadRules(R"(
    @@$document,sitekey=MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBANGtTstne7e8MbmDHDiMFkGbcuBgXmiVesGOG3gtYeM1EkrzVhBjGUvKXYE4GLFwqty3v5MuWWbvItUWBTYoVVsCAwEAAQ
  )");

  EXPECT_TRUE(subscription->HasSpecialFilter(
      SpecialFilterType::Document, GURL("https://whatever.com"), "whatever.com",
      SiteKey("MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBANGtTstne7e8MbmDHDiMFkGbcuBgXmiV"
              "esGOG3gt"
              "YeM1EkrzVhBjGUvKXYE4GLFwqty3v5MuWWbvItUWBTYoVVsCAwEAAQ")));
  // No allow rule without sitekey
  EXPECT_FALSE(subscription->HasSpecialFilter(SpecialFilterType::Document,
                                              GURL("https://whatever.com"),
                                              "whatever.com", SiteKey()));
}

TEST_F(AdblockConverterTest, UrlFilterException_Popup) {
  auto subscription = ConvertAndLoadRules(R"(
    ! exceptions/popup
    ||testpages.adblockplus.org/testfiles/popup_exception/link.html^$popup
    @@||testpages.adblockplus.org/testfiles/popup_exception/link.html^$popup
  )");

  // Finds the blocking filter:
  EXPECT_TRUE(subscription->HasPopupFilter(
      GURL("https://testpages.adblockplus.org/testfiles/popup_exception/"
           "link.html"),
      GURL("https://testpages.adblockplus.org"), SiteKey(),
      FilterCategory::Blocking));

  // But also finds the allowing filter:
  EXPECT_TRUE(subscription->HasPopupFilter(
      GURL("https://testpages.adblockplus.org/testfiles/popup_exception/"
           "link.html"),
      GURL("https://testpages.adblockplus.org"), SiteKey(),
      FilterCategory::Allowing));
}

TEST_F(AdblockConverterTest, UrlFilterException_XmlHttpRequest) {
  auto subscription = ConvertAndLoadRules(R"(
    ! exceptions/xmlhttprequest
    ||testpages.adblockplus.org/testfiles/xmlhttprequest_exception/*
    @@testpages.adblockplus.org/testfiles/xmlhttprequest_exception/$xmlhttprequest
  )");

  EXPECT_TRUE(VerifyRequestAllowed(
      subscription,
      GURL("https://testpages.adblockplus.org/testfiles/"
           "xmlhttprequest_exception/link.html"),
      "https://testpages.adblockplus.org", ContentType::Xmlhttprequest));
}

TEST_F(AdblockConverterTest, UrlFilterException_Ping) {
  auto subscription = ConvertAndLoadRules(R"(
    ! exceptions/ping
    testpages.adblockplus.org/*^$ping
    @@testpages.adblockplus.org/en/exceptions/ping*^$ping
  )");
  EXPECT_TRUE(VerifyRequestAllowed(
      subscription,
      GURL("https://testpages.adblockplus.org/en/exceptions/ping/link.html"),
      "https://testpages.adblockplus.org", ContentType::Ping));
}

TEST_F(AdblockConverterTest, UrlFilterException_Subdocument) {
  auto subscription = ConvertAndLoadRules(R"(
    ! exceptions/subdocument
    ||testpages.adblockplus.org/testfiles/subdocument_exception/*
    @@testpages.adblockplus.org/testfiles/subdocument_exception/$subdocument
  )");
  EXPECT_TRUE(VerifyRequestAllowed(
      subscription,
      GURL("https://testpages.adblockplus.org/testfiles/subdocument_exception/"
           "link.html"),
      "testpages.adblockplus.org", ContentType::Subdocument));
}

TEST_F(AdblockConverterTest, UrlFilterException_Script) {
  auto subscription = ConvertAndLoadRules(R"(
    ! exceptions/script
    ||testpages.adblockplus.org/testfiles/script_exception/*
    @@testpages.adblockplus.org/testfiles/script_exception/$script
  )");
  EXPECT_TRUE(VerifyRequestAllowed(
      subscription,
      GURL("https://testpages.adblockplus.org/testfiles/script_exception/"
           "link.html"),
      "testpages.adblockplus.org", ContentType::Script));
}

TEST_F(AdblockConverterTest, UrlFilterException_Image) {
  auto subscription = ConvertAndLoadRules(R"(
    ! exceptions/image
    ||testpages.adblockplus.org/testfiles/image_exception/*
    @@testpages.adblockplus.org/testfiles/image_exception/$image
  )");
  EXPECT_TRUE(VerifyRequestAllowed(
      subscription,
      GURL("https://testpages.adblockplus.org/testfiles/image_exception/"
           "image.jpg"),
      "testpages.adblockplus.org", ContentType::Image));
}

TEST_F(AdblockConverterTest, UrlFilterException_Stylesheet) {
  auto subscription = ConvertAndLoadRules(R"(
    ! exceptions/stylesheet
    ||testpages.adblockplus.org/testfiles/stylesheet_exception/*
    @@testpages.adblockplus.org/testfiles/stylesheet_exception/$stylesheet
  )");
  EXPECT_TRUE(VerifyRequestAllowed(subscription,
                                   GURL("https://testpages.adblockplus.org/"
                                        "testfiles/stylesheet_exception/"
                                        "style.css"),
                                   "testpages.adblockplus.org",
                                   ContentType::Stylesheet));
}

TEST_F(AdblockConverterTest, UrlFilterException_WebSocket) {
  auto subscription = ConvertAndLoadRules(R"(
    ! exceptions/websocket
    $websocket,domain=testpages.adblockplus.org
    @@$websocket,domain=testpages.adblockplus.org
  )");

  EXPECT_TRUE(VerifyRequestAllowed(
      subscription, GURL("https://whatever.org/socket.wss"),
      "testpages.adblockplus.org", ContentType::Websocket));
}

TEST_F(AdblockConverterTest, UrlRegexAnythingEndingOnline) {
  auto subscription = ConvertAndLoadRules(R"(
    /^https?:\/\/.*\.(onlineee|online|)\/.*/$domain=hclips.com
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://moneypunchstep.online/saber/ball/nomad/"), "hclips.com",
      ContentType::Subdocument, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilterRegexContains$WithFilterOptions) {
  auto subscription = ConvertAndLoadRules(R"(
    /^https?:\/\/cdn\.[0-9a-z]{3,6}\.xyz\/[a-z0-9]{8,}\.js$/$script,third-party
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://cdn.what.xyz/adsertscript.js"), "domain.com",
      ContentType::Script, SiteKey(), FilterCategory::Blocking));

  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://cdn.what.xyz/adsertscript.js"), "domain.com",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, UrlFilterRegexContains$WithoutFilterOption) {
  auto subscription = ConvertAndLoadRules(R"(
    /^https?:\/\/cdn\.[0-9a-z]{3,6}\.xyz\/[a-z0-9]{8,}\.js$/
  )");

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://cdn.what.xyz/adsertscript.js"), "domain.com",
      ContentType::Script, SiteKey(), FilterCategory::Blocking));

  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://cdn.what.xyz/adsertscript.js"), "domain.com",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
}

// TODO generic block filters may soon be removed from the ContentType enum and
// handled internally.
TEST_F(AdblockConverterTest, UrlFilterException_GenericBlock) {
  auto subscription = ConvertAndLoadRules(R"(
    ! exceptions/genericblock
    /testfiles/genericblock/generic.png
    /testfiles/genericblock/specific.png$domain=testpages.adblockplus.org
    @@||testpages.adblockplus.org/en/exceptions/genericblock$genericblock
  )");

  EXPECT_TRUE(subscription->HasSpecialFilter(
      SpecialFilterType::Genericblock,
      GURL("https://testpages.adblockplus.org/en/exceptions/genericblock"),
      "testpages.adblockplus.org", SiteKey()));
  // Since there is a genericblock rule for this parent, we would search for
  // specific-only rules
  // The rule /testfiles/genericblock/generic.png does not apply as it is not
  // domain-specific:
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://testpages.adblockplus.org/testfiles/genericblock/"
           "generic.png"),
      "testpages.adblockplus.org", ContentType::Image, SiteKey(),
      FilterCategory::DomainSpecificBlocking));
  // The rule
  // /testfiles/genericblock/specific.png$domain=testpages.adblockplus.org
  // applies:
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://testpages.adblockplus.org/testfiles/genericblock/"
           "specific.png"),
      "testpages.adblockplus.org", ContentType::Image, SiteKey(),
      FilterCategory::DomainSpecificBlocking));
}

TEST_F(AdblockConverterTest, UrlFilterException_DomainSpecificExclusion) {
  auto subscription = ConvertAndLoadRules(R"(
||media.net^$third-party
@@||media.net^$document
@@||media.net^$third-party,domain=~fandom.com
@@||contextual.media.net/bidexchange.js
  )");

  // This reflects a bug squished IRL:
  // the URL is blocked by ||media.net^$third-party and should NOT be allowed
  // by any of the allow rules because:
  // 1. It's not a DOCUMENT type
  // 2. The third-party allow rule does not apply on roblox.fandom.com
  // 3. bidexchange.js is a path allowed only for a different page
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://hbx.media.net/bidexchange.js?cid=8CUDYP2MO&version=6.1"),
      "roblox.fandom.com", ContentType::Script, SiteKey(),
      FilterCategory::Blocking));
  EXPECT_FALSE(subscription->HasUrlFilter(
      GURL("https://hbx.media.net/bidexchange.js?cid=8CUDYP2MO&version=6.1"),
      "roblox.fandom.com", ContentType::Script, SiteKey(),
      FilterCategory::Allowing));
}
TEST_F(AdblockConverterTest, Elementhide_generic_selector) {
  auto subscriptions = ConvertAndLoadRules("##.zad.billboard");
  auto selectors = subscriptions->GetElemhideSelectors(
      GURL("https://pl.ign.com/marvels-avengers/41262/news/"
           "marvels-avengers-kratos-zagra-czarna-pantere"),
      false);
  EXPECT_EQ(FilterSelectors(selectors),
            std::set<base::StringPiece>({".zad.billboard"}));
}

TEST_F(AdblockConverterTest, Elementhide_excludes_sub) {
  auto subscriptions = ConvertAndLoadRules(R"(
        example.org###ad_1
        example.org###ad_2
        foo.example.org#@##ad_2
    )");
  const auto selectors_1 = subscriptions->GetElemhideSelectors(
      GURL("http://foo.example.org"), false);

  const auto selectors_2 =
      subscriptions->GetElemhideSelectors(GURL("http://example.org"), false);

  EXPECT_EQ(FilterSelectors(selectors_1),
            std::set<base::StringPiece>({"#ad_1"}));
  EXPECT_EQ(FilterSelectors(selectors_2),
            std::set<base::StringPiece>({"#ad_1", "#ad_2"}));
}

TEST_F(AdblockConverterTest, Elementhide_domain_specifc) {
  auto subscriptions = ConvertAndLoadRules(R"(
    ! other type of filters
    /testcasefiles/blocking/addresspart/abptestcasepath/
    example.org#?#div:-abp-properties(width: 213px)
    ! element hiding selectors
    ###testcase-eh-id
    example.org###testcase-eh-id
    example.org##.testcase-eh-class
    example.org##.testcase-container > .testcase-eh-descendant
    ~foo.example.org,example.org##foo
    ~othersiteneg.org##testneg
    ! other site
    )");
  auto selectors =
      subscriptions->GetElemhideSelectors(GURL("http://example.org"), false);
  EXPECT_EQ(std::set<base::StringPiece>(
                {"#testcase-eh-id", "testneg",
                 ".testcase-container > .testcase-eh-descendant",
                 ".testcase-eh-class", "foo"}),
            FilterSelectors(selectors));
}

TEST_F(AdblockConverterTest, Elementhide_same_result) {
  auto subscriptions = ConvertAndLoadRules(R"(
    example1.org###testcase-eh-id
    example2.org###testcase-eh-id
    )");
  auto selectors_1 =
      subscriptions->GetElemhideSelectors(GURL("http://example1.org"), false);

  auto selectors_2 =
      subscriptions->GetElemhideSelectors(GURL("http://example2.org"), false);

  auto selectors_3 = subscriptions->GetElemhideSelectors(
      GURL("http://non-existing-domain.com"), false);

  EXPECT_EQ(FilterSelectors(selectors_1), FilterSelectors(selectors_2));
  EXPECT_EQ(FilterSelectors(selectors_1),
            std::set<base::StringPiece>({"#testcase-eh-id"}));
  EXPECT_EQ(FilterSelectors(selectors_3).size(), 0u);
}

TEST_F(AdblockConverterTest, Elementhide_exception_main_domain) {
  auto subscriptions = ConvertAndLoadRules(R"(
    sub.example.org###testcase-eh-id
    example.org#@##testcase-eh-id
    )");
  auto selectors = subscriptions->GetElemhideSelectors(
      GURL("http://sub.example.org"), false);

  EXPECT_EQ(FilterSelectors(selectors).size(), 0u);
}

TEST_F(AdblockConverterTest, Elementhide_apply_just_domain) {
  auto subscriptions = ConvertAndLoadRules("example.org###div");

  auto selectors =
      subscriptions->GetElemhideSelectors(GURL("http://example.org"), true);

  EXPECT_EQ(FilterSelectors(selectors), std::set<base::StringPiece>({"#div"}));
  auto selectors2 =
      subscriptions->GetElemhideSelectors(GURL("http://example2.org"), false);

  EXPECT_EQ(FilterSelectors(selectors2).size(), 0u);
}

TEST_F(AdblockConverterTest, Elementhideemu_generic) {
  auto subscriptions = ConvertAndLoadRules("example.org#?#foo");
  const auto selectors =
      subscriptions->GetElemhideEmulationSelectors(GURL("http://example.org"));

  EXPECT_EQ(FilterSelectors(selectors), std::set<base::StringPiece>({"foo"}));
}

TEST_F(AdblockConverterTest, Elementhideemu_allow_list) {
  auto subscriptions = ConvertAndLoadRules(R"(
    example.org#?#foo
    example.org#@#foo
    )");
  const auto selectors =
      subscriptions->GetElemhideEmulationSelectors(GURL("http://example.org"));

  EXPECT_EQ(FilterSelectors(selectors).size(), 0u);
}

TEST_F(AdblockConverterTest, Elementhideemu_allow_list_2) {
  auto subscriptions = ConvertAndLoadRules(R"(
    example.org#?#foo
    example.org#?#another
    example.org#@#foo
    )");
  const auto selectors =
      subscriptions->GetElemhideEmulationSelectors(GURL("http://example.org"));

  EXPECT_EQ(FilterSelectors(selectors),
            std::set<base::StringPiece>({"another"}));
}

TEST_F(AdblockConverterTest, Elementhideemu_allow_list_3) {
  auto subscriptions = ConvertAndLoadRules(R"(
    example.org#?#foo
    example.org#?#another
    example2.org#?#foo
    example.org#@#foo
    )");
  const auto selectors =
      subscriptions->GetElemhideEmulationSelectors(GURL("http://example2.org"));

  EXPECT_EQ(FilterSelectors(selectors), std::set<base::StringPiece>({"foo"}));
}

TEST_F(AdblockConverterTest, Elementhideemu_domain_n_subdomain) {
  auto subscriptions = ConvertAndLoadRules(R"(
      !other type of filters
      /testcasefiles/blocking/addresspart/abptestcasepath/
      example.org###testcase-eh-id
      !element hiding emulation selectors
      example.org#?#div:-abp-properties(width: 213px)
      example.org#?#div:-abp-has(>div>img.testcase-es-has)
      example.org#?#span:-abp-contains(ESContainsTarget)
      ~foo.example.org,example.org#?#div:-abp-properties(width: 213px)
      !allowlisted
      example.org#@#foo
      !other site
      othersite.com###testcase-eh-id
    )");
  const auto selectors_1 =
      subscriptions->GetElemhideEmulationSelectors(GURL("http://example.org"));

  // should be 3 unique selectors
  EXPECT_EQ(FilterSelectors(selectors_1),
            std::set<base::StringPiece>({
                "div:-abp-properties(width: 213px)",
                "div:-abp-has(>div>img.testcase-es-has)",
                "span:-abp-contains(ESContainsTarget)",
            }));

  const auto selectors_2 = subscriptions->GetElemhideEmulationSelectors(
      GURL("http://foo.example.org"));

  EXPECT_EQ(FilterSelectors(selectors_2),
            std::set<base::StringPiece>({
                "div:-abp-properties(width: 213px)",
                "div:-abp-has(>div>img.testcase-es-has)",
                "span:-abp-contains(ESContainsTarget)",
            }));
}

TEST_F(AdblockConverterTest, Elementhideemu_excludes_sub) {
  auto subscriptions = ConvertAndLoadRules(R"(
        example.org#?#general
        example.org#?#specific
        foo.example.org#@#specific
    )");
  const auto selectors_1 = subscriptions->GetElemhideEmulationSelectors(
      GURL("http://foo.example.org"));

  const auto selectors_2 =
      subscriptions->GetElemhideEmulationSelectors(GURL("http://example.org"));

  EXPECT_EQ(FilterSelectors(selectors_1),
            std::set<base::StringPiece>{"general"});

  EXPECT_EQ(FilterSelectors(selectors_2), std::set<base::StringPiece>({
                                              "general",
                                              "specific",
                                          }));
}
TEST_F(AdblockConverterTest, Elementhideemu_list_diff) {
  auto subscriptions = ConvertAndLoadRules(R"(
      example1.org#?#div:-abp-properties(width: 213px)
      example2.org#?#div:-abp-properties(width: 213px)
      example2.org#@#div:-abp-properties(width: 213px)
    )");
  const auto selectors_1 =
      subscriptions->GetElemhideEmulationSelectors(GURL("http://example1.org"));

  EXPECT_EQ(FilterSelectors(selectors_1),
            std::set<base::StringPiece>{"div:-abp-properties(width: 213px)"});

  const auto selectors_2 =
      subscriptions->GetElemhideEmulationSelectors(GURL("http://example2.org"));
  EXPECT_EQ(FilterSelectors(selectors_2).size(), 0u);
}

TEST_F(AdblockConverterTest, Elementhide_exception_with_excluded_url) {
  auto subscriptions = ConvertAndLoadRules(R"(
     ###_AD
     ~imore.com#@##_AD
    )");
  const auto selectors = subscriptions->GetElemhideSelectors(
      GURL("https://www.imore.com/"), false);

  EXPECT_EQ(FilterSelectors(selectors), std::set<base::StringPiece>{"#_AD"});
}

TEST_F(AdblockConverterTest, Elementhide_with_generic_excluded_url) {
  auto subscriptions = ConvertAndLoadRules(R"(
    ~imore.com###_AD
    )");
  const auto selectors = subscriptions->GetElemhideSelectors(
      GURL("https://www.domain.com/"), false);

  EXPECT_EQ(FilterSelectors(selectors), std::set<base::StringPiece>{"#_AD"});
}

TEST_F(AdblockConverterTest, Elementhide_top_tier_domain_match) {
  auto subscriptions = ConvertAndLoadRules(R"(
    com###_AD
    )");
  const auto selectors = subscriptions->GetElemhideSelectors(
      GURL("https://www.domain.com/"), true);

  EXPECT_EQ(FilterSelectors(selectors), std::set<base::StringPiece>{"#_AD"});
}

TEST_F(AdblockConverterTest, Elementhide_with_excluded_url_specific) {
  auto subscriptions = ConvertAndLoadRules(R"(
    ###_AD
     ~imore.com#@##_AD
    )");
  const auto selectors = subscriptions->GetElemhideSelectors(
      GURL("https://www.domain.com/"), true);

  EXPECT_EQ(FilterSelectors(selectors).size(), 0u);
}

TEST_F(AdblockConverterTest, Elementhide_with_excluded_url_case_insensitive) {
  auto subscriptions = ConvertAndLoadRules(R"(
    ###_AD
    ~IMore.com#@##_AD
    )");
  const auto selectors = subscriptions->GetElemhideSelectors(
      GURL("https://www.imore.com/"), false);

  EXPECT_EQ(FilterSelectors(selectors), std::set<base::StringPiece>{"#_AD"});
}

TEST_F(AdblockConverterTest, Elementhide_with_excluded_url_case_insensitive_2) {
  auto subscriptions = ConvertAndLoadRules(R"(
    ###_AD
    ~imore.com#@##_AD
    )");
  const auto selectors = subscriptions->GetElemhideSelectors(
      GURL("https://www.IMORE.com/"), false);

  EXPECT_EQ(FilterSelectors(selectors), std::set<base::StringPiece>{"#_AD"});
}

TEST_F(AdblockConverterTest, Elementhide_exception_subdomain) {
  auto subscriptions = ConvertAndLoadRules(R"(
    ##.ad_box
    domain.com,~www.domain.com#@#.ad_box
    )");
  const auto selectors =
      subscriptions->GetElemhideSelectors(GURL("https://domain.com/"), false);
  const auto selectors_2 = subscriptions->GetElemhideSelectors(
      GURL("https://www.domain.com/"), false);
  const auto selectors_3 =
      subscriptions->GetElemhideSelectors(GURL("https://domain2.com/"), false);

  EXPECT_EQ(FilterSelectors(selectors).size(), 0u);
  EXPECT_EQ(FilterSelectors(selectors_2),
            std::set<base::StringPiece>{".ad_box"});
  EXPECT_EQ(FilterSelectors(selectors_3),
            std::set<base::StringPiece>{".ad_box"});
}

TEST_F(AdblockConverterTest, Elementhideemu_exception_subdomain) {
  auto subscriptions = ConvertAndLoadRules(R"(
    domain.com#?#.ad_box
    mail.domain.com,~www.mail.domain.com#@#.ad_box
    )");
  const auto selectors = subscriptions->GetElemhideEmulationSelectors(
      GURL("https://mail.domain.com/"));
  const auto selectors_2 = subscriptions->GetElemhideEmulationSelectors(
      GURL("https://www.mail.domain.com/"));

  EXPECT_EQ(FilterSelectors(selectors).size(), 0u);

  EXPECT_EQ(FilterSelectors(selectors_2),
            std::set<base::StringPiece>{".ad_box"});
}

TEST_F(AdblockConverterTest, Elementhideemu_without_include_domains) {
  // The Elemhide emu filter has no include domains, only an exclude domain,
  // which makes it generic. Elemhide emu filters cannot be generic, so we
  // don't apply this filter.
  auto subscriptions = ConvertAndLoadRules(R"(
    ~domain.com#?#.ad_box
    )");
  const auto selectors =
      subscriptions->GetElemhideEmulationSelectors(GURL("https://test.com/"));

  EXPECT_EQ(FilterSelectors(selectors).size(), 0u);
}

TEST_F(AdblockConverterTest, Elementhideemu_on_top_level_domain) {
  // The Elemhide emu filter is defined to apply on all .com domains.
  // Elemhide emu filters cannot be applied so widely.
  auto subscriptions = ConvertAndLoadRules(R"(
    com#?#.ad_box
    .com#?#.ad_box
    )");
  const auto selectors =
      subscriptions->GetElemhideEmulationSelectors(GURL("https://test.com/"));

  EXPECT_EQ(FilterSelectors(selectors).size(), 0u);
}

TEST_F(AdblockConverterTest, SnippetEmpty) {
  auto index = ConvertAndLoadRulesToIndex(R"(
     test.com#$#
    )",
                                          {}, true);
  EXPECT_EQ(index->index_->snippet()->size(), 0u);
}

TEST_F(AdblockConverterTest, SnippetBasic) {
  ExpectSnippetTokenizerWill("mock_snippet", {{"log", "Test"}});
  auto index = ConvertAndLoadRulesToIndex(R"(
     test.com,~other.test.com#$#mock_snippet
    )",
                                          {}, true);
  ASSERT_EQ(index->index_->snippet()->size(), 1u);
  auto* entry = index->index_->snippet()->Get(0);
  ASSERT_EQ(entry->filter()->size(), 1u);
  EXPECT_EQ(entry->domain()->str(), "test.com");

  auto* snippet = entry->filter()->Get(0);
  ASSERT_EQ(snippet->include_domains()->size(), 1u);
  EXPECT_EQ(snippet->include_domains()->Get(0)->str(), "test.com");
  ASSERT_EQ(snippet->exclude_domains()->size(), 1u);
  EXPECT_EQ(snippet->exclude_domains()->Get(0)->str(), "other.test.com");
  EXPECT_EQ(snippet->script()->size(), 1u);

  auto* call = snippet->script()->Get(0);
  EXPECT_EQ(call->command()->str(), "log");
  ASSERT_EQ(call->arguments()->size(), 1u);
  EXPECT_EQ(call->arguments()->Get(0)->str(), "Test");
}

TEST_F(AdblockConverterTest, SnippetSpace) {
  ExpectSnippetTokenizerWill("mock_snippet", {{"log", "   Test\t arg"}});
  auto index = ConvertAndLoadRulesToIndex(R"(
     test.com#$#mock_snippet
    )",
                                          {}, true);
  ASSERT_EQ(index->index_->snippet()->size(), 1u);
  auto* entry = index->index_->snippet()->Get(0);
  ASSERT_EQ(entry->filter()->size(), 1u);
  auto* snippet = entry->filter()->Get(0);
  ASSERT_EQ(snippet->include_domains()->size(), 1u);

  auto* call = snippet->script()->Get(0);
  EXPECT_EQ(call->command()->str(), "log");
  ASSERT_EQ(call->arguments()->size(), 1u);
  EXPECT_EQ(call->arguments()->Get(0)->str(), "   Test\t arg");
}

TEST_F(AdblockConverterTest, SnippetArgumentPack) {
  ExpectSnippetTokenizerWill("mock_snippet", {{"log", "ab", "a b", "", "ccc"}});
  auto index = ConvertAndLoadRulesToIndex(R"(
     test.com#$#mock_snippet
    )",
                                          {}, true);
  ASSERT_EQ(index->index_->snippet()->size(), 1u);
  auto* entry = index->index_->snippet()->Get(0);
  ASSERT_EQ(entry->filter()->size(), 1u);
  auto* snippet = entry->filter()->Get(0);
  ASSERT_EQ(snippet->script()->size(), 1u);

  auto* call = snippet->script()->Get(0);
  EXPECT_EQ(call->command()->str(), "log");
  ASSERT_EQ(call->arguments()->size(), 4u);
  EXPECT_EQ(call->arguments()->Get(0)->str(), "ab");
  EXPECT_EQ(call->arguments()->Get(1)->str(), "a b");
  EXPECT_EQ(call->arguments()->Get(2)->str(), "");
  EXPECT_EQ(call->arguments()->Get(3)->str(), "ccc");
}

TEST_F(AdblockConverterTest, SnippetMultipleCallss) {
  ExpectSnippetTokenizerWill("mock_snippet",
                             {{"log", "test"}, {"log", "test"}, {"log"}});
  auto index = ConvertAndLoadRulesToIndex(R"(
     test.com#$#mock_snippet
    )",
                                          {}, true);
  ASSERT_EQ(index->index_->snippet()->size(), 1u);
  auto* entry = index->index_->snippet()->Get(0);
  ASSERT_EQ(entry->filter()->size(), 1u);
  auto* snippet = entry->filter()->Get(0);
  ASSERT_EQ(snippet->script()->size(), 3u);

  auto* call = snippet->script()->Get(0);
  EXPECT_EQ(call->command()->str(), "log");
  ASSERT_EQ(call->arguments()->size(), 1u);
  EXPECT_EQ(call->arguments()->Get(0)->str(), "test");

  call = snippet->script()->Get(1);
  EXPECT_EQ(call->command()->str(), "log");
  ASSERT_EQ(call->arguments()->size(), 1u);
  EXPECT_EQ(call->arguments()->Get(0)->str(), "test");

  call = snippet->script()->Get(2);
  EXPECT_EQ(call->command()->str(), "log");
  EXPECT_EQ(call->arguments()->size(), 0u);
}

TEST_F(AdblockConverterTest, NoSnippetTest) {
  auto sub = ConvertAndLoadRules(R"(
    test.com##selector
    )",
                                 {}, true);
  EXPECT_EQ(sub->MatchSnippets("random.org").size(), 0u);
}

TEST_F(AdblockConverterTest, NoSnippetForDomainTest) {
  ExpectSnippetTokenizerWill("mock_snippet",
                             {{"log", "test"}, {"log", "test"}, {"log"}});
  auto sub = ConvertAndLoadRules(R"(
     test.com#$#mock_snippet
    )",
                                 {}, true);
  EXPECT_EQ(sub->MatchSnippets("domain.com").size(), 0u);
  EXPECT_EQ(sub->MatchSnippets("test.com").size(), 1u);
}

TEST_F(AdblockConverterTest, SnippetFiltersTopLevelDomain) {
  auto sub = ConvertAndLoadRules(R"(
     test.com,gov.ua,com,localhost#$#abort-on-property-write 1 2 3
    )",
                                 {}, true);
  // has to have at least one subdomain
  // but it might be a Top Level domain like gov.ua
  EXPECT_EQ(sub->MatchSnippets("test.com").size(), 1u);
  EXPECT_EQ(sub->MatchSnippets("gov.ua").size(), 1u);
  EXPECT_EQ(sub->MatchSnippets("com").size(), 0u);
  EXPECT_EQ(sub->MatchSnippets("localhost").size(), 1u);
}

TEST_F(AdblockConverterTest, SnippetFiltersSubdomain) {
  auto sub = ConvertAndLoadRules(R"(
     example.com,gov.ua#$#abort-on-property-write 1 2 3
    )",
                                 {}, true);
  EXPECT_EQ(sub->MatchSnippets("www.example.com").size(), 1u);
}

TEST_F(AdblockConverterTest, SnippetsIgnoredForNonpriviledged) {
  auto index = ConvertAndLoadRulesToIndex(R"(
     example.com,gov.ua#$#abort-on-property-write 1 2 3
    )",
                                          {}, false);
  ASSERT_EQ(index->index_->snippet()->size(), 0u);
}

TEST_F(AdblockConverterTest, EscapeSelectors) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com###foo[data-bar='{{foo: 1}}']
    )");
  const auto selectors =
      subscriptions->GetElemhideSelectors(GURL("https://test.com/"), false);

  EXPECT_EQ(FilterSelectors(selectors),
            std::set<base::StringPiece>(
                {"#foo[data-bar='\\7b \\7b foo: 1\\7d \\7d ']"}));
}

TEST_F(AdblockConverterTest, CspFilterForUrl) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$csp=script-src 'self'
    )");

  EXPECT_EQ(subscriptions->FindCspFilter(GURL("https://test.com/resource.jpg"),
                                         "test.com", FilterCategory::Blocking),
            "script-src 'self'");

  // Different URL, not found.
  EXPECT_FALSE(
      subscriptions->FindCspFilter(GURL("https://test.org/resource.jpg"),
                                   "test.com", FilterCategory::Blocking));

  // Allowing filter, not found.
  EXPECT_FALSE(
      subscriptions->FindCspFilter(GURL("https://test.com/resource.jpg"),
                                   "test.com", FilterCategory::Allowing));
}

TEST_F(AdblockConverterTest, CspFilterForDomain) {
  auto subscriptions = ConvertAndLoadRules(R"(
    $csp=script-src 'self' '*' 'unsafe-inline',domain=dom_a.com|dom_b.com
    )");

  EXPECT_EQ(subscriptions->FindCspFilter(GURL("https://test.com/resource.jpg"),
                                         "dom_b.com", FilterCategory::Blocking),
            "script-src 'self' '*' 'unsafe-inline'");
  EXPECT_EQ(subscriptions->FindCspFilter(GURL("https://test.com/resource.jpg"),
                                         "dom_a.com", FilterCategory::Blocking),
            "script-src 'self' '*' 'unsafe-inline'");
  // URL and domain flipped.
  EXPECT_FALSE(
      subscriptions->FindCspFilter(GURL("https://dom_a.com/resource.jpg"),
                                   "test.com", FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, AllowingCspFilterNoPayload) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$csp=script-src 'self'
    @@test.com^$csp
    )");

  EXPECT_EQ(subscriptions->FindCspFilter(GURL("https://test.com/resource.jpg"),
                                         "test.com", FilterCategory::Blocking),
            "script-src 'self'");

  // Allowing filter is found, with an empty payload.
  EXPECT_EQ(subscriptions->FindCspFilter(GURL("https://test.com/resource.jpg"),
                                         "test.com", FilterCategory::Allowing),
            "");
}

TEST_F(AdblockConverterTest, AllowingCspFilterWithPayload) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$csp=script-src 'self'
    @@test.com^$csp=script-src 'self'
    )");

  EXPECT_EQ(subscriptions->FindCspFilter(GURL("https://test.com/resource.jpg"),
                                         "test.com", FilterCategory::Blocking),
            "script-src 'self'");

  // Allowing filter is found, with payload.
  EXPECT_EQ(subscriptions->FindCspFilter(GURL("https://test.com/resource.jpg"),
                                         "test.com", FilterCategory::Allowing),
            "script-src 'self'");
}

TEST_F(AdblockConverterTest, DomainSpecificOnlyCspFilter) {
  // This filter is not domain-specific.
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$csp=script-src 'self'
    )");

  // It's not found when domain_specific_only = true.
  EXPECT_FALSE(subscriptions->FindCspFilter(
      GURL("https://test.com/resource.jpg"), "test.com",
      FilterCategory::DomainSpecificBlocking));
}

TEST_F(AdblockConverterTest, ThirdPartyCspFilters) {
  auto subscriptions = ConvertAndLoadRules(R"(
    only-third.com^$csp=only-third,third-party
    never-third.com^$csp=never-third,~third-party
    )");

  // only-third is only found when the URL is from a different domain.
  EXPECT_EQ(
      subscriptions->FindCspFilter(GURL("https://only-third.com/resource.jpg"),
                                   "different.com", FilterCategory::Blocking),
      "only-third");
  EXPECT_FALSE(
      subscriptions->FindCspFilter(GURL("https://only-third.com/resource.jpg"),
                                   "only-third.com", FilterCategory::Blocking));

  // never-third is only found when the URL is from the same domain.
  EXPECT_EQ(
      subscriptions->FindCspFilter(GURL("https://never-third.com/resource.jpg"),
                                   "never-third.com", FilterCategory::Blocking),
      "never-third");
  EXPECT_FALSE(
      subscriptions->FindCspFilter(GURL("https://never-third.com/resource.jpg"),
                                   "different.com", FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, BlockingCspFilterWithoutPayloadIgnored) {
  // It's impossible to say what CSP header should be injected if the filter
  // doesn't specify.
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$csp
    )");

  EXPECT_FALSE(
      subscriptions->FindCspFilter(GURL("https://test.com/resource.jpg"),
                                   "test.com", FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, RewriteValid) {
  auto subscriptions = ConvertAndLoadRules(R"(
     ||adform.net/banners/scripts/adx.css$domain=delfi.lt,rewrite=abp-resource:blank-css
    )");
  EXPECT_EQ(subscriptions->FindRewriteFilter(
                GURL("https://adform.net/banners/scripts/adx.css"), "delfi.lt",
                FilterCategory::Blocking),
            "data:text/css,");
}

TEST_F(AdblockConverterTest, RewriteFirstParty) {
  auto subscriptions = ConvertAndLoadRules(R"(
     ||adform.net/banners/scripts/adx.js$domain=adform.net,rewrite=abp-resource:blank-js,~third-party
    )");
  EXPECT_EQ(subscriptions->FindRewriteFilter(
                GURL("https://adform.net/banners/scripts/adx.js"), "adform.net",
                FilterCategory::Blocking),
            "data:application/javascript,");
}

TEST_F(AdblockConverterTest, RewriteWrongResource) {
  auto subscriptions = ConvertAndLoadRules(R"(
     ||adform.net/banners/scripts/adx.js$domain=delfi.lt,rewrite=abp-resource:blank-xxx
    )");
  EXPECT_FALSE(subscriptions->FindRewriteFilter(
      GURL("https://adform.net/banners/scripts/adx.js"), "delfi.lt",
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, RewriteWrongScheme) {
  auto subscriptions = ConvertAndLoadRules(R"(
     ||adform.net/banners/scripts/adx.js$domain=delfi.lt,rewrite=about::blank
    )");
  EXPECT_FALSE(subscriptions->FindRewriteFilter(
      GURL("https://adform.net/banners/scripts/adx.js"), "delfi.lt",
      FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, RewriteStar) {
  auto subscriptions = ConvertAndLoadRules(R"(
     *$domain=delfi.lt,rewrite=abp-resource:blank-html
    )");
  EXPECT_EQ(
      subscriptions->FindRewriteFilter(
          GURL("https://adform.net/banners/scripts/adx.html"), "delfi.lt",
          FilterCategory::Blocking),
      "data:text/html,<!DOCTYPE html><html><head></head><body></body></html>");
}

TEST_F(AdblockConverterTest, RewriteWrongStar) {
  auto subscriptions = ConvertAndLoadRules(R"(
     *test.com$domain=delfi.lt,rewrite=abp-resource:blank-html
    )");
  EXPECT_FALSE(subscriptions->FindRewriteFilter(
      GURL("https://test.com/ad.html"), "delfi.lt", FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, RewriteStrict) {
  auto subscriptions = ConvertAndLoadRules(R"(
     test.com$domain=delfi.lt,rewrite=abp-resource:blank-html
    )");
  EXPECT_FALSE(subscriptions->FindRewriteFilter(
      GURL("https://test.com/ad.html"), "delfi.lt", FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, RewriteNoDomain) {
  auto subscriptions = ConvertAndLoadRules(R"(
     ||test.com$rewrite=abp-resource:blank-html
    )");
  EXPECT_FALSE(subscriptions->FindRewriteFilter(
      GURL("https://test.com/ad.html"), "test.com", FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, RewriteExcludeDomain) {
  auto subscriptions = ConvertAndLoadRules(R"(
     ||test.com$domain=~delfi.lt,rewrite=abp-resource:blank-html
    )");
  EXPECT_FALSE(subscriptions->FindRewriteFilter(
      GURL("https://test.com/ad.html"), "test.com", FilterCategory::Blocking));
}

TEST_F(AdblockConverterTest, HeaderFilterIgnoredForNonpriviledged) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$header=X-Frame-Options=sameorigin
    )",
                                           {}, false);

  std::set<HeaderFilterData> blocking_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.jpg"),
                                   ContentType::Image, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  EXPECT_EQ(0u, blocking_filters.size());
}

TEST_F(AdblockConverterTest, BlockingHeaderFilterWithoutPayloadIgnored) {
  // It's impossible to say if request should be blocked if the filter
  // doesn't specify disallowed header.
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$header
    )",
                                           {}, true);

  std::set<HeaderFilterData> blocking_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.jpg"),
                                   ContentType::Image, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  EXPECT_EQ(0u, blocking_filters.size());
}

TEST_F(AdblockConverterTest, HeaderFilterForUrl) {
  GURL subscription_url{"url.com"};
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$header=X-Frame-Options=sameorigin
    )",
                                           {}, true);
  std::set<HeaderFilterData> blocking_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.jpg"),
                                   ContentType::Image, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  EXPECT_TRUE(
      blocking_filters.count({"X-Frame-Options=sameorigin", subscription_url}));

  blocking_filters.clear();
  // Different URL, not found.
  subscriptions->FindHeaderFilters(GURL("https://test.org/resource.jpg"),
                                   ContentType::Image, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  EXPECT_EQ(0u, blocking_filters.size());
}

TEST_F(AdblockConverterTest, NoDomainSpecificHeaderFilter) {
  // This filter is not domain-specific.
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$header=X-Frame-Options=sameorigin
    )",
                                           {}, true);

  std::set<HeaderFilterData> blocking_filters{};
  // It's not found when domain_specific_only = true.
  subscriptions->FindHeaderFilters(
      GURL("https://test.com/resource.jpg"), ContentType::Image, "test.com",
      FilterCategory::DomainSpecificBlocking, blocking_filters);
  EXPECT_EQ(0u, blocking_filters.size());
}

TEST_F(AdblockConverterTest, DomainSpecificHeaderFilter) {
  auto subscriptions = ConvertAndLoadRules(R"(
    $header=X-Frame-Options=sameorigin,domain=dom_a.com|dom_b.com
    )",
                                           {}, true);

  std::set<HeaderFilterData> blocking_filters{};
  subscriptions->FindHeaderFilters(
      GURL("https://test.com/resource.jpg"), ContentType::Image, "dom_b.com",
      FilterCategory::DomainSpecificBlocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  EXPECT_TRUE(blocking_filters.count({"X-Frame-Options=sameorigin", GURL()}));

  blocking_filters.clear();
  subscriptions->FindHeaderFilters(
      GURL("https://test.com/resource.jpg"), ContentType::Image, "dom_a.com",
      FilterCategory::DomainSpecificBlocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  EXPECT_TRUE(blocking_filters.count({"X-Frame-Options=sameorigin", GURL()}));

  blocking_filters.clear();
  subscriptions->FindHeaderFilters(
      GURL("https://dom_a.com/resource.jpg"), ContentType::Image, "test.com",
      FilterCategory::DomainSpecificBlocking, blocking_filters);
  EXPECT_EQ(0u, blocking_filters.size());
}

TEST_F(AdblockConverterTest, HeaderFilterForSpecificResource) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$script,header=X-Frame-Options=sameorigin
    )",
                                           {}, true);

  std::set<HeaderFilterData> blocking_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.js"),
                                   ContentType::Script, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  EXPECT_TRUE(blocking_filters.count({"X-Frame-Options=sameorigin", GURL()}));

  blocking_filters.clear();
  subscriptions->FindHeaderFilters(GURL("https://dom_a.com/resource.jpg"),
                                   ContentType::Image, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  EXPECT_EQ(0u, blocking_filters.size());
}

TEST_F(AdblockConverterTest, HeaderFilterForMultipleSpecificResources) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$script,header=X-Frame-Options=sameorigin
    test.com^$xmlhttprequest,header=X-Frame-Options=sameorigin
    )",
                                           {}, true);

  std::set<HeaderFilterData> blocking_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.js"),
                                   ContentType::Script, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  EXPECT_TRUE(blocking_filters.count({"X-Frame-Options=sameorigin", GURL()}));

  blocking_filters.clear();
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.xml"),
                                   ContentType::Xmlhttprequest, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  EXPECT_TRUE(blocking_filters.count({"X-Frame-Options=sameorigin", GURL()}));

  blocking_filters.clear();
  subscriptions->FindHeaderFilters(GURL("https://dom_a.com/resource.jpg"),
                                   ContentType::Image, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  EXPECT_EQ(0u, blocking_filters.size());
}

TEST_F(AdblockConverterTest, HeaderFilterWithComma) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$script,header=X-Frame-Options=same\x2corigin
    )",
                                           {}, true);

  std::set<HeaderFilterData> blocking_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.js"),
                                   ContentType::Script, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  EXPECT_TRUE(blocking_filters.count({"X-Frame-Options=same,origin", GURL()}));
  EXPECT_TRUE(
      blocking_filters.count({"X-Frame-Options=same\x2corigin", GURL()}));
}

TEST_F(AdblockConverterTest, HeaderFilterWithx2cAsPartOfFilter) {
  auto subscriptions = ConvertAndLoadRules(R"(
    test.com^$script,header=X-Frame-Options=same\\x2corigin
    )",
                                           {}, true);

  std::set<HeaderFilterData> blocking_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.js"),
                                   ContentType::Script, "test.com",
                                   FilterCategory::Blocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  // This count  "X-Frame-Options=same\x2corigin" occurrences, extra \ is
  // omitted during string consctruction
  EXPECT_TRUE(
      blocking_filters.count({"X-Frame-Options=same\\x2corigin", GURL()}));
}

TEST_F(AdblockConverterTest, AllowingHeaderFilterNoPayload) {
  auto subscriptions = ConvertAndLoadRules(R"(
    @@test.com^$header
    )",
                                           {}, true);

  // Allowing filter is found, with an empty payload.
  std::set<HeaderFilterData> allowing_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.jpg"),
                                   ContentType::Image, "test.com",
                                   FilterCategory::Allowing, allowing_filters);
  ASSERT_EQ(1u, allowing_filters.size());
  EXPECT_TRUE(allowing_filters.count({"", GURL()}));
}

TEST_F(AdblockConverterTest, AllowingHeaderFilterWithPayload) {
  auto subscriptions = ConvertAndLoadRules(R"(
    @@test.com^$header=X-Frame-Options=value1
    )",
                                           {}, true);

  std::set<HeaderFilterData> allowing_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.jpg"),
                                   ContentType::Image, "test.com",
                                   FilterCategory::Allowing, allowing_filters);
  ASSERT_EQ(1u, allowing_filters.size());
  EXPECT_TRUE(allowing_filters.count({"X-Frame-Options=value1", GURL()}));
}

TEST_F(AdblockConverterTest, AllowingHeaderFilterForSpecificResource) {
  auto subscriptions = ConvertAndLoadRules(R"(
    @@test.com^$script,header=X-Frame-Options=sameorigin
    )",
                                           {}, true);

  std::set<HeaderFilterData> allowing_filters{};
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.js"),
                                   ContentType::Script, "test.com",
                                   FilterCategory::Allowing, allowing_filters);
  ASSERT_EQ(1u, allowing_filters.size());
  EXPECT_TRUE(allowing_filters.count({"X-Frame-Options=sameorigin", GURL()}));

  allowing_filters.clear();
  subscriptions->FindHeaderFilters(GURL("https://test.com/resource.jpg"),
                                   ContentType::Image, "test.com",
                                   FilterCategory::Allowing, allowing_filters);
  ASSERT_EQ(0u, allowing_filters.size());
}

TEST_F(AdblockConverterTest, ThirdPartyHeaderFilters) {
  auto subscriptions = ConvertAndLoadRules(R"(
    only-third.com^$header=only-third,third-party
    never-third.com^$header=never-third,~third-party
    )",
                                           {}, true);

  std::set<HeaderFilterData> blocking_filters{};
  // only-third is only found when the URL is from a different domain.
  subscriptions->FindHeaderFilters(GURL("https://only-third.com/resource.jpg"),
                                   ContentType::Image, "different.com",
                                   FilterCategory::Blocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  EXPECT_TRUE(blocking_filters.count({"only-third", GURL()}));

  blocking_filters.clear();
  subscriptions->FindHeaderFilters(GURL("https://only-third.com/resource.jpg"),
                                   ContentType::Image, "only-third.com",
                                   FilterCategory::Blocking, blocking_filters);
  EXPECT_EQ(0u, blocking_filters.size());

  // never-third is only found when the URL is from the same domain.
  blocking_filters.clear();
  subscriptions->FindHeaderFilters(GURL("https://never-third.com/resource.jpg"),
                                   ContentType::Image, "never-third.com",
                                   FilterCategory::Blocking, blocking_filters);
  ASSERT_EQ(1u, blocking_filters.size());
  EXPECT_TRUE(blocking_filters.count({"never-third", GURL()}));

  blocking_filters.clear();
  subscriptions->FindHeaderFilters(GURL("https://never-third.com/resource.jpg"),
                                   ContentType::Image, "different.com",
                                   FilterCategory::Blocking, blocking_filters);
  EXPECT_EQ(0u, blocking_filters.size());
}

TEST_F(AdblockConverterTest, UrlFilterWildcardUrlShortKeywords) {
  auto subscription = ConvertAndLoadRules(R"(
    /test*iles/a/b.png
    /test*iles/a/b.js
  )");
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://domain.com/testfiles/a/b.png"), "domain.com",
      ContentType::Image, SiteKey(), FilterCategory::Blocking));
  EXPECT_TRUE(subscription->HasUrlFilter(
      GURL("https://domain.com/testfiles/a/b.js"), "domain.com",
      ContentType::Script, SiteKey(), FilterCategory::Blocking));
}

// TODO(mpawlowski) support multiple CSP filters per URL + frame hierarchy:
// DPD-1145.

}  // namespace adblock
