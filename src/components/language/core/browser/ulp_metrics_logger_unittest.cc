// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/core/browser/ulp_metrics_logger.h"

#include "base/metrics/metrics_hashes.h"
#include "base/test/metrics/histogram_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace language {

using ::testing::ElementsAre;
using ::testing::IsEmpty;

TEST(ULPMetricsLoggerTest, TestLanguageCount) {
  ULPMetricsLogger logger;
  base::HistogramTester histogram;

  logger.RecordInitiationLanguageCount(2);

  histogram.ExpectUniqueSample(kInitiationLanguageCountHistogram, 2, 1);
}

TEST(ULPMetricsLoggerTest, TestUILanguageStatus) {
  ULPMetricsLogger logger;
  base::HistogramTester histogram;

  logger.RecordInitiationUILanguageInULP(
      ULPLanguageStatus::kTopULPLanguageExactMatch);

  histogram.ExpectUniqueSample(kInitiationUILanguageInULPHistogram,
                               ULPLanguageStatus::kTopULPLanguageExactMatch, 1);
}

TEST(ULPMetricsLoggerTest, TestTranslateTargetStatus) {
  ULPMetricsLogger logger;
  base::HistogramTester histogram;

  logger.RecordInitiationTranslateTargetInULP(
      ULPLanguageStatus::kNonTopULPLanguageExactMatch);

  histogram.ExpectUniqueSample(kInitiationTranslateTargetInULPHistogram,
                               ULPLanguageStatus::kNonTopULPLanguageExactMatch,
                               1);
}

TEST(ULPMetricsLoggerTest, TestTopAcceptLanguageStatus) {
  ULPMetricsLogger logger;
  base::HistogramTester histogram;

  logger.RecordInitiationTopAcceptLanguageInULP(
      ULPLanguageStatus::kLanguageNotInULP);

  histogram.ExpectUniqueSample(kInitiationTopAcceptLanguageInULPHistogram,
                               ULPLanguageStatus::kLanguageNotInULP, 1);
}

TEST(ULPMetricsLoggerTest, TestAcceptLanguagesULPOverlap) {
  ULPMetricsLogger logger;
  base::HistogramTester histogram;

  logger.RecordInitiationAcceptLanguagesULPOverlap(21);

  histogram.ExpectUniqueSample(kInitiationAcceptLanguagesULPOverlapHistogram,
                               21, 1);
}

TEST(ULPMetricsLoggerTest, TestNeverLanguagesMissingFromULP) {
  ULPMetricsLogger logger;
  base::HistogramTester histogram;

  std::vector<std::string> never_languages_not_in_ulp = {"en-US", "de"};
  logger.RecordInitiationNeverLanguagesMissingFromULP(
      never_languages_not_in_ulp);

  histogram.ExpectBucketCount(kInitiationNeverLanguagesMissingFromULP,
                              base::HashMetricName("en-US"), 1);
  histogram.ExpectBucketCount(kInitiationNeverLanguagesMissingFromULP,
                              base::HashMetricName("de"), 1);
}

TEST(ULPMetricsLoggerTest, TestNeverLanguagesMissingFromULPCount) {
  ULPMetricsLogger logger;
  base::HistogramTester histogram;

  logger.RecordInitiationNeverLanguagesMissingFromULPCount(3);
  histogram.ExpectUniqueSample(kInitiationNeverLanguagesMissingFromULPCount, 3,
                               1);
}

TEST(ULPMetricsLoggerTest, TestDetermineLanguageStatus) {
  ULPMetricsLogger logger;
  std::vector<std::string> ulp_languages = {"en-US", "es-419", "pt-BR", "de",
                                            "fr-CA"};

  EXPECT_EQ(ULPLanguageStatus::kTopULPLanguageExactMatch,
            logger.DetermineLanguageStatus("en-US", ulp_languages));

  EXPECT_EQ(ULPLanguageStatus::kNonTopULPLanguageExactMatch,
            logger.DetermineLanguageStatus("de", ulp_languages));

  EXPECT_EQ(ULPLanguageStatus::kTopULPLanguageBaseMatch,
            logger.DetermineLanguageStatus("en-GB", ulp_languages));

  EXPECT_EQ(ULPLanguageStatus::kNonTopULPLanguageBaseMatch,
            logger.DetermineLanguageStatus("pt", ulp_languages));

  EXPECT_EQ(ULPLanguageStatus::kLanguageNotInULP,
            logger.DetermineLanguageStatus("zu", ulp_languages));

  EXPECT_EQ(ULPLanguageStatus::kLanguageEmpty,
            logger.DetermineLanguageStatus("", ulp_languages));

  EXPECT_EQ(ULPLanguageStatus::kLanguageEmpty,
            logger.DetermineLanguageStatus("und", ulp_languages));
}

TEST(ULPMetricsLoggerTest, TestULPLanguagesInAcceptLanguagesRatio) {
  ULPMetricsLogger logger;
  std::vector<std::string> ulp_languages = {"en-US", "es", "pt-BR", "de",
                                            "fr-CA"};

  EXPECT_EQ(0, logger.ULPLanguagesInAcceptLanguagesRatio({"fi-FI", "af", "zu"},
                                                         ulp_languages));

  EXPECT_EQ(20, logger.ULPLanguagesInAcceptLanguagesRatio({"en-GB", "af", "zu"},
                                                          ulp_languages));

  EXPECT_EQ(20, logger.ULPLanguagesInAcceptLanguagesRatio({"en", "af", "zu"},
                                                          ulp_languages));

  EXPECT_EQ(40, logger.ULPLanguagesInAcceptLanguagesRatio(
                    {"en-US", "af", "zu", "es"}, ulp_languages));

  EXPECT_EQ(60, logger.ULPLanguagesInAcceptLanguagesRatio(
                    {"en-US", "af", "pt-BR", "es"}, ulp_languages));

  EXPECT_EQ(60, logger.ULPLanguagesInAcceptLanguagesRatio(
                    {"en", "af", "pt", "es"}, ulp_languages));

  EXPECT_EQ(60, logger.ULPLanguagesInAcceptLanguagesRatio(
                    {"en", "af", "pt-PT", "es"}, ulp_languages));

  EXPECT_EQ(80, logger.ULPLanguagesInAcceptLanguagesRatio(
                    {"en-US", "af", "pt-BR", "es", "de"}, ulp_languages));

  EXPECT_EQ(100,
            logger.ULPLanguagesInAcceptLanguagesRatio(
                {"en-US", "af", "pt-BR", "es", "de", "fr-CA"}, ulp_languages));
}

TEST(ULPMetricsLoggerTest, TestRemoveULPLanguages) {
  ULPMetricsLogger logger;
  std::vector<std::string> ulp_languages = {"en-US", "es", "pt-BR", "de"};

  EXPECT_THAT(
      logger.RemoveULPLanguages({"af", "en", "am", "as"}, ulp_languages),
      ElementsAre("af", "am", "as"));

  EXPECT_THAT(logger.RemoveULPLanguages(
                  {"en-GB", "af", "en-AU", "am", "pt", "as"}, ulp_languages),
              ElementsAre("af", "am", "as"));

  EXPECT_THAT(
      logger.RemoveULPLanguages({"en", "pt-BR", "es-MX"}, ulp_languages),
      IsEmpty());
}

}  // namespace language
