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
#include <fstream>

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/string_piece_forward.h"
#include "base/strings/string_util.h"
#include "base/synchronization/lock_impl.h"
#include "base/system/sys_info.h"
#include "base/task/thread_pool.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/thread_restrictions.h"
#include "components/adblock/core/classifier/resource_classifier_impl.h"
#include "components/adblock/core/common/adblock_constants.h"
#include "components/adblock/core/common/flatbuffer_data.h"
#include "components/adblock/core/common/sitekey.h"
#include "components/adblock/core/converter/converter.h"
#include "components/adblock/core/subscription/installed_subscription_impl.h"
#include "components/adblock/core/subscription/subscription_collection_impl.h"
#include "components/adblock/core/subscription/subscription_config.h"
#include "components/adblock_comparison/libadblockplus_reference_database.h"

namespace adblock::test {

base::FilePath DefaultDataPath() {
  base::FilePath default_path;
  DCHECK(base::PathService::Get(base::DIR_SOURCE_ROOT, &default_path));
  default_path = default_path.AppendASCII("components")
                     .AppendASCII("test")
                     .AppendASCII("data")
                     .AppendASCII("adblock");
  return default_path;
}

class FlatbufferAdblockVerifier {
 public:
  FlatbufferAdblockVerifier(const base::FilePath& reference_db_file,
                            const base::FilePath& subscription_dir)
      : reference_db_file_(reference_db_file) {
    auto subscriptions_base_path = subscription_dir;
    auto easy_stream =
        GetFileStream(subscriptions_base_path.AppendASCII("easylist.txt"));
    auto exception_stream = GetFileStream(
        subscriptions_base_path.AppendASCII("exceptionrules.txt"));
    auto anti_cv_stream =
        GetFileStream(subscriptions_base_path.AppendASCII("anticv.txt"));

    auto easylist_fb = adblock::Converter().Convert(
        easy_stream, {adblock::DefaultSubscriptionUrl(),
                      adblock::config::AllowPrivilegedFilters(
                          adblock::DefaultSubscriptionUrl())});
    auto exceptions_fb = adblock::Converter().Convert(
        exception_stream,
        {adblock::AcceptableAdsUrl(),
         adblock::config::AllowPrivilegedFilters(adblock::AcceptableAdsUrl())});
    auto anti_cv_fb = adblock::Converter().Convert(
        anti_cv_stream,
        {adblock::AntiCVUrl(),
         adblock::config::AllowPrivilegedFilters(adblock::AntiCVUrl())});
    fb_subscriptions_ = std::make_unique<adblock::SubscriptionCollectionImpl>(
        std::vector<scoped_refptr<adblock::InstalledSubscription>>{
            base::MakeRefCounted<adblock::InstalledSubscriptionImpl>(
                std::move(easylist_fb.data),
                adblock::Subscription::InstallationState::Installed,
                base::Time()),
            base::MakeRefCounted<adblock::InstalledSubscriptionImpl>(
                std::move(exceptions_fb.data),
                adblock::Subscription::InstallationState::Installed,
                base::Time()),
            base::MakeRefCounted<adblock::InstalledSubscriptionImpl>(
                std::move(anti_cv_fb.data),
                adblock::Subscription::InstallationState::Installed,
                base::Time())});
    classifier_ = base::MakeRefCounted<ResourceClassifierImpl>();
  }

  static std::ifstream GetFileStream(const base::FilePath& file) {
    std::ifstream file_stream(file.AsUTF8Unsafe());
    if (!file_stream) {
      assert(false);
    }
    return file_stream;
  }

  bool FlatbufferUrlFilterImplementation(const test::Request& request) {
    std::vector<GURL> frame_hierarchy = {GURL("https://" + request.domain)};
    auto result = classifier_->ClassifyRequest(
        *fb_subscriptions_, request.url, frame_hierarchy,
        static_cast<adblock::ContentType>(request.content_type), SiteKey());
    return result.decision ==
           ResourceClassifier::ClassificationResult::Decision::Blocked;
  }

  test::LibadblockplusReferenceDatabase::ElemhideResults
  FlatbufferElemhideImplementation(const test::Request& request) {
    std::vector<GURL> frame_hierarchy = {GURL("https://" + request.domain)};
    if (fb_subscriptions_->FindBySpecialFilter(SpecialFilterType::Document,
                                               request.url, frame_hierarchy,
                                               SiteKey()) ||
        fb_subscriptions_->FindBySpecialFilter(SpecialFilterType::Elemhide,
                                               request.url, frame_hierarchy,
                                               SiteKey())) {
      return {};
    }

    std::vector<std::string> emu_selectors;
    const auto elemhide_selectors = fb_subscriptions_->GetElementHideSelectors(
        request.url, frame_hierarchy, SiteKey());
    std::string stylesheet = elemhide_selectors.size() == 0
                                 ? ""
                                 : base::JoinString(elemhide_selectors, ", ") +
                                       " {display: none !important;}\n";
    const auto emu =
        fb_subscriptions_->GetElementHideEmulationSelectors(request.url);
    std::transform(emu.begin(), emu.end(), std::back_inserter(emu_selectors),
                   [](const auto& val) { return std::string(val); });
    std::sort(emu_selectors.begin(), emu_selectors.end());
    // TODO (DPD-1279) snippets
    return test::LibadblockplusReferenceDatabase::ElemhideResults(
        std::move(stylesheet), std::move(emu_selectors), "" /* snippets */);
  }

