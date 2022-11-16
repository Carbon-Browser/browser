// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

GEN_INCLUDE(['dictation_test_base.js']);

/** Dictation feature using accessibility common extension browser tests. */
DictationE2ETest = class extends DictationE2ETestBase {};

AX_TEST_F('DictationE2ETest', 'ResetsImeAfterToggleOff', async function() {
  // Set something as the active IME.
  this.mockInputMethodPrivate.setCurrentInputMethod('keyboard_cat');
  this.mockLanguageSettingsPrivate.addInputMethod('keyboard_cat');
  this.toggleDictationOn();
  this.toggleDictationOff();
  this.checkDictationImeInactive('keyboard_cat');
});

AX_TEST_F('DictationE2ETest', 'UpdateRecognitionProps', async function() {
  assertFalse(this.getSpeechRecognitionActive());
  assertEquals(undefined, this.getSpeechRecognitionLocale());
  assertEquals(undefined, this.getSpeechRecognitionInterimResults());

  const locale = await this.getPref(Dictation.DICTATION_LOCALE_PREF);
  this.updateSpeechRecognitionProperties(
      {locale: locale.value, interimResults: true});

  assertEquals(locale.value, this.getSpeechRecognitionLocale());
  assertTrue(this.getSpeechRecognitionInterimResults());
});

AX_TEST_F(
    'DictationE2ETest', 'UpdatesSpeechRecognitionLangOnLocaleChange',
    async function() {
      let locale = await this.getPref(Dictation.DICTATION_LOCALE_PREF);
      this.updateSpeechRecognitionProperties({locale: locale.value});
      assertEquals(locale.value, this.getSpeechRecognitionLocale());
      // Change the locale.
      await this.setPref(Dictation.DICTATION_LOCALE_PREF, 'es-ES');
      // Wait for the callbacks to Dictation.
      locale = await this.getPref(Dictation.DICTATION_LOCALE_PREF);
      this.updateSpeechRecognitionProperties({locale: locale.value});
      assertEquals('es-ES', this.getSpeechRecognitionLocale());
    });

AX_TEST_F('DictationE2ETest', 'StopsOnRecognitionError', async function() {
  this.toggleDictationOn();
  this.sendSpeechRecognitionErrorEvent();
  assertFalse(this.getDictationActive());
  assertFalse(this.getSpeechRecognitionActive());
});

AX_TEST_F('DictationE2ETest', 'StopsOnImeBlur', async function() {
  this.toggleDictationOn();
  this.blurInputContext();
  assertFalse(this.getSpeechRecognitionActive());
  assertFalse(this.getDictationActive());
  assertFalse(Boolean(this.mockInputIme.getLastCommittedParameters()));
});

AX_TEST_F('DictationE2ETest', 'CommitsFinalResults', async function() {
  this.toggleDictationOn();
  this.sendInterimSpeechResult('kittens');
  assertFalse(Boolean(this.mockInputIme.getLastCommittedParameters()));
  assertTrue(this.getDictationActive());

  this.mockInputIme.clearLastParameters();
  this.sendFinalSpeechResult('kittens!');
  await this.assertCommittedText('kittens!');
  assertTrue(this.getDictationActive());

  this.mockInputIme.clearLastParameters();
  this.sendFinalSpeechResult('puppies!');
  await this.assertCommittedText('puppies!');
  assertTrue(this.getDictationActive());

  this.mockInputIme.clearLastParameters();
  this.toggleDictationOff();
  assertFalse(this.getDictationActive());
  assertFalse(Boolean(this.mockInputIme.getLastCommittedParameters()));
});

AX_TEST_F(
    'DictationE2ETest', 'CommitsInterimResultsWhenRecognitionStops',
    async function() {
      this.toggleDictationOn();
      this.sendInterimSpeechResult('fish fly');
      this.sendSpeechRecognitionStopEvent();
      assertFalse(this.getDictationActive());
      await this.assertCommittedText('fish fly');
    });

AX_TEST_F(
    'DictationE2ETest', 'DoesNotCommitInterimResultsAfterImeBlur',
    async function() {
      this.toggleDictationOn();
      this.sendInterimSpeechResult('ducks dig');
      this.blurInputContext();
      assertFalse(this.getDictationActive());
      assertFalse(Boolean(this.mockInputIme.getLastCommittedParameters()));
    });

AX_TEST_F('DictationE2ETest', 'TimesOutWithNoImeContext', async function() {
  this.mockSetTimeoutMethod();
  this.toggleDictationOn();

  const callback =
      this.getCallbackWithDelay(Dictation.Timeouts.NO_FOCUSED_IME_MS);
  assertNotNullNorUndefined(callback);

  // Triggering the timeout should cause a request to toggle Dictation, but
  // nothing should be committed after AccessibilityPrivate toggle is received.
  callback();
  this.clearSetTimeoutData();
  assertFalse(this.getDictationActive());
  assertFalse(Boolean(this.mockInputIme.getLastCommittedParameters()));
});

