// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/gmock_move_support.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/mock_callback.h"
#include "base/values.h"
#include "chrome/browser/extensions/api/passwords_private/passwords_private_delegate.h"
#include "chrome/browser/extensions/api/passwords_private/passwords_private_delegate_impl.h"
#include "chrome/browser/extensions/api/passwords_private/passwords_private_event_router.h"
#include "chrome/browser/extensions/api/passwords_private/passwords_private_event_router_factory.h"
#include "chrome/browser/password_manager/account_password_store_factory.h"
#include "chrome/browser/password_manager/chrome_password_manager_client.h"
#include "chrome/browser/password_manager/password_manager_test_util.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "chrome/common/extensions/api/passwords_private.h"
#include "chrome/test/base/testing_profile.h"
#include "components/password_manager/content/browser/password_manager_log_router_factory.h"
#include "components/password_manager/core/browser/insecure_credentials_table.h"
#include "components/password_manager/core/browser/mock_password_feature_manager.h"
#include "components/password_manager/core/browser/password_form.h"
#include "components/password_manager/core/browser/password_list_sorter.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"
#include "components/password_manager/core/browser/password_manager_test_utils.h"
#include "components/password_manager/core/browser/reauth_purpose.h"
#include "components/password_manager/core/browser/test_password_store.h"
#include "components/signin/public/base/signin_metrics.h"
#include "components/sync/driver/test_sync_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "extensions/browser/test_event_router.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/clipboard/test/test_clipboard.h"

using MockReauthCallback = base::MockCallback<
    password_manager::PasswordAccessAuthenticator::ReauthCallback>;
using password_manager::ReauthPurpose;
using password_manager::TestPasswordStore;
using ::testing::_;
using ::testing::Eq;
using ::testing::IsNull;
using ::testing::Ne;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SizeIs;
using ::testing::StrictMock;
namespace extensions {

namespace {

constexpr char kHistogramName[] = "PasswordManager.AccessPasswordInSettings";

using MockPlaintextPasswordCallback =
    base::MockCallback<PasswordsPrivateDelegate::PlaintextPasswordCallback>;

class MockPasswordManagerClient : public ChromePasswordManagerClient {
 public:
  // Creates the mock and attaches it to |web_contents|.
  static MockPasswordManagerClient* CreateForWebContentsAndGet(
      content::WebContents* web_contents);

  ~MockPasswordManagerClient() override = default;

  // ChromePasswordManagerClient overrides.
  MOCK_METHOD(void,
              TriggerReauthForPrimaryAccount,
              (signin_metrics::ReauthAccessPoint,
               base::OnceCallback<void(ReauthSucceeded)>),
              (override));
  const password_manager::MockPasswordFeatureManager*
  GetPasswordFeatureManager() const override {
    return &mock_password_feature_manager_;
  }

  password_manager::MockPasswordFeatureManager* GetPasswordFeatureManager() {
    return &mock_password_feature_manager_;
  }

 private:
  explicit MockPasswordManagerClient(content::WebContents* web_contents)
      : ChromePasswordManagerClient(web_contents, nullptr) {}

  password_manager::MockPasswordFeatureManager mock_password_feature_manager_;
};

// static
MockPasswordManagerClient*
MockPasswordManagerClient::CreateForWebContentsAndGet(
    content::WebContents* web_contents) {
  // Avoid creation of log router.
  password_manager::PasswordManagerLogRouterFactory::GetInstance()
      ->SetTestingFactory(
          web_contents->GetBrowserContext(),
          base::BindRepeating(
              [](content::BrowserContext*) -> std::unique_ptr<KeyedService> {
                return nullptr;
              }));
  auto* mock_client = new MockPasswordManagerClient(web_contents);
  web_contents->SetUserData(UserDataKey(), base::WrapUnique(mock_client));
  return mock_client;
}

void SetUpSyncInTransportMode(Profile* profile) {
  auto* sync_service = static_cast<syncer::TestSyncService*>(
      SyncServiceFactory::GetInstance()->SetTestingFactoryAndUse(
          profile,
          base::BindRepeating(
              [](content::BrowserContext*) -> std::unique_ptr<KeyedService> {
                return std::make_unique<syncer::TestSyncService>();
              })));
  CoreAccountInfo account;
  account.email = "foo@gmail.com";
  account.gaia = "foo";
  account.account_id = CoreAccountId::FromGaiaId(account.gaia);
  sync_service->SetAccountInfo(account);
  sync_service->SetDisableReasons({});
  sync_service->SetTransportState(syncer::SyncService::TransportState::ACTIVE);
  sync_service->SetHasSyncConsent(false);
  ASSERT_FALSE(sync_service->IsSyncFeatureEnabled());
}

class PasswordEventObserver
    : public extensions::TestEventRouter::EventObserver {
 public:
  // The observer will only listen to events with the |event_name|.
  explicit PasswordEventObserver(const std::string& event_name);

  PasswordEventObserver(const PasswordEventObserver&) = delete;
  PasswordEventObserver& operator=(const PasswordEventObserver&) = delete;

  ~PasswordEventObserver() override;

  // Removes |event_args_| from |*this| and returns them.
  base::Value PassEventArgs();

  // extensions::TestEventRouter::EventObserver:
  void OnBroadcastEvent(const extensions::Event& event) override;

 private:
  // The name of the observed event.
  const std::string event_name_;

  // The arguments passed for the last observed event.
  base::Value event_args_;
};

PasswordEventObserver::PasswordEventObserver(const std::string& event_name)
    : event_name_(event_name) {}

PasswordEventObserver::~PasswordEventObserver() = default;

base::Value PasswordEventObserver::PassEventArgs() {
  return std::move(event_args_);
}

void PasswordEventObserver::OnBroadcastEvent(const extensions::Event& event) {
  if (event.event_name != event_name_) {
    return;
  }
  event_args_ = base::Value(event.event_args.Clone());
}

std::unique_ptr<KeyedService> BuildPasswordsPrivateEventRouter(
    content::BrowserContext* context) {
  return std::unique_ptr<KeyedService>(
      PasswordsPrivateEventRouter::Create(context));
}

password_manager::PasswordForm CreateSampleForm(
    password_manager::PasswordForm::Store store =
        password_manager::PasswordForm::Store::kProfileStore) {
  password_manager::PasswordForm form;
  form.signon_realm = "http://abc1.com";
  form.url = GURL("http://abc1.com");
  form.username_value = u"test@gmail.com";
  form.password_value = u"test";
  form.in_store = store;
  return form;
}

MATCHER_P(PasswordUiEntryDataEquals, expected, "") {
  return testing::Value(expected.get().urls.link, arg.urls.link) &&
         testing::Value(expected.get().username, arg.username) &&
         testing::Value(expected.get().password_note, arg.password_note) &&
         testing::Value(expected.get().stored_in, arg.stored_in) &&
         testing::Value(expected.get().is_android_credential,
                        arg.is_android_credential);
}

}  // namespace

class PasswordsPrivateDelegateImplTest : public testing::Test {
 public:
  PasswordsPrivateDelegateImplTest();

