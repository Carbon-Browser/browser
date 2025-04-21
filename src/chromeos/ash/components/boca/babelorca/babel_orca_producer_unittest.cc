// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/ash/components/boca/babelorca/babel_orca_producer.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/check_op.h"
#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "chromeos/ash/components/boca/babelorca/babel_orca_speech_recognizer.h"
#include "chromeos/ash/components/boca/babelorca/fakes/fake_tachyon_authed_client.h"
#include "chromeos/ash/components/boca/babelorca/fakes/fake_tachyon_request_data_provider.h"
#include "chromeos/ash/components/boca/babelorca/fakes/fake_token_manager.h"
#include "chromeos/ash/components/boca/babelorca/fakes/fake_translation_dispatcher.h"
#include "chromeos/ash/components/boca/babelorca/live_caption_controller_wrapper.h"
#include "chromeos/ash/components/boca/babelorca/proto/babel_orca_message.pb.h"
#include "chromeos/ash/components/boca/babelorca/proto/tachyon.pb.h"
#include "chromeos/ash/components/boca/babelorca/tachyon_request_data_provider.h"
#include "components/live_caption/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "media/mojo/mojom/speech_recognition.mojom.h"
#include "media/mojo/mojom/speech_recognition_result.h"
#include "services/network/public/cpp/data_element.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ash::babelorca {
namespace {

const std::string kLanguage = "en-US";
const std::string kTranslationTargetLanguage = "de-DE";

class MockSpeechRecognizer : public BabelOrcaSpeechRecognizer {
 public:
  MockSpeechRecognizer() = default;
  ~MockSpeechRecognizer() override = default;
  MOCK_METHOD(void, Start, (), (override));
  MOCK_METHOD(void, Stop, (), (override));
  MOCK_METHOD(void,
              ObserveSpeechRecognition,
              (TranscriptionResultCallback,
               LanguageIdentificationEventCallback),
              (override));
  MOCK_METHOD(void, RemoveSpeechRecognitionObservation, (), (override));
};

class MockLiveCaptionControllerWrapper : public LiveCaptionControllerWrapper {
 public:
  MockLiveCaptionControllerWrapper() = default;
  ~MockLiveCaptionControllerWrapper() override = default;
  MOCK_METHOD(bool,
              DispatchTranscription,
              (const media::SpeechRecognitionResult&),
              (override));
  MOCK_METHOD(void,
              OnLanguageIdentificationEvent,
              (const media::mojom::LanguageIdentificationEventPtr&),
              (override));
  MOCK_METHOD(void, ToggleLiveCaptionForBabelOrca, (bool), (override));
  MOCK_METHOD(void, OnAudioStreamEnd, (), (override));
  MOCK_METHOD(void, RestartCaptions, (), (override));
};

void RegisterStringPrefs(TestingPrefServiceSimple* pref_service) {
  pref_service->registry()->RegisterStringPref(
      prefs::kUserMicrophoneCaptionLanguageCode, kLanguage);
  // For most tests we aren't testing translations, in the translation
  // test specifically we change this value to the kTranslationTargetLanguage.
  pref_service->registry()->RegisterStringPref(
      prefs::kLiveTranslateTargetLanguageCode, kLanguage);
}

class BabelOrcaProducerTest : public testing::Test {
 protected:
  using TranscriptionResultCallback =
      BabelOrcaSpeechRecognizer::TranscriptionResultCallback;

  void SetUp() override {
    RegisterStringPrefs(&pref_service_);
    speech_recognizer_ =
        std::make_unique<testing::NiceMock<MockSpeechRecognizer>>();
    caption_controller_wrapper_ =
        std::make_unique<testing::NiceMock<MockLiveCaptionControllerWrapper>>();
    authed_client_ = std::make_unique<FakeTachyonAuthedClient>();

    auto fake_translation_dispatcher =
        std::make_unique<FakeBabelOrcaTranslationDispatcher>();
    translation_dispatcher_ = fake_translation_dispatcher->GetWeakPtr();

    translator_ = std::make_unique<BabelOrcaCaptionTranslator>(
        std::move(fake_translation_dispatcher));
  }