AX_TEST_F('DictationE2ETest', 'TimesOutWithNoSpeech', async function() {
  this.mockSetTimeoutMethod();
  this.toggleDictationOn();

  const callback = this.getCallbackWithDelay(Dictation.Timeouts.NO_SPEECH_MS);
  assertNotNullNorUndefined(callback);

  // Triggering the timeout should cause a request to toggle Dictation, but
  // nothing should be committed after AccessibilityPrivate toggle is received.
  callback();
  this.clearSetTimeoutData();
  assertFalse(this.getDictationActive());
  assertFalse(Boolean(this.mockInputIme.getLastCommittedParameters()));
});

AX_TEST_F(
    'DictationE2ETest', 'TimesOutAfterInterimResultsAndCommits',
    async function() {
      this.mockSetTimeoutMethod();
      this.toggleDictationOn();
      this.sendInterimSpeechResult('sheep sleep');
      this.mockInputIme.clearLastParameters();

      // The timeout should be set based on the interim result.
      const callback =
          this.getCallbackWithDelay(Dictation.Timeouts.NO_NEW_SPEECH_MS);
      assertNotNullNorUndefined(callback);

      // Triggering the timeout should cause a request to toggle Dictation, and
      // after AccessibilityPrivate calls toggleDictation, to commit the interim
      // text.
      callback();
      this.clearSetTimeoutData();
      assertFalse(this.getDictationActive());
      await this.assertCommittedText('sheep sleep');
    });

AX_TEST_F('DictationE2ETest', 'TimesOutAfterFinalResults', async function() {
  this.mockSetTimeoutMethod();
  this.toggleDictationOn();
  this.sendFinalSpeechResult('bats bounce');
  await this.assertCommittedText('bats bounce');
  this.mockInputIme.clearLastParameters();

  // The timeout should be set based on the final result.
  const callback = this.getCallbackWithDelay(Dictation.Timeouts.NO_SPEECH_MS);

  // Triggering the timeout should stop listening.
  callback();
  this.clearSetTimeoutData();
  assertFalse(this.getDictationActive());
  assertFalse(Boolean(this.mockInputIme.getLastCommittedParameters()));
});

AX_TEST_F(
    'DictationE2ETest', 'CommandsDoNotCommitThemselves', async function() {
      this.toggleDictationOn();
      for (const command of Object.values(this.commandStrings)) {
        this.sendInterimSpeechResult(command);
        if (command !== this.commandStrings.LIST_COMMANDS) {
          // LIST_COMMANDS opens a new tab and ends Dictation. Skip this.
          this.sendFinalSpeechResult(command);
        }
        if (command === this.commandStrings.NEW_LINE) {
          await this.assertCommittedText('\n');
          this.mockInputIme.clearLastParameters();
        } else {
          // On final result, nothing is committed; instead, an action is taken.
          assertFalse(Boolean(this.mockInputIme.getLastCommittedParameters()));
        }

        // Try to type the command e.g. "type delete".
        // The command should be entered but not the word "type".
        this.sendFinalSpeechResult(`type ${command}`);
        await this.assertCommittedText(command);
        this.mockInputIme.clearLastParameters();
      }
    });

AX_TEST_F(
    'DictationE2ETest', 'TypePrefixWorksForNonCommands', async function() {
      this.toggleDictationOn();
      this.sendFinalSpeechResult('type this is a test');
      await this.assertCommittedText('this is a test');
    });

AX_TEST_F(
    'DictationE2ETest', 'DontCommitAfterMacroSuccess', async function() {
      this.toggleDictationOn();
      this.sendInterimSpeechResult('move to the next line');
      // Perform the next line command.
      this.sendFinalSpeechResult('move to the next line');
      // Wait for the UI to show macro success.
      await this.waitForUIProperties({
        visible: true,
        icon: this.iconType.MACRO_SUCCESS,
        text: this.commandStrings.NAV_NEXT_LINE,
      });
      this.toggleDictationOff();
      // No text should be committed.
      assertFalse(Boolean(this.mockInputIme.getLastCommittedParameters()));
    });


AX_TEST_F('DictationE2ETest', 'NoCommandsWhenNotSupported', async function() {
  this.toggleDictationOn();
  this.sendFinalSpeechResult('New line');
  await this.assertCommittedText('\n');
  this.mockInputIme.clearLastParameters();

  // System language is en-US. If the Dictation locale doesn't match,
  // commands should not work.
  await this.setPref(Dictation.DICTATION_LOCALE_PREF, 'es-ES');
  // Wait for the callbacks to Dictation.
  await this.getPref(Dictation.DICTATION_LOCALE_PREF);

  // Now this text should just get typed in instead of reinterpreted.
  this.sendFinalSpeechResult('New line');
  await this.assertCommittedText('New line');
  this.mockInputIme.clearLastParameters();
});