  PasswordsPrivateDelegateImplTest(const PasswordsPrivateDelegateImplTest&) =
      delete;
  PasswordsPrivateDelegateImplTest& operator=(
      const PasswordsPrivateDelegateImplTest&) = delete;

  ~PasswordsPrivateDelegateImplTest() override;

  // Sets up a testing password store and fills it with |forms|.
  void SetUpPasswordStores(std::vector<password_manager::PasswordForm> forms);

  // Sets up a testing EventRouter with a production
  // PasswordsPrivateEventRouter.
  void SetUpRouters();

  base::HistogramTester& histogram_tester() { return histogram_tester_; }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  raw_ptr<extensions::TestEventRouter> event_router_ = nullptr;
  scoped_refptr<TestPasswordStore> profile_store_ =
      CreateAndUseTestPasswordStore(&profile_);
  scoped_refptr<TestPasswordStore> account_store_ =
      CreateAndUseTestAccountPasswordStore(&profile_);
  raw_ptr<ui::TestClipboard> test_clipboard_ =
      ui::TestClipboard::CreateForCurrentThread();

 private:
  base::HistogramTester histogram_tester_;
};

PasswordsPrivateDelegateImplTest::PasswordsPrivateDelegateImplTest() {
  SetUpRouters();
  SetUpSyncInTransportMode(&profile_);
}

PasswordsPrivateDelegateImplTest::~PasswordsPrivateDelegateImplTest() {
  ui::Clipboard::DestroyClipboardForCurrentThread();
}

void PasswordsPrivateDelegateImplTest::SetUpPasswordStores(
    std::vector<password_manager::PasswordForm> forms) {
  for (const password_manager::PasswordForm& form : forms) {
    if (form.IsUsingAccountStore())
      account_store_->AddLogin(form);
    else if (form.IsUsingProfileStore())
      profile_store_->AddLogin(form);
    else
      NOTREACHED() << "Store not set";
  }
  // Spin the loop to allow PasswordStore tasks being processed.
  base::RunLoop().RunUntilIdle();
}

void PasswordsPrivateDelegateImplTest::SetUpRouters() {
  event_router_ = extensions::CreateAndUseTestEventRouter(&profile_);
  // Set the production PasswordsPrivateEventRouter::Create as a testing
  // factory, because at some point during the preceding initialization, a null
  // factory is set, resulting in nul PasswordsPrivateEventRouter.
  PasswordsPrivateEventRouterFactory::GetInstance()->SetTestingFactory(
      &profile_, base::BindRepeating(&BuildPasswordsPrivateEventRouter));
}

TEST_F(PasswordsPrivateDelegateImplTest, GetSavedPasswordsList) {
  PasswordsPrivateDelegateImpl delegate(&profile_);

  base::MockCallback<PasswordsPrivateDelegate::UiEntriesCallback> callback;
  EXPECT_CALL(callback, Run).Times(0);
  delegate.GetSavedPasswordsList(callback.Get());

  EXPECT_CALL(callback, Run);
  SetUpPasswordStores({});

  EXPECT_CALL(callback, Run);
  delegate.GetSavedPasswordsList(callback.Get());
}

TEST_F(PasswordsPrivateDelegateImplTest,
       PasswordsDuplicatedInStoresAreRepresentedAsSingleEntity) {
  PasswordsPrivateDelegateImpl delegate(&profile_);

  password_manager::PasswordForm account_password =
      CreateSampleForm(password_manager::PasswordForm::Store::kAccountStore);
  password_manager::PasswordForm profile_password =
      CreateSampleForm(password_manager::PasswordForm::Store::kProfileStore);

  SetUpPasswordStores({account_password, profile_password});

  base::MockCallback<PasswordsPrivateDelegate::UiEntriesCallback> callback;
  EXPECT_CALL(callback, Run(SizeIs(1)))
      .WillOnce([&](const PasswordsPrivateDelegate::UiEntries& passwords) {
        EXPECT_EQ(api::passwords_private::PASSWORD_STORE_SET_DEVICE_AND_ACCOUNT,
                  passwords[0].stored_in);
      });

  delegate.GetSavedPasswordsList(callback.Get());
}

TEST_F(PasswordsPrivateDelegateImplTest, GetPasswordExceptionsList) {
  PasswordsPrivateDelegateImpl delegate(&profile_);

  base::MockCallback<PasswordsPrivateDelegate::ExceptionEntriesCallback>
      callback;
  EXPECT_CALL(callback, Run).Times(0);
  delegate.GetPasswordExceptionsList(callback.Get());

  EXPECT_CALL(callback, Run);
  SetUpPasswordStores({});

  EXPECT_CALL(callback, Run);
  delegate.GetPasswordExceptionsList(callback.Get());
}

TEST_F(PasswordsPrivateDelegateImplTest,
       ExceptionsDuplicatedInStoresAreRepresentedAsSingleEntity) {
  PasswordsPrivateDelegateImpl delegate(&profile_);
  password_manager::PasswordForm account_exception;
  account_exception.blocked_by_user = true;
  account_exception.url = GURL("https://test.com");
  account_exception.in_store =
      password_manager::PasswordForm::Store::kAccountStore;
  password_manager::PasswordForm profile_exception;
  profile_exception.url = GURL("https://test.com");
  profile_exception.blocked_by_user = true;
  profile_exception.in_store =
      password_manager::PasswordForm::Store::kProfileStore;

  SetUpPasswordStores({account_exception, profile_exception});

  base::MockCallback<PasswordsPrivateDelegate::ExceptionEntriesCallback>
      callback;

  EXPECT_CALL(callback, Run(SizeIs(1)));
  delegate.GetPasswordExceptionsList(callback.Get());
}

TEST_F(PasswordsPrivateDelegateImplTest, AddPassword) {
  // This enables uses of TestWebContents.
  content::RenderViewHostTestEnabler test_render_host_factories;
  std::unique_ptr<content::WebContents> web_contents =
      content::WebContentsTester::CreateTestWebContents(&profile_, nullptr);
  auto* client =
      MockPasswordManagerClient::CreateForWebContentsAndGet(web_contents.get());
  ON_CALL(*(client->GetPasswordFeatureManager()), IsOptedInForAccountStorage)
      .WillByDefault(Return(false));
  PasswordsPrivateDelegateImpl delegate(&profile_);
  // Spin the loop to allow PasswordStore tasks posted on the creation of
  // |delegate| to be completed.
  base::RunLoop().RunUntilIdle();

  // Double check that the contents of the passwords list matches our
  // expectation.
  base::MockCallback<PasswordsPrivateDelegate::UiEntriesCallback> callback;
  EXPECT_CALL(callback, Run(SizeIs(0)));
  delegate.GetSavedPasswordsList(callback.Get());

  EXPECT_TRUE(
      delegate.AddPassword(/*url=*/"example1.com", /*username=*/u"username1",
                           /*password=*/u"password1", /*note=*/u"",
                           /*use_account_store=*/true, web_contents.get()));
  EXPECT_TRUE(delegate.AddPassword(
      /*url=*/"http://example2.com/login?param=value",
      /*username=*/u"", /*password=*/u"password2", /*note=*/u"note",
      /*use_account_store=*/false, web_contents.get()));
  // Spin the loop to allow PasswordStore tasks posted when adding the
  // password to be completed.
  base::RunLoop().RunUntilIdle();

  // Check that adding passwords got reflected in the passwords list.
  api::passwords_private::PasswordUiEntry expected_entry1;
  expected_entry1.urls.link = "https://example1.com/";
  expected_entry1.username = "username1";
  expected_entry1.password_note = "";
  expected_entry1.stored_in =
      api::passwords_private::PASSWORD_STORE_SET_ACCOUNT;
  api::passwords_private::PasswordUiEntry expected_entry2;
  expected_entry2.urls.link = "http://example2.com/login";
  expected_entry2.username = "";
  expected_entry2.password_note = "note";
  expected_entry2.stored_in = api::passwords_private::PASSWORD_STORE_SET_DEVICE;
  EXPECT_CALL(callback,
              Run(testing::UnorderedElementsAre(
                  PasswordUiEntryDataEquals(testing::ByRef(expected_entry1)),
                  PasswordUiEntryDataEquals(testing::ByRef(expected_entry2)))));
  delegate.GetSavedPasswordsList(callback.Get());
}

TEST_F(PasswordsPrivateDelegateImplTest, AddPasswordUpdatesDefaultStore) {
  // This enables uses of TestWebContents.
  content::RenderViewHostTestEnabler test_render_host_factories;
  std::unique_ptr<content::WebContents> web_contents =
      content::WebContentsTester::CreateTestWebContents(&profile_, nullptr);
  auto* client =
      MockPasswordManagerClient::CreateForWebContentsAndGet(web_contents.get());
  PasswordsPrivateDelegateImpl delegate(&profile_);

  // NOT update default store if not opted-in for account storage.
  ON_CALL(*(client->GetPasswordFeatureManager()), IsOptedInForAccountStorage)
      .WillByDefault(Return(false));
  EXPECT_CALL(*(client->GetPasswordFeatureManager()), SetDefaultPasswordStore)
      .Times(0);
  EXPECT_TRUE(
      delegate.AddPassword("example1.com", u"username1", u"password1", u"",
                           /*use_account_store=*/false, web_contents.get()));

  // Updates the default store if opted-in and operation succeeded.
  ON_CALL(*(client->GetPasswordFeatureManager()), IsOptedInForAccountStorage)
      .WillByDefault(Return(true));
  EXPECT_CALL(*(client->GetPasswordFeatureManager()),
              SetDefaultPasswordStore(
                  password_manager::PasswordForm::Store::kAccountStore));
  EXPECT_TRUE(
      delegate.AddPassword("example2.com", u"username2", u"password2", u"",
                           /*use_account_store=*/true, web_contents.get()));

  // NOT update default store if opted-in, but operation failed.
  EXPECT_CALL(*(client->GetPasswordFeatureManager()), SetDefaultPasswordStore)
      .Times(0);
  EXPECT_FALSE(delegate.AddPassword("", u"", u"", u"",
                                    /*use_account_store=*/false,
                                    web_contents.get()));
}

TEST_F(PasswordsPrivateDelegateImplTest, ChangeSavedPassword) {
  password_manager::PasswordForm sample_form = CreateSampleForm();
  SetUpPasswordStores({sample_form});

  PasswordsPrivateDelegateImpl delegate(&profile_);
  // Spin the loop to allow PasswordStore tasks posted on the creation of
  // |delegate| to be completed.
  base::RunLoop().RunUntilIdle();

  // Double check that the contents of the passwords list matches our
  // expectation.
  base::MockCallback<PasswordsPrivateDelegate::UiEntriesCallback> callback;
  EXPECT_CALL(callback, Run(SizeIs(1)))
      .WillOnce([&](const PasswordsPrivateDelegate::UiEntries& passwords) {
        EXPECT_EQ(sample_form.username_value,
                  base::UTF8ToUTF16(passwords[0].username));
      });
  delegate.GetSavedPasswordsList(callback.Get());
  int sample_form_id = delegate.GetIdForCredential(
      password_manager::CredentialUIEntry(sample_form));

  api::passwords_private::ChangeSavedPasswordParams params;
  params.password = "new_pass";
  params.username = "new_user";

  sample_form.username_value = u"new_user";
  sample_form.password_value = u"new_pass";
  int new_form_id = delegate.GetIdForCredential(
      password_manager::CredentialUIEntry(sample_form));

  auto result = delegate.ChangeSavedPassword({sample_form_id}, params);
  EXPECT_THAT(result->account_id, IsNull());
  EXPECT_THAT(result->device_id, Pointee(new_form_id));

  // Spin the loop to allow PasswordStore tasks posted when changing the
  // password to be completed.
  base::RunLoop().RunUntilIdle();

  // Check that the changing the password got reflected in the passwords list.
  EXPECT_CALL(callback, Run(SizeIs(1)))
      .WillOnce([](const PasswordsPrivateDelegate::UiEntries& passwords) {
        EXPECT_EQ("new_user", passwords[0].username);
        EXPECT_EQ("", passwords[0].password_note);
      });
  delegate.GetSavedPasswordsList(callback.Get());
}

TEST_F(PasswordsPrivateDelegateImplTest, ChangeSavedPasswordWithNote) {
  password_manager::PasswordForm sample_form = CreateSampleForm();
  sample_form.notes.emplace_back(
      u"display name", u"note with non-empty display name",
      /*date_created=*/base::Time::Now(), /*hide_by_default=*/true);
  sample_form.notes.emplace_back(u"note with empty display name",
                                 /*date_created=*/base::Time::Now());
  SetUpPasswordStores({sample_form});

  PasswordsPrivateDelegateImpl delegate(&profile_);
  // Spin the loop to allow PasswordStore tasks posted on the creation of
  // |delegate| to be completed.
  base::RunLoop().RunUntilIdle();

  // Double check that the contents of the passwords list matches our
  // expectation. The note with an empty `unique_display_name` is returned.
  base::MockCallback<PasswordsPrivateDelegate::UiEntriesCallback> callback;
  EXPECT_CALL(callback, Run(SizeIs(1)))
      .WillOnce([&](const PasswordsPrivateDelegate::UiEntries& passwords) {
        EXPECT_EQ(sample_form.username_value,
                  base::UTF8ToUTF16(passwords[0].username));
        EXPECT_EQ(sample_form.notes[1].value,
                  base::UTF8ToUTF16(passwords[0].password_note));
      });
  delegate.GetSavedPasswordsList(callback.Get());
  int sample_form_id = delegate.GetIdForCredential(
      password_manager::CredentialUIEntry(sample_form));

  api::passwords_private::ChangeSavedPasswordParams params;
  params.password = "new_pass";
  params.username = "new_user";
  params.note = std::make_unique<std::string>("new note");

  sample_form.password_value = u"new_pass";
  sample_form.username_value = u"new_user";
  int new_form_id = delegate.GetIdForCredential(
      password_manager::CredentialUIEntry(sample_form));

  auto result = delegate.ChangeSavedPassword({sample_form_id}, params);
  EXPECT_THAT(result->account_id, IsNull());
  EXPECT_THAT(result->device_id, Pointee(new_form_id));

  // Spin the loop to allow PasswordStore tasks posted when changing the
  // password to be completed.
  base::RunLoop().RunUntilIdle();

  // Check that the changing the password got reflected in the passwords
  EXPECT_CALL(callback, Run(SizeIs(1)))
      .WillOnce([](const PasswordsPrivateDelegate::UiEntries& passwords) {
        EXPECT_EQ("new_user", passwords[0].username);
        EXPECT_EQ("new note", passwords[0].password_note);
      });
  delegate.GetSavedPasswordsList(callback.Get());
}

TEST_F(PasswordsPrivateDelegateImplTest, ChangeSavedPasswordInBothStores) {
  password_manager::PasswordForm profile_form = CreateSampleForm();
  password_manager::PasswordForm account_form = profile_form;
  account_form.in_store = password_manager::PasswordForm::Store::kAccountStore;
  SetUpPasswordStores({profile_form, account_form});

  PasswordsPrivateDelegateImpl delegate(&profile_);
  // Spin the loop to allow PasswordStore tasks posted on the creation of
  // |delegate| to be completed.
  base::RunLoop().RunUntilIdle();

  int profile_form_id = delegate.GetIdForCredential(
      password_manager::CredentialUIEntry(profile_form));
  int account_form_id = delegate.GetIdForCredential(
      password_manager::CredentialUIEntry(account_form));

  api::passwords_private::ChangeSavedPasswordParams params;
  params.password = "new_pass";
  params.username = "new_user";

  profile_form.username_value = u"new_user";
  profile_form.password_value = u"new_pass";
  int new_profile_form_id = delegate.GetIdForCredential(
      password_manager::CredentialUIEntry(profile_form));
  account_form.username_value = u"new_user";
  account_form.password_value = u"new_pass";
  int new_account_form_id = delegate.GetIdForCredential(
      password_manager::CredentialUIEntry(account_form));

  auto result =
      delegate.ChangeSavedPassword({profile_form_id, account_form_id}, params);
  EXPECT_THAT(result->account_id, Pointee(new_account_form_id));
  EXPECT_THAT(result->device_id, Pointee(new_profile_form_id));
}

TEST_F(PasswordsPrivateDelegateImplTest, ChangeSavedPasswordInAccountStore) {
  password_manager::PasswordForm profile_form = CreateSampleForm();
  profile_form.password_value = u"different_pass";
  password_manager::PasswordForm account_form = CreateSampleForm();
  account_form.in_store = password_manager::PasswordForm::Store::kAccountStore;
  SetUpPasswordStores({profile_form, account_form});

  PasswordsPrivateDelegateImpl delegate(&profile_);
  // Spin the loop to allow PasswordStore tasks posted on the creation of
  // |delegate| to be completed.
  base::RunLoop().RunUntilIdle();

  int account_form_id = delegate.GetIdForCredential(
      password_manager::CredentialUIEntry(account_form));

  api::passwords_private::ChangeSavedPasswordParams params;
  params.password = "new_pass";
  params.username = "new_user";

  account_form.username_value = u"new_user";
  account_form.password_value = u"new_pass";
  int new_account_form_id = delegate.GetIdForCredential(
      password_manager::CredentialUIEntry(account_form));

  auto result = delegate.ChangeSavedPassword({account_form_id}, params);
  EXPECT_THAT(result->account_id, Pointee(new_account_form_id));
  EXPECT_THAT(result->device_id, IsNull());
}

// Checking callback result of RequestPlaintextPassword with reason Copy.
// By implementation for Copy, callback will receive empty string.
TEST_F(PasswordsPrivateDelegateImplTest, TestCopyPasswordCallbackResult) {
  password_manager::PasswordForm form = CreateSampleForm();
  SetUpPasswordStores({form});

  PasswordsPrivateDelegateImpl delegate(&profile_);
  base::RunLoop().RunUntilIdle();

  MockReauthCallback callback;
  delegate.set_os_reauth_call(callback.Get());

  EXPECT_CALL(callback, Run(ReauthPurpose::COPY_PASSWORD, _))
      .WillOnce(testing::WithArg<1>(
          [&](password_manager::PasswordAccessAuthenticator::AuthResultCallback
                  callback) { std::move(callback).Run(true); }));

  MockPlaintextPasswordCallback password_callback;
  EXPECT_CALL(password_callback, Run(Eq(std::u16string())));
  delegate.RequestPlaintextPassword(
      0, api::passwords_private::PLAINTEXT_REASON_COPY, password_callback.Get(),
      nullptr);

  std::u16string result;
  test_clipboard_->ReadText(ui::ClipboardBuffer::kCopyPaste,
                            /* data_dst = */ nullptr, &result);
  EXPECT_EQ(form.password_value, result);

  histogram_tester().ExpectUniqueSample(
      kHistogramName, password_manager::metrics_util::ACCESS_PASSWORD_COPIED,
      1);
}

TEST_F(PasswordsPrivateDelegateImplTest, TestShouldReauthForOptIn) {
  // This enables uses of TestWebContents.
  content::RenderViewHostTestEnabler test_render_host_factories;

  std::unique_ptr<content::WebContents> web_contents =
      content::WebContentsTester::CreateTestWebContents(&profile_, nullptr);
  auto* client =
      MockPasswordManagerClient::CreateForWebContentsAndGet(web_contents.get());
  ON_CALL(*(client->GetPasswordFeatureManager()), IsOptedInForAccountStorage)
      .WillByDefault(Return(false));

  EXPECT_CALL(*client,
              TriggerReauthForPrimaryAccount(
                  signin_metrics::ReauthAccessPoint::kPasswordSettings, _));

  PasswordsPrivateDelegateImpl delegate(&profile_);
  delegate.SetAccountStorageOptIn(true, web_contents.get());
}

TEST_F(PasswordsPrivateDelegateImplTest,
       TestShouldNotReauthForOptOutAndShouldSetPref) {
  // This enables uses of TestWebContents.
  content::RenderViewHostTestEnabler test_render_host_factories;

  std::unique_ptr<content::WebContents> web_contents =
      content::WebContentsTester::CreateTestWebContents(&profile_, nullptr);
  auto* client =
      MockPasswordManagerClient::CreateForWebContentsAndGet(web_contents.get());
  password_manager::MockPasswordFeatureManager* feature_manager =
      client->GetPasswordFeatureManager();
  ON_CALL(*feature_manager, IsOptedInForAccountStorage)
      .WillByDefault(Return(true));

  EXPECT_CALL(*client,
              TriggerReauthForPrimaryAccount(
                  signin_metrics::ReauthAccessPoint::kPasswordSettings, _))
      .Times(0);
  EXPECT_CALL(*feature_manager, OptOutOfAccountStorageAndClearSettings);

  PasswordsPrivateDelegateImpl delegate(&profile_);
  delegate.SetAccountStorageOptIn(false, web_contents.get());
}

TEST_F(PasswordsPrivateDelegateImplTest, TestCopyPasswordCallbackResultFail) {
  SetUpPasswordStores({CreateSampleForm()});

  PasswordsPrivateDelegateImpl delegate(&profile_);
  base::RunLoop().RunUntilIdle();

  MockReauthCallback callback;
  delegate.set_os_reauth_call(callback.Get());

  EXPECT_CALL(callback, Run(ReauthPurpose::COPY_PASSWORD, _))
      .WillOnce(testing::WithArg<1>(
          [&](password_manager::PasswordAccessAuthenticator::AuthResultCallback
                  callback) { std::move(callback).Run(false); }));

  base::Time before_call = test_clipboard_->GetLastModifiedTime();

  MockPlaintextPasswordCallback password_callback;
  EXPECT_CALL(password_callback, Run(Eq(absl::nullopt)));
  delegate.RequestPlaintextPassword(
      0, api::passwords_private::PLAINTEXT_REASON_COPY, password_callback.Get(),
      nullptr);
  // Clipboard should not be modifiend in case Reauth failed
  std::u16string result;
  test_clipboard_->ReadText(ui::ClipboardBuffer::kCopyPaste,
                            /* data_dst = */ nullptr, &result);
  EXPECT_EQ(std::u16string(), result);
  EXPECT_EQ(before_call, test_clipboard_->GetLastModifiedTime());

  // Since Reauth had failed password was not copied and metric wasn't recorded
  histogram_tester().ExpectTotalCount(kHistogramName, 0);
}

TEST_F(PasswordsPrivateDelegateImplTest, TestPassedReauthOnView) {
  SetUpPasswordStores({CreateSampleForm()});

  PasswordsPrivateDelegateImpl delegate(&profile_);
  // Spin the loop to allow PasswordStore tasks posted on the creation of
  // |delegate| to be completed.
  base::RunLoop().RunUntilIdle();

  MockReauthCallback callback;
  delegate.set_os_reauth_call(callback.Get());

  EXPECT_CALL(callback, Run(ReauthPurpose::VIEW_PASSWORD, _))
      .WillOnce(testing::WithArg<1>(
          [&](password_manager::PasswordAccessAuthenticator::AuthResultCallback
                  callback) { std::move(callback).Run(true); }));

  MockPlaintextPasswordCallback password_callback;
  EXPECT_CALL(password_callback, Run(Eq(u"test")));
  delegate.RequestPlaintextPassword(
      0, api::passwords_private::PLAINTEXT_REASON_VIEW, password_callback.Get(),
      nullptr);

  histogram_tester().ExpectUniqueSample(
      kHistogramName, password_manager::metrics_util::ACCESS_PASSWORD_VIEWED,
      1);
}

TEST_F(PasswordsPrivateDelegateImplTest, TestFailedReauthOnView) {
  SetUpPasswordStores({CreateSampleForm()});

  PasswordsPrivateDelegateImpl delegate(&profile_);
  // Spin the loop to allow PasswordStore tasks posted on the creation of
  // |delegate| to be completed.
  base::RunLoop().RunUntilIdle();

  MockReauthCallback callback;
  delegate.set_os_reauth_call(callback.Get());

  EXPECT_CALL(callback, Run(ReauthPurpose::VIEW_PASSWORD, _))
      .WillOnce(testing::WithArg<1>(
          [&](password_manager::PasswordAccessAuthenticator::AuthResultCallback
                  callback) { std::move(callback).Run(false); }));

  MockPlaintextPasswordCallback password_callback;
  EXPECT_CALL(password_callback, Run(Eq(absl::nullopt)));
  delegate.RequestPlaintextPassword(
      0, api::passwords_private::PLAINTEXT_REASON_VIEW, password_callback.Get(),
      nullptr);

  // Since Reauth had failed password was not viewed and metric wasn't recorded
  histogram_tester().ExpectTotalCount(kHistogramName, 0);
}

TEST_F(PasswordsPrivateDelegateImplTest, TestReauthFailedOnExport) {
  SetUpPasswordStores({CreateSampleForm()});
  StrictMock<base::MockCallback<base::OnceCallback<void(const std::string&)>>>
      mock_accepted;

  PasswordsPrivateDelegateImpl delegate(&profile_);
  // Spin the loop to allow PasswordStore tasks posted on the creation of
  // |delegate| to be completed.
  base::RunLoop().RunUntilIdle();

  EXPECT_CALL(mock_accepted, Run(std::string("reauth-failed")));

  MockReauthCallback callback;
  delegate.set_os_reauth_call(callback.Get());

  EXPECT_CALL(callback, Run(ReauthPurpose::EXPORT, _))
      .WillOnce(testing::WithArg<1>(
          [&](password_manager::PasswordAccessAuthenticator::AuthResultCallback
                  callback) { std::move(callback).Run(false); }));
  delegate.ExportPasswords(mock_accepted.Get(), nullptr);
}

// Verifies that PasswordsPrivateDelegateImpl::GetPlaintextInsecurePassword
// fails if the re-auth fails.
TEST_F(PasswordsPrivateDelegateImplTest,
       TestReauthOnGetPlaintextInsecurePasswordFails) {
  PasswordsPrivateDelegateImpl delegate(&profile_);

  MockReauthCallback reauth_callback;
  delegate.set_os_reauth_call(reauth_callback.Get());

  base::MockCallback<
      PasswordsPrivateDelegate::PlaintextInsecurePasswordCallback>
      credential_callback;

  EXPECT_CALL(reauth_callback, Run(ReauthPurpose::VIEW_PASSWORD, _))
      .WillOnce(testing::WithArg<1>(
          [&](password_manager::PasswordAccessAuthenticator::AuthResultCallback
                  callback) { std::move(callback).Run(false); }));

  EXPECT_CALL(credential_callback, Run(Eq(absl::nullopt)));

  delegate.GetPlaintextInsecurePassword(
      api::passwords_private::InsecureCredential(),
      api::passwords_private::PLAINTEXT_REASON_VIEW, nullptr,
      credential_callback.Get());
}

// Verifies that PasswordsPrivateDelegateImpl::GetPlaintextInsecurePassword
// succeeds if the re-auth succeeds and there is a matching compromised
// credential in the store.
TEST_F(PasswordsPrivateDelegateImplTest, TestReauthOnGetPlaintextCompPassword) {
  PasswordsPrivateDelegateImpl delegate(&profile_);

  password_manager::PasswordForm form = CreateSampleForm();
  form.password_issues = {
      {password_manager::InsecureType::kLeaked,
       password_manager::InsecurityMetadata(base::Time::FromTimeT(1),
                                            password_manager::IsMuted(false))}};
  profile_store_->AddLogin(form);
  base::RunLoop().RunUntilIdle();

  api::passwords_private::InsecureCredential credential =
      std::move(delegate.GetCompromisedCredentials().at(0));

  MockReauthCallback reauth_callback;
  delegate.set_os_reauth_call(reauth_callback.Get());

  base::MockCallback<
      PasswordsPrivateDelegate::PlaintextInsecurePasswordCallback>
      credential_callback;

  absl::optional<api::passwords_private::InsecureCredential> opt_credential;
  EXPECT_CALL(reauth_callback, Run(ReauthPurpose::VIEW_PASSWORD, _))
      .WillOnce(testing::WithArg<1>(
          [&](password_manager::PasswordAccessAuthenticator::AuthResultCallback
                  callback) { std::move(callback).Run(true); }));
  EXPECT_CALL(credential_callback, Run).WillOnce(MoveArg(&opt_credential));

  delegate.GetPlaintextInsecurePassword(
      std::move(credential), api::passwords_private::PLAINTEXT_REASON_VIEW,
      nullptr, credential_callback.Get());

  ASSERT_TRUE(opt_credential.has_value());
  EXPECT_EQ(form.signon_realm, opt_credential->signon_realm);
  EXPECT_EQ(form.username_value, base::UTF8ToUTF16(opt_credential->username));
  EXPECT_EQ(form.password_value, base::UTF8ToUTF16(*opt_credential->password));
}

TEST_F(PasswordsPrivateDelegateImplTest,
       GetUrlCollectionValueWithSchemeWhenIpAddress) {
  PasswordsPrivateDelegateImpl delegate(&profile_);
  const absl::optional<api::passwords_private::UrlCollection> urls =
      delegate.GetUrlCollection("127.0.0.1");
  EXPECT_TRUE(urls.has_value());
  EXPECT_EQ("127.0.0.1", urls.value().shown);
  EXPECT_EQ("http://127.0.0.1/", urls.value().origin);
  EXPECT_EQ("http://127.0.0.1/", urls.value().link);
}

TEST_F(PasswordsPrivateDelegateImplTest,
       GetUrlCollectionValueWithSchemeWhenWebAddress) {
  PasswordsPrivateDelegateImpl delegate(&profile_);
  const absl::optional<api::passwords_private::UrlCollection> urls =
      delegate.GetUrlCollection("example.com/login");
  EXPECT_TRUE(urls.has_value());
  EXPECT_EQ("example.com", urls.value().shown);
  EXPECT_EQ("https://example.com/", urls.value().origin);
  EXPECT_EQ("https://example.com/login", urls.value().link);
}

TEST_F(PasswordsPrivateDelegateImplTest,
       GetUrlCollectionStrippedValueWhenFullUrl) {
  PasswordsPrivateDelegateImpl delegate(&profile_);
  const absl::optional<api::passwords_private::UrlCollection> urls =
      delegate.GetUrlCollection(
          "http://username:password@example.com/login?param=value#ref");
  EXPECT_TRUE(urls.has_value());
  EXPECT_EQ("example.com", urls.value().shown);
  EXPECT_EQ("http://example.com/", urls.value().origin);
  EXPECT_EQ("http://example.com/login", urls.value().link);
}

TEST_F(PasswordsPrivateDelegateImplTest,
       GetUrlCollectionNoValueWhenUnsupportedScheme) {
  PasswordsPrivateDelegateImpl delegate(&profile_);
  const absl::optional<api::passwords_private::UrlCollection> urls =
      delegate.GetUrlCollection("scheme://unsupported");
  EXPECT_FALSE(urls.has_value());
}

TEST_F(PasswordsPrivateDelegateImplTest,
       GetUrlCollectionNoValueWhenInvalidUrl) {
  PasswordsPrivateDelegateImpl delegate(&profile_);
  const absl::optional<api::passwords_private::UrlCollection> urls =
      delegate.GetUrlCollection("https://;/invalid");
  EXPECT_FALSE(urls.has_value());
}

TEST_F(PasswordsPrivateDelegateImplTest, IsAccountStoreDefault) {
  // This enables uses of TestWebContents.
  content::RenderViewHostTestEnabler test_render_host_factories;
  std::unique_ptr<content::WebContents> web_contents =
      content::WebContentsTester::CreateTestWebContents(&profile_, nullptr);
  auto* client =
      MockPasswordManagerClient::CreateForWebContentsAndGet(web_contents.get());
  ON_CALL(*(client->GetPasswordFeatureManager()), IsOptedInForAccountStorage)
      .WillByDefault(Return(true));
  PasswordsPrivateDelegateImpl delegate(&profile_);

  EXPECT_CALL(*(client->GetPasswordFeatureManager()), GetDefaultPasswordStore)
      .WillOnce(Return(password_manager::PasswordForm::Store::kAccountStore));
  EXPECT_TRUE(delegate.IsAccountStoreDefault(web_contents.get()));

  EXPECT_CALL(*(client->GetPasswordFeatureManager()), GetDefaultPasswordStore)
      .WillOnce(Return(password_manager::PasswordForm::Store::kProfileStore));
  EXPECT_FALSE(delegate.IsAccountStoreDefault(web_contents.get()));
}

TEST_F(PasswordsPrivateDelegateImplTest, TestMovePasswordsToAccountStore) {
  // This enables uses of TestWebContents.
  content::RenderViewHostTestEnabler test_render_host_factories;
  std::unique_ptr<content::WebContents> web_contents =
      content::WebContentsTester::CreateTestWebContents(&profile_, nullptr);
  auto* client =
      MockPasswordManagerClient::CreateForWebContentsAndGet(web_contents.get());
  ON_CALL(*(client->GetPasswordFeatureManager()), IsOptedInForAccountStorage)
      .WillByDefault(Return(true));

  PasswordsPrivateDelegateImpl delegate(&profile_);

  password_manager::PasswordForm form1 =
      CreateSampleForm(password_manager::PasswordForm::Store::kProfileStore);
  password_manager::PasswordForm form2 = form1;
  form2.username_value = u"different_username";

  SetUpPasswordStores({form1, form2});

  int first_id =
      delegate.GetIdForCredential(password_manager::CredentialUIEntry(form1));
  int second_id =
      delegate.GetIdForCredential(password_manager::CredentialUIEntry(form2));

  delegate.MovePasswordsToAccount({first_id, second_id}, web_contents.get());
  base::RunLoop().RunUntilIdle();

  histogram_tester().ExpectUniqueSample(
      "PasswordManager.AccountStorage.MoveToAccountStoreFlowAccepted",
      password_manager::metrics_util::MoveToAccountStoreTrigger::
          kExplicitlyTriggeredInSettings,
      2);
}

TEST_F(PasswordsPrivateDelegateImplTest, AndroidCredential) {
  PasswordsPrivateDelegateImpl delegate(&profile_);

  password_manager::PasswordForm android_form;
  android_form.signon_realm = "android://hash@example.com";
  android_form.username_value = u"test@gmail.com";
  android_form.in_store = password_manager::PasswordForm::Store::kProfileStore;
  SetUpPasswordStores({android_form});

  base::MockCallback<PasswordsPrivateDelegate::UiEntriesCallback> callback;

  api::passwords_private::PasswordUiEntry expected_entry;
  expected_entry.urls.link =
      "https://play.google.com/store/apps/details?id=example.com";
  expected_entry.username = "test@gmail.com";
  expected_entry.password_note = "";
  expected_entry.is_android_credential = true;
  expected_entry.stored_in = api::passwords_private::PASSWORD_STORE_SET_DEVICE;
  EXPECT_CALL(callback, Run(testing::ElementsAre(PasswordUiEntryDataEquals(
                            testing::ByRef(expected_entry)))));
  delegate.GetSavedPasswordsList(callback.Get());
}

}  // namespace extensions