  base::StringPiece MismatchTypeToString(
      LibadblockplusReferenceDatabase::MismatchType type) {
    switch (type) {
      case LibadblockplusReferenceDatabase::MismatchType::ElemhideCss:
        return "Elemhide CSS";
      case LibadblockplusReferenceDatabase::MismatchType::ElemhideEmuSelectors:
        return "Elemhide emulation selectors";
      case LibadblockplusReferenceDatabase::MismatchType::Snippets:
        return "Snippets";
      case LibadblockplusReferenceDatabase::MismatchType::UrlFilterMatch:
        return "URL filter match";
    }
  }

  void OnMismatchFound(int line_number,
                       LibadblockplusReferenceDatabase::MismatchType type) {
    base::AutoLock auto_lock(mismatches_count_lock_);
    LOG(ERROR) << "Found mismatch in line " << line_number << " for "
               << MismatchTypeToString(type);
    mismatches_count_[type]++;
  }

  int VerifyMismatches() {
    if (mismatches_count_.empty()) {
      LOG(INFO) << "All results OK";
    } else {
      for (const auto& pair : mismatches_count_) {
        LOG(INFO) << MismatchTypeToString(pair.first)
                  << " mismatches: " << pair.second;
      }
    }
    return mismatches_count_.empty() ? 0 : 1;
  }

  int RunTest(int number_of_tasks) {
    for (int task_id = 0; task_id < number_of_tasks; task_id++) {
      base::ThreadPool::PostTask(
          base::BindOnce(&FlatbufferAdblockVerifier::RunTestThread,
                         base::Unretained(this), task_id, number_of_tasks),
          FROM_HERE);
    }
    base::ThreadPoolInstance::Get()->JoinForTesting();
    return VerifyMismatches();
  }

  int CheckLines(const std::string& lines) {
    LibadblockplusReferenceDatabase reference_database(reference_db_file_);
    reference_database.CompareImplementationAgainstSpecificRequests(
        base::BindRepeating(
            &FlatbufferAdblockVerifier::FlatbufferUrlFilterImplementation,
            base::Unretained(this)),
        base::BindRepeating(
            &FlatbufferAdblockVerifier::FlatbufferElemhideImplementation,
            base::Unretained(this)),
        base::BindRepeating(&FlatbufferAdblockVerifier::OnMismatchFound,
                            base::Unretained(this)),
        lines);
    return VerifyMismatches();
  }

  void RunTestThread(int task_id, int task_count) {
    base::ScopedAllowBlockingForTesting allow_file_access;
    LibadblockplusReferenceDatabase reference_database(reference_db_file_);
    reference_database.CompareImplementationAgainstReference(
        base::BindRepeating(
            &FlatbufferAdblockVerifier::FlatbufferUrlFilterImplementation,
            base::Unretained(this)),
        base::BindRepeating(
            &FlatbufferAdblockVerifier::FlatbufferElemhideImplementation,
            base::Unretained(this)),
        base::BindRepeating(&FlatbufferAdblockVerifier::OnMismatchFound,
                            base::Unretained(this)),
        task_id, task_count);
  }

 private:
  const base::FilePath reference_db_file_;
  std::unique_ptr<adblock::SubscriptionCollection> fb_subscriptions_;
  scoped_refptr<ResourceClassifier> classifier_;
  base::Lock mismatches_count_lock_;
  std::map<LibadblockplusReferenceDatabase::MismatchType, int>
      mismatches_count_;
};

}  // namespace adblock::test

constexpr char kInputSwitch[] = "input";
constexpr char kSubscriptionDir[] = "subscription-dir";
constexpr char kLinesSwitch[] = "lines";

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  base::ScopedAllowBlockingForTesting allow_file_access;
  const int num_threads = base::SysInfo::NumberOfProcessors();
  base::ThreadPoolInstance::Create("threadpool");
  base::ThreadPoolInstance::Get()->Start(
      base::ThreadPoolInstance::InitParams(num_threads));

  const auto* cmd = base::CommandLine::ForCurrentProcess();
  base::FilePath subscription_dir = adblock::test::DefaultDataPath();
  if (cmd->HasSwitch(kSubscriptionDir))
    subscription_dir = cmd->GetSwitchValuePath(kSubscriptionDir);
  base::FilePath input =
      subscription_dir.AppendASCII("random_shuf_100.tsv.sqlite");
  if (cmd->HasSwitch(kInputSwitch))
    input = cmd->GetSwitchValuePath(kInputSwitch);
  CHECK(base::PathExists(input)) << "Input " << input << " does not exist";
  CHECK(base::PathExists(subscription_dir))
      << "Subscription directory " << subscription_dir << " does not exist";

  std::string lines;
  if (cmd->HasSwitch(kLinesSwitch)) {
    lines = cmd->GetSwitchValueASCII(kLinesSwitch);
  }

  adblock::test::FlatbufferAdblockVerifier verifier(input, subscription_dir);

  if (lines.empty())
    return verifier.RunTest(num_threads);
  return verifier.CheckLines(lines);
}