  media::SpeechRecognitionResult GetTranscriptFromRequest(
      const std::string& request) {
    InboxSendRequest send_request;
    CHECK(send_request.ParseFromString(request));
    BabelOrcaMessage message;
    CHECK(message.ParseFromString(send_request.message().message()));
    return media::SpeechRecognitionResult(
        message.current_transcript().text(),
        message.current_transcript().is_final());
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<MockSpeechRecognizer> speech_recognizer_;
  std::unique_ptr<MockLiveCaptionControllerWrapper> caption_controller_wrapper_;
  std::unique_ptr<FakeTachyonAuthedClient> authed_client_;
  FakeTachyonRequestDataProvider request_data_provider_;
  base::WeakPtr<FakeBabelOrcaTranslationDispatcher> translation_dispatcher_;
  std::unique_ptr<BabelOrcaCaptionTranslator> translator_;
  TestingPrefServiceSimple pref_service_;
};

TEST_F(BabelOrcaProducerTest, EnableLocalCaptionsOutOfSession) {
  media::SpeechRecognitionResult transcript("transcript", /*is_final=*/true);
  MockSpeechRecognizer* speech_recognizer_ptr = speech_recognizer_.get();
  MockLiveCaptionControllerWrapper* caption_controller_wrapper_ptr =
      caption_controller_wrapper_.get();
  TranscriptionResultCallback transcript_cb;
  BabelOrcaProducer producer(
      url_loader_factory_.GetSafeWeakWrapper(), std::move(speech_recognizer_),
      std::move(caption_controller_wrapper_), std::move(authed_client_),
      &request_data_provider_, std::move(translator_), &pref_service_);

  EXPECT_CALL(*caption_controller_wrapper_ptr,
              ToggleLiveCaptionForBabelOrca(true))
      .Times(1);
  EXPECT_CALL(*speech_recognizer_ptr, ObserveSpeechRecognition)
      .WillOnce(
          [&transcript_cb](
              TranscriptionResultCallback transcript_cb_param,
              BabelOrcaSpeechRecognizer::LanguageIdentificationEventCallback
                  language_id_cb_param) {
            transcript_cb = std::move(transcript_cb_param);
          });
  EXPECT_CALL(*speech_recognizer_ptr, Start).Times(1);
  producer.OnLocalCaptionConfigUpdated(/*local_captions_enabled=*/true);

  ASSERT_TRUE(transcript_cb);
  EXPECT_CALL(*caption_controller_wrapper_ptr,
              DispatchTranscription(transcript))
      .WillOnce(testing::Return(true));
  transcript_cb.Run(transcript, kLanguage);

  EXPECT_CALL(*caption_controller_wrapper_ptr,
              ToggleLiveCaptionForBabelOrca(false))
      .Times(1);
  EXPECT_CALL(*speech_recognizer_ptr, RemoveSpeechRecognitionObservation)
      .Times(1);
  EXPECT_CALL(*speech_recognizer_ptr, Stop).Times(1);
  EXPECT_CALL(*caption_controller_wrapper_ptr, OnAudioStreamEnd).Times(1);
  producer.OnLocalCaptionConfigUpdated(/*local_captions_enabled=*/false);

  // Stop recognition methods are called on`producer` destruction as a safe
  // guard in case the object was destroyed before stopping recognition.
  EXPECT_CALL(*speech_recognizer_ptr, RemoveSpeechRecognitionObservation)
      .Times(1);
  EXPECT_CALL(*speech_recognizer_ptr, Stop).Times(1);
  EXPECT_CALL(*caption_controller_wrapper_ptr, OnAudioStreamEnd).Times(1);
}

TEST_F(BabelOrcaProducerTest, EnableSessionCaptionsOutOfSession) {
  MockSpeechRecognizer* speech_recognizer_ptr = speech_recognizer_.get();
  TranscriptionResultCallback transcript_cb;
  BabelOrcaProducer producer(
      url_loader_factory_.GetSafeWeakWrapper(), std::move(speech_recognizer_),
      std::move(caption_controller_wrapper_), std::move(authed_client_),
      &request_data_provider_, std::move(translator_), &pref_service_);

  EXPECT_CALL(*speech_recognizer_ptr, ObserveSpeechRecognition).Times(0);
  EXPECT_CALL(*speech_recognizer_ptr, Start).Times(0);
  producer.OnSessionCaptionConfigUpdated(/*session_captions_enabled=*/true,
                                         /*translations_enabled=*/false);
}

TEST_F(BabelOrcaProducerTest, EnableSessionCaptionsThenLocalCaptionsInSession) {
  media::SpeechRecognitionResult transcript1("transcript1", /*is_final=*/true);
  media::SpeechRecognitionResult transcript2("transcript2", /*is_final=*/true);
  FakeTachyonRequestDataProvider data_provider("session-id",
                                               /*tachyon_token=*/std::nullopt,
                                               "group-id", "sender@email.com");
  MockSpeechRecognizer* speech_recognizer_ptr = speech_recognizer_.get();
  MockLiveCaptionControllerWrapper* caption_controller_wrapper_ptr =
      caption_controller_wrapper_.get();
  FakeTachyonAuthedClient* authed_client_ptr = authed_client_.get();
  TranscriptionResultCallback transcript_cb;
  BabelOrcaProducer producer(
      url_loader_factory_.GetSafeWeakWrapper(), std::move(speech_recognizer_),
      std::move(caption_controller_wrapper_), std::move(authed_client_),
      &data_provider, std::move(translator_), &pref_service_);

  producer.OnSessionStarted();
  producer.OnSessionCaptionConfigUpdated(/*session_captions_enabled=*/true,
                                         /*translations_enabled=*/false);
  base::OnceCallback<void(bool)> signin_cb = data_provider.TakeSigninCb();
  ASSERT_FALSE(signin_cb.is_null());
  EXPECT_CALL(*speech_recognizer_ptr, ObserveSpeechRecognition)
      .WillOnce(
          [&transcript_cb](
              TranscriptionResultCallback transcript_cb_param,
              BabelOrcaSpeechRecognizer::LanguageIdentificationEventCallback
                  language_id_cb_param) {
            transcript_cb = std::move(transcript_cb_param);
          });
  EXPECT_CALL(*speech_recognizer_ptr, Start).Times(1);
  data_provider.set_tachyon_token("tachyon_token");
  std::move(signin_cb).Run(true);

  ASSERT_TRUE(transcript_cb);
  // Local captions not enabled.
  EXPECT_CALL(*caption_controller_wrapper_ptr,
              DispatchTranscription(testing::_))
      .Times(0);
  transcript_cb.Run(transcript1, kLanguage);
  authed_client_ptr->WaitForRequest();
  media::SpeechRecognitionResult sent_transcript1 =
      GetTranscriptFromRequest(authed_client_ptr->GetRequestString());
  EXPECT_EQ(sent_transcript1, transcript1);

  EXPECT_CALL(*caption_controller_wrapper_ptr,
              ToggleLiveCaptionForBabelOrca(true))
      .Times(1);
  producer.OnLocalCaptionConfigUpdated(/*local_captions_enabled=*/true);
  EXPECT_CALL(*caption_controller_wrapper_ptr,
              DispatchTranscription(transcript2))
      .WillOnce(testing::Return(true));
  transcript_cb.Run(transcript2, kLanguage);
  authed_client_ptr->WaitForRequest();
  media::SpeechRecognitionResult sent_transcript2 =
      GetTranscriptFromRequest(authed_client_ptr->GetRequestString());
  EXPECT_EQ(sent_transcript2, transcript2);

  EXPECT_CALL(*caption_controller_wrapper_ptr,
              ToggleLiveCaptionForBabelOrca(false))
      .Times(1);
  producer.OnLocalCaptionConfigUpdated(/*local_captions_enabled=*/false);
  // 2 Times, one on enabled set to false and one on destruction.
  EXPECT_CALL(*speech_recognizer_ptr, RemoveSpeechRecognitionObservation)
      .Times(2);
  EXPECT_CALL(*speech_recognizer_ptr, Stop).Times(2);
  EXPECT_CALL(*caption_controller_wrapper_ptr, OnAudioStreamEnd).Times(2);
  producer.OnSessionCaptionConfigUpdated(/*session_captions_enabled=*/false,
                                         /*translations_enabled=*/false);
}

TEST_F(BabelOrcaProducerTest, EnableLocalCaptionsThenSessionCaptionsInSession) {
  media::SpeechRecognitionResult transcript1("transcript1", /*is_final=*/true);
  media::SpeechRecognitionResult transcript2("transcript2", /*is_final=*/true);
  FakeTachyonRequestDataProvider data_provider("session-id",
                                               /*tachyon_token=*/std::nullopt,
                                               "group-id", "sender@email.com");
  MockSpeechRecognizer* speech_recognizer_ptr = speech_recognizer_.get();
  MockLiveCaptionControllerWrapper* caption_controller_wrapper_ptr =
      caption_controller_wrapper_.get();
  FakeTachyonAuthedClient* authed_client_ptr = authed_client_.get();
  TranscriptionResultCallback transcript_cb;
  BabelOrcaProducer producer(
      url_loader_factory_.GetSafeWeakWrapper(), std::move(speech_recognizer_),
      std::move(caption_controller_wrapper_), std::move(authed_client_),
      &data_provider, std::move(translator_), &pref_service_);

  producer.OnSessionStarted();

  EXPECT_CALL(*caption_controller_wrapper_ptr,
              ToggleLiveCaptionForBabelOrca(true))
      .Times(1);
  EXPECT_CALL(*speech_recognizer_ptr, ObserveSpeechRecognition)
      .WillOnce(
          [&transcript_cb](
              TranscriptionResultCallback transcript_cb_param,
              BabelOrcaSpeechRecognizer::LanguageIdentificationEventCallback
                  langauge_id_cb_param) {
            transcript_cb = std::move(transcript_cb_param);
          });
  EXPECT_CALL(*speech_recognizer_ptr, Start).Times(1);
  producer.OnLocalCaptionConfigUpdated(/*local_captions_enabled=*/true);

  ASSERT_TRUE(transcript_cb);
  EXPECT_CALL(*caption_controller_wrapper_ptr,
              DispatchTranscription(transcript1))
      .WillOnce(testing::Return(true));
  transcript_cb.Run(transcript1, kLanguage);

  producer.OnSessionCaptionConfigUpdated(/*session_captions_enabled=*/true,
                                         /*translations_enabled=*/false);
  base::OnceCallback<void(bool)> signin_cb = data_provider.TakeSigninCb();
  ASSERT_FALSE(signin_cb.is_null());
  data_provider.set_tachyon_token("tachyon_token");
  std::move(signin_cb).Run(true);
  EXPECT_CALL(*caption_controller_wrapper_ptr,
              DispatchTranscription(transcript2))
      .WillOnce(testing::Return(true));
  transcript_cb.Run(transcript2, kLanguage);
  authed_client_ptr->WaitForRequest();
  media::SpeechRecognitionResult sent_transcript2 =
      GetTranscriptFromRequest(authed_client_ptr->GetRequestString());
  EXPECT_EQ(sent_transcript2, transcript2);

  producer.OnSessionCaptionConfigUpdated(/*session_captions_enabled=*/false,
                                         /*translations_enabled=*/false);

  EXPECT_CALL(*caption_controller_wrapper_ptr,
              ToggleLiveCaptionForBabelOrca(false))
      .Times(1);
  // 2 Times, one on enabled set to false and one on destruction.
  EXPECT_CALL(*speech_recognizer_ptr, RemoveSpeechRecognitionObservation)
      .Times(2);
  EXPECT_CALL(*speech_recognizer_ptr, Stop).Times(2);
  EXPECT_CALL(*caption_controller_wrapper_ptr, OnAudioStreamEnd).Times(2);
  producer.OnLocalCaptionConfigUpdated(/*local_captions_enabled=*/false);
}

TEST_F(BabelOrcaProducerTest, NoSigninIfTachyonTokenIsSet) {
  FakeTachyonRequestDataProvider data_provider("session-id", "tachyon_token",
                                               "group-id", "sender@email.com");
  MockSpeechRecognizer* speech_recognizer_ptr = speech_recognizer_.get();
  TranscriptionResultCallback transcript_cb;
  BabelOrcaProducer producer(
      url_loader_factory_.GetSafeWeakWrapper(), std::move(speech_recognizer_),
      std::move(caption_controller_wrapper_), std::move(authed_client_),
      &data_provider, std::move(translator_), &pref_service_);

  producer.OnSessionStarted();

  EXPECT_CALL(*speech_recognizer_ptr, ObserveSpeechRecognition).Times(1);
  EXPECT_CALL(*speech_recognizer_ptr, Start).Times(1);
  producer.OnSessionCaptionConfigUpdated(/*session_captions_enabled=*/true,
                                         /*translations_enabled=*/false);

  base::OnceCallback<void(bool)> signin_cb = data_provider.TakeSigninCb();
  ASSERT_TRUE(signin_cb.is_null());
}

TEST_F(BabelOrcaProducerTest, FailedSignWillNotStartCaptions) {
  FakeTachyonRequestDataProvider data_provider("session-id",
                                               /*tachyon_token=*/std::nullopt,
                                               "group-id", "sender@email.com");
  MockSpeechRecognizer* speech_recognizer_ptr = speech_recognizer_.get();
  TranscriptionResultCallback transcript_cb;
  BabelOrcaProducer producer(
      url_loader_factory_.GetSafeWeakWrapper(), std::move(speech_recognizer_),
      std::move(caption_controller_wrapper_), std::move(authed_client_),
      &data_provider, std::move(translator_), &pref_service_);

  producer.OnSessionStarted();
  producer.OnSessionCaptionConfigUpdated(/*session_captions_enabled=*/true,
                                         /*translations_enabled=*/false);

  EXPECT_CALL(*speech_recognizer_ptr, ObserveSpeechRecognition).Times(0);
  EXPECT_CALL(*speech_recognizer_ptr, Start).Times(0);
  base::OnceCallback<void(bool)> signin_cb = data_provider.TakeSigninCb();
  ASSERT_FALSE(signin_cb.is_null());
  std::move(signin_cb).Run(false);
}

TEST_F(BabelOrcaProducerTest, DisableSessionCaptionWhileSigninInFlight) {
  FakeTachyonRequestDataProvider data_provider("session-id",
                                               /*tachyon_token=*/std::nullopt,
                                               "group-id", "sender@email.com");
  MockSpeechRecognizer* speech_recognizer_ptr = speech_recognizer_.get();
  TranscriptionResultCallback transcript_cb;
  BabelOrcaProducer producer(
      url_loader_factory_.GetSafeWeakWrapper(), std::move(speech_recognizer_),
      std::move(caption_controller_wrapper_), std::move(authed_client_),
      &data_provider, std::move(translator_), &pref_service_);

  producer.OnSessionStarted();
  producer.OnSessionCaptionConfigUpdated(/*session_captions_enabled=*/true,
                                         /*translations_enabled=*/false);

  producer.OnSessionCaptionConfigUpdated(/*session_captions_enabled=*/false,
                                         /*translations_enabled=*/false);
  EXPECT_CALL(*speech_recognizer_ptr, ObserveSpeechRecognition).Times(0);
  EXPECT_CALL(*speech_recognizer_ptr, Start).Times(0);
  base::OnceCallback<void(bool)> signin_cb = data_provider.TakeSigninCb();
  ASSERT_FALSE(signin_cb.is_null());
  data_provider.set_tachyon_token("tachyon_token");
  std::move(signin_cb).Run(true);
}

TEST_F(BabelOrcaProducerTest, SessionEndedWhileSigninInFlight) {
  FakeTachyonRequestDataProvider data_provider("session-id",
                                               /*tachyon_token=*/std::nullopt,
                                               "group-id", "sender@email.com");
  MockSpeechRecognizer* speech_recognizer_ptr = speech_recognizer_.get();
  TranscriptionResultCallback transcript_cb;
  BabelOrcaProducer producer(
      url_loader_factory_.GetSafeWeakWrapper(), std::move(speech_recognizer_),
      std::move(caption_controller_wrapper_), std::move(authed_client_),
      &data_provider, std::move(translator_), &pref_service_);

  producer.OnSessionStarted();
  producer.OnSessionCaptionConfigUpdated(/*session_captions_enabled=*/true,
                                         /*translations_enabled=*/false);

  producer.OnSessionEnded();
  EXPECT_CALL(*speech_recognizer_ptr, ObserveSpeechRecognition).Times(0);
  EXPECT_CALL(*speech_recognizer_ptr, Start).Times(0);
  base::OnceCallback<void(bool)> signin_cb = data_provider.TakeSigninCb();
  ASSERT_FALSE(signin_cb.is_null());
  data_provider.set_tachyon_token("tachyon_token");
  std::move(signin_cb).Run(true);
}

TEST_F(BabelOrcaProducerTest, SessionEndLocalCaptionsDisabled) {
  FakeTachyonRequestDataProvider data_provider("session-id", "tachyon_token",
                                               "group-id", "sender@email.com");
  MockSpeechRecognizer* speech_recognizer_ptr = speech_recognizer_.get();
  MockLiveCaptionControllerWrapper* caption_controller_wrapper_ptr =
      caption_controller_wrapper_.get();
  TranscriptionResultCallback transcript_cb;
  BabelOrcaProducer producer(
      url_loader_factory_.GetSafeWeakWrapper(), std::move(speech_recognizer_),
      std::move(caption_controller_wrapper_), std::move(authed_client_),
      &data_provider, std::move(translator_), &pref_service_);

  producer.OnSessionStarted();
  EXPECT_CALL(*speech_recognizer_ptr, ObserveSpeechRecognition).Times(1);
  EXPECT_CALL(*speech_recognizer_ptr, Start).Times(1);
  producer.OnSessionCaptionConfigUpdated(/*session_captions_enabled=*/true,
                                         /*translations_enabled=*/false);

  // 2 Times, one on `OnSessionEnded` and one on destruction.
  EXPECT_CALL(*speech_recognizer_ptr, RemoveSpeechRecognitionObservation)
      .Times(2);
  EXPECT_CALL(*speech_recognizer_ptr, Stop).Times(2);
  EXPECT_CALL(*caption_controller_wrapper_ptr, OnAudioStreamEnd).Times(2);
  producer.OnSessionEnded();
}

TEST_F(BabelOrcaProducerTest, SessionEndLocalCaptionsEnabled) {
  FakeTachyonRequestDataProvider data_provider("session-id", "tachyon_token",
                                               "group-id", "sender@email.com");
  MockSpeechRecognizer* speech_recognizer_ptr = speech_recognizer_.get();
  MockLiveCaptionControllerWrapper* caption_controller_wrapper_ptr =
      caption_controller_wrapper_.get();
  TranscriptionResultCallback transcript_cb;
  BabelOrcaProducer producer(
      url_loader_factory_.GetSafeWeakWrapper(), std::move(speech_recognizer_),
      std::move(caption_controller_wrapper_), std::move(authed_client_),
      &data_provider, std::move(translator_), &pref_service_);

  producer.OnSessionStarted();
  producer.OnLocalCaptionConfigUpdated(/*local_captions_enabled=*/true);

  EXPECT_CALL(*speech_recognizer_ptr, RemoveSpeechRecognitionObservation)
      .Times(0);
  EXPECT_CALL(*speech_recognizer_ptr, Stop).Times(0);
  EXPECT_CALL(*caption_controller_wrapper_ptr, OnAudioStreamEnd).Times(0);
  producer.OnSessionEnded();

  // Stop recognition on destruction.
  EXPECT_CALL(*speech_recognizer_ptr, RemoveSpeechRecognitionObservation)
      .Times(1);
  EXPECT_CALL(*speech_recognizer_ptr, Stop).Times(1);
  EXPECT_CALL(*caption_controller_wrapper_ptr, OnAudioStreamEnd).Times(1);
}

TEST_F(BabelOrcaProducerTest, DisableLocalWhileSessionCaptionsEnabled) {
  media::SpeechRecognitionResult transcript("transcript", /*is_final=*/true);
  FakeTachyonRequestDataProvider data_provider("session-id", "tachyon-token",
                                               "group-id", "sender@email.com");
  MockSpeechRecognizer* speech_recognizer_ptr = speech_recognizer_.get();
  MockLiveCaptionControllerWrapper* caption_controller_wrapper_ptr =
      caption_controller_wrapper_.get();
  FakeTachyonAuthedClient* authed_client_ptr = authed_client_.get();
  TranscriptionResultCallback transcript_cb;
  BabelOrcaProducer producer(
      url_loader_factory_.GetSafeWeakWrapper(), std::move(speech_recognizer_),
      std::move(caption_controller_wrapper_), std::move(authed_client_),
      &data_provider, std::move(translator_), &pref_service_);

  producer.OnSessionStarted();

  EXPECT_CALL(*caption_controller_wrapper_ptr,
              ToggleLiveCaptionForBabelOrca(true))
      .Times(1);
  EXPECT_CALL(*speech_recognizer_ptr, ObserveSpeechRecognition)
      .WillOnce(
          [&transcript_cb](
              TranscriptionResultCallback transcript_cb_param,
              BabelOrcaSpeechRecognizer::LanguageIdentificationEventCallback
                  language_id_cb_param) {
            transcript_cb = std::move(transcript_cb_param);
          });
  EXPECT_CALL(*speech_recognizer_ptr, Start).Times(1);
  producer.OnLocalCaptionConfigUpdated(/*local_captions_enabled=*/true);
  producer.OnSessionCaptionConfigUpdated(/*session_captions_enabled=*/true,
                                         /*translations_enabled=*/false);

  EXPECT_CALL(*caption_controller_wrapper_ptr,
              ToggleLiveCaptionForBabelOrca(false))
      .Times(1);
  EXPECT_CALL(*caption_controller_wrapper_ptr, OnAudioStreamEnd).Times(1);
  producer.OnLocalCaptionConfigUpdated(/*local_captions_enabled=*/false);

  ASSERT_TRUE(transcript_cb);
  // Verify that transcription is not dispatched to the bubble.
  EXPECT_CALL(*caption_controller_wrapper_ptr,
              DispatchTranscription(transcript))
      .Times(0);
  transcript_cb.Run(transcript, kLanguage);
  authed_client_ptr->WaitForRequest();
  media::SpeechRecognitionResult sent_transcript =
      GetTranscriptFromRequest(authed_client_ptr->GetRequestString());
  EXPECT_EQ(sent_transcript, transcript);

  // Called on destruction.
  EXPECT_CALL(*caption_controller_wrapper_ptr, OnAudioStreamEnd).Times(1);
}

TEST_F(BabelOrcaProducerTest, RestartCaptionsIfDispatchFailed) {
  media::SpeechRecognitionResult transcript("transcript", /*is_final=*/true);
  MockLiveCaptionControllerWrapper* caption_controller_wrapper_ptr =
      caption_controller_wrapper_.get();
  MockSpeechRecognizer* speech_recognizer_ptr = speech_recognizer_.get();
  TranscriptionResultCallback transcript_cb;
  BabelOrcaProducer producer(
      url_loader_factory_.GetSafeWeakWrapper(), std::move(speech_recognizer_),
      std::move(caption_controller_wrapper_), std::move(authed_client_),
      &request_data_provider_, std::move(translator_), &pref_service_);

  EXPECT_CALL(*speech_recognizer_ptr, ObserveSpeechRecognition)
      .WillOnce(
          [&transcript_cb](
              TranscriptionResultCallback transcript_cb_param,
              BabelOrcaSpeechRecognizer::LanguageIdentificationEventCallback
                  language_id_cb_param) {
            transcript_cb = std::move(transcript_cb_param);
          });
  producer.OnLocalCaptionConfigUpdated(/*local_captions_enabled=*/true);

  ASSERT_TRUE(transcript_cb);
  EXPECT_CALL(*caption_controller_wrapper_ptr,
              DispatchTranscription(transcript))
      .WillOnce(testing::Return(false))
      .WillOnce(testing::Return(true));
  EXPECT_CALL(*caption_controller_wrapper_ptr, RestartCaptions).Times(1);
  transcript_cb.Run(transcript, kLanguage);
}

TEST_F(BabelOrcaProducerTest, EnableTranslations) {
  media::SpeechRecognitionResult transcript1("transcript1", /*is_final=*/true);
  media::SpeechRecognitionResult transcript2("transcript3", /*is_final=*/true);
  std::string translated_transcript_string = "translated_transcript";
  media::SpeechRecognitionResult translated_transcript(
      translated_transcript_string, /*is_final=*/true);
  FakeTachyonRequestDataProvider data_provider("session-id",
                                               /*tachyon_token=*/std::nullopt,
                                               "group-id", "sender@email.com");
  MockSpeechRecognizer* speech_recognizer_ptr = speech_recognizer_.get();
  MockLiveCaptionControllerWrapper* caption_controller_wrapper_ptr =
      caption_controller_wrapper_.get();
  TranscriptionResultCallback transcript_cb;
  BabelOrcaProducer producer(
      url_loader_factory_.GetSafeWeakWrapper(), std::move(speech_recognizer_),
      std::move(caption_controller_wrapper_), std::move(authed_client_),
      &data_provider, std::move(translator_), &pref_service_);

  producer.OnSessionStarted();

  EXPECT_CALL(*caption_controller_wrapper_ptr,
              ToggleLiveCaptionForBabelOrca(true))
      .Times(1);
  EXPECT_CALL(*speech_recognizer_ptr, ObserveSpeechRecognition)
      .WillOnce(
          [&transcript_cb](
              TranscriptionResultCallback transcript_cb_param,
              BabelOrcaSpeechRecognizer::LanguageIdentificationEventCallback
                  language_id_cb_param) {
            transcript_cb = std::move(transcript_cb_param);
          });
  EXPECT_CALL(*speech_recognizer_ptr, Start).Times(1);
  translation_dispatcher_->InjectTranslationResult(
      translated_transcript_string);
  producer.OnLocalCaptionConfigUpdated(/*local_captions_enabled=*/true);

  ASSERT_TRUE(transcript_cb);
  EXPECT_CALL(*caption_controller_wrapper_ptr,
              DispatchTranscription(transcript1))
      .WillOnce(testing::Return(true));
  transcript_cb.Run(transcript1, kLanguage);

  // Now we set a target language that is distinct from the source
  // language to ensure that we translate when relevant.
  pref_service_.SetString(prefs::kLiveTranslateTargetLanguageCode,
                          kTranslationTargetLanguage);

  EXPECT_CALL(*caption_controller_wrapper_ptr,
              DispatchTranscription(translated_transcript))
      .WillOnce(testing::Return(true));
  transcript_cb.Run(transcript2, kLanguage);
}

TEST_F(BabelOrcaProducerTest, TranslationsDontAffectSentTranscripts) {
  media::SpeechRecognitionResult transcript("transcript1", /*is_final=*/true);
  std::string translated_transcript_string = "translated_transcript";
  media::SpeechRecognitionResult translated_transcript(
      translated_transcript_string, /*is_final=*/true);
  FakeTachyonRequestDataProvider data_provider("session-id",
                                               /*tachyon_token=*/std::nullopt,
                                               "group-id", "sender@email.com");
  MockSpeechRecognizer* speech_recognizer_ptr = speech_recognizer_.get();
  MockLiveCaptionControllerWrapper* caption_controller_wrapper_ptr =
      caption_controller_wrapper_.get();
  TranscriptionResultCallback transcript_cb;
  FakeTachyonAuthedClient* authed_client_ptr = authed_client_.get();
  BabelOrcaProducer producer(
      url_loader_factory_.GetSafeWeakWrapper(), std::move(speech_recognizer_),
      std::move(caption_controller_wrapper_), std::move(authed_client_),
      &data_provider, std::move(translator_), &pref_service_);

  producer.OnSessionStarted();

  EXPECT_CALL(*caption_controller_wrapper_ptr,
              ToggleLiveCaptionForBabelOrca(true))
      .Times(1);
  EXPECT_CALL(*speech_recognizer_ptr, ObserveSpeechRecognition)
      .WillOnce(
          [&transcript_cb](
              TranscriptionResultCallback transcript_cb_param,
              BabelOrcaSpeechRecognizer::LanguageIdentificationEventCallback
                  language_id_cb_param) {
            transcript_cb = std::move(transcript_cb_param);
          });
  EXPECT_CALL(*speech_recognizer_ptr, Start).Times(1);
  pref_service_.SetString(prefs::kLiveTranslateTargetLanguageCode,
                          kTranslationTargetLanguage);
  translation_dispatcher_->InjectTranslationResult(
      translated_transcript_string);
  producer.OnLocalCaptionConfigUpdated(/*local_captions_enabled=*/true);
  // Session translations are only relevant for consumers.  Translations
  // on the producer is controlled by the TranslationTargetLanguage
  // preference.
  producer.OnSessionCaptionConfigUpdated(/*session_captions_enabled=*/true,
                                         /*translations_enabled=*/false);
  base::OnceCallback<void(bool)> signin_cb = data_provider.TakeSigninCb();
  ASSERT_FALSE(signin_cb.is_null());
  data_provider.set_tachyon_token("tachyon_token");
  std::move(signin_cb).Run(true);

  ASSERT_TRUE(transcript_cb);
  EXPECT_CALL(*caption_controller_wrapper_ptr,
              DispatchTranscription(translated_transcript))
      .WillOnce(testing::Return(true));
  transcript_cb.Run(transcript, kLanguage);
  authed_client_ptr->WaitForRequest();
  media::SpeechRecognitionResult sent_transcript =
      GetTranscriptFromRequest(authed_client_ptr->GetRequestString());
  // Something has gone wrong if we got the translated string here.
  EXPECT_EQ(transcript, sent_transcript);
}

TEST_F(BabelOrcaProducerTest,
       SourceLanguageSwitchTriggersOnLanguageIdentificationEvent) {
  FakeTachyonRequestDataProvider data_provider("session-id",
                                               /*tachyon_token=*/std::nullopt,
                                               "group-id", "sender@email.com");
  MockSpeechRecognizer* speech_recognizer_ptr = speech_recognizer_.get();
  MockLiveCaptionControllerWrapper* caption_controller_wrapper_ptr =
      caption_controller_wrapper_.get();
  BabelOrcaSpeechRecognizer::LanguageIdentificationEventCallback language_id_cb;
  BabelOrcaProducer producer(
      url_loader_factory_.GetSafeWeakWrapper(), std::move(speech_recognizer_),
      std::move(caption_controller_wrapper_), std::move(authed_client_),
      &data_provider, std::move(translator_), &pref_service_);

  // Values here are arbitrary, we're just testing that the producer forwards
  // this object to the caption controller.
  media::mojom::LanguageIdentificationEventPtr language_id_event =
      media::mojom::LanguageIdentificationEvent::New(
          kTranslationTargetLanguage,
          media::mojom::ConfidenceLevel::kDefaultValue);
  language_id_event->asr_switch_result =
      media::mojom::AsrSwitchResult::kSwitchSucceeded;

  producer.OnSessionStarted();
  producer.OnSessionCaptionConfigUpdated(/*session_captions_enabled=*/true,
                                         /*translations_enabled=*/false);
  base::OnceCallback<void(bool)> signin_cb = data_provider.TakeSigninCb();
  ASSERT_FALSE(signin_cb.is_null());
  EXPECT_CALL(*speech_recognizer_ptr, ObserveSpeechRecognition)
      .WillOnce(
          [&language_id_cb](
              TranscriptionResultCallback transcript_cb_param,
              BabelOrcaSpeechRecognizer::LanguageIdentificationEventCallback
                  language_id_cb_param) {
            language_id_cb = std::move(language_id_cb_param);
          });
  EXPECT_CALL(*speech_recognizer_ptr, Start).Times(1);
  data_provider.set_tachyon_token("tachyon_token");
  std::move(signin_cb).Run(true);

  ASSERT_TRUE(language_id_cb);
  EXPECT_CALL(*caption_controller_wrapper_ptr, OnLanguageIdentificationEvent)
      .Times(1);
  language_id_cb.Run(std::move(language_id_event));
}

}  // namespace
}  // namespace ash::babelorca
