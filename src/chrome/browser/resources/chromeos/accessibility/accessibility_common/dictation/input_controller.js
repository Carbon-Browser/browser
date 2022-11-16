// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {EditingUtil} from './editing_util.js';
import {FocusHandler} from './focus_handler.js';
import {LocaleInfo} from './locale_info.js';

const AutomationNode = chrome.automation.AutomationNode;
const EventType = chrome.automation.EventType;

/** InputController handles interaction with input fields for Dictation. */
export class InputController {
  constructor(stopDictationCallback, focusHandler) {
    /** @private {number} */
    this.activeImeContextId_ = InputController.NO_ACTIVE_IME_CONTEXT_ID_;

    /** @private {!FocusHandler} */
    this.focusHandler_ = focusHandler;

    /**
     * The engine ID of the previously active IME input method. Used to
     * restore the previous IME after Dictation is deactivated.
     * @private {string}
     */
    this.previousImeEngineId_ = '';

    /** @private {function():void} */
    this.stopDictationCallback_ = stopDictationCallback;

    /** @private {?function():void} */
    this.onConnectCallback_ = null;

    this.initialize_();
  }

  /**
   * Sets up Dictation's various IME listeners.
   * @private
   */
  initialize_() {
    // Listen for IME focus changes.
    chrome.input.ime.onFocus.addListener(context => this.onImeFocus_(context));
    chrome.input.ime.onBlur.addListener(
        contextId => this.onImeBlur_(contextId));
  }

  /**
   * Whether this is the active IME and has a focused input.
   * @return {boolean}
   */
  isActive() {
    return this.activeImeContextId_ !==
        InputController.NO_ACTIVE_IME_CONTEXT_ID_;
  }

  /**
   * Connect as the active Input Method Manager.
   * @param {function():void} callback The callback to run after IME is
   *     connected.
   */
  connect(callback) {
    this.onConnectCallback_ = callback;
    chrome.inputMethodPrivate.getCurrentInputMethod(
        method => this.saveCurrentInputMethodAndStart_(method));
  }

  /**
   * Called when InputController has received the current input method. We save
   * the current method to reset when InputController toggles off, then continue
   * with starting up Dictation after the input gets focus (onImeFocus_()).
   * @param {string} method The currently active IME ID.
   * @private
   */
  saveCurrentInputMethodAndStart_(method) {
    this.previousImeEngineId_ = method;
    // Add AccessibilityCommon as an input method and activate it.
    chrome.languageSettingsPrivate.addInputMethod(
        InputController.IME_ENGINE_ID);
    chrome.inputMethodPrivate.setCurrentInputMethod(
        InputController.IME_ENGINE_ID);
  }

  /**
   * Disconnects as the active Input Method Manager. If any text was being
   * composed, commits it.
   */
  disconnect() {
    // Clean up IME state and reset to the previous IME method.
    this.activeImeContextId_ = InputController.NO_ACTIVE_IME_CONTEXT_ID_;
    chrome.inputMethodPrivate.setCurrentInputMethod(this.previousImeEngineId_);
    this.previousImeEngineId_ = '';
  }

  /**
   * Commits the given text to the active IME context.
   * @param {string} text The text to commit
   */
  commitText(text) {
    if (!this.isActive() || !text) {
      return;
    }

    const data = this.getEditableNodeData_();
    if (LocaleInfo.allowSmartCapAndSpacing() && data) {
      const {value, caretIndex} = data;
      text = EditingUtil.smartCapitalization(value, caretIndex, text);
      text = EditingUtil.smartSpacing(value, caretIndex, text);
    }

    chrome.input.ime.commitText({contextID: this.activeImeContextId_, text});
  }

  /**
   * chrome.input.ime.onFocus callback. Save the active context ID, and
   * finish starting speech recognition if needed. This needs to be done
   * before starting recognition in order for browser tests to know that
   * Dictation is already set up as an IME.
   * @param {chrome.input.ime.InputContext} context Input field context.
   * @private
   */
  onImeFocus_(context) {
    this.activeImeContextId_ = context.contextID;
    if (this.onConnectCallback_) {
      this.onConnectCallback_();
      this.onConnectCallback_ = null;
    }
  }

  /**
   * chrome.input.ime.onFocus callback. Stops Dictation if the active
   * context ID lost focus.
   * @param {number} contextId
   * @private
   */
  onImeBlur_(contextId) {
    if (contextId === this.activeImeContextId_) {
      // Clean up context ID immediately. We can no longer use this context.
      this.activeImeContextId_ = InputController.NO_ACTIVE_IME_CONTEXT_ID_;
      this.stopDictationCallback_();
    }
  }

  /**
   * Deletes the sentence to the left of the text caret. If the caret is in the
   * middle of a sentence, it will delete a portion of the sentence it
   * intersects.
   */
  deletePrevSentence() {
    const data = this.getEditableNodeData_();
    if (!data) {
      return;
    }

    const {value, caretIndex} = data;
    const prevSentenceStart = EditingUtil.navPrevSent(value, caretIndex);
    const length = caretIndex - prevSentenceStart;
    this.deleteSurroundingText_(length, -length);
  }

  /**
   * @param {number} length The number of characters to be deleted.
   * @param {number} offset The offset from the caret position where deletion
   * will start. This value can be negative.
   * @private
   */
  deleteSurroundingText_(length, offset) {
    chrome.input.ime.deleteSurroundingText({
      contextID: this.activeImeContextId_,
      engineID: InputController.IME_ENGINE_ID,
      length,
      offset,
    });
  }

  /**
   * Deletes a phrase to the left of the text caret. If multiple instances of
   * `phrase` are present, it deletes the one closest to the text caret.
   * @param {string} phrase The phrase to be deleted.
   */
  deletePhrase(phrase) {
    this.replacePhrase(phrase, '');
  }

  /**
   * Replaces a phrase to the left of the text caret with another phrase. If
   * multiple instances of `deletePhrase` are present, this function will
   * replace the one closest to the text caret.
   * @param {string} deletePhrase The phrase to be deleted.
   * @param {string} insertPhrase The phrase to be inserted.
   */
  replacePhrase(deletePhrase, insertPhrase) {
    let data = this.getEditableNodeData_();
    if (!data) {
      return;
    }

    const {value, caretIndex} = data;
    data = EditingUtil.replacePhrase(
        value, caretIndex, deletePhrase, insertPhrase);
    const newValue = data.value;
    const newIndex = data.caretIndex;
    this.setEditableValueAndUpdateCaretPosition_(newValue, newIndex);
  }

  /**
   * Inserts `insertPhrase` directly before `beforePhrase` (and separates them
   * with a space). This function operates on the text to the left of the caret.
   * If multiple instances of `beforePhrase` are present, this function will
   * use the one closest to the text caret.
   * @param {string} insertPhrase
   * @param {string} beforePhrase
   */
  insertBefore(insertPhrase, beforePhrase) {
    let data = this.getEditableNodeData_();
    if (!data) {
      return;
    }

    const {value, caretIndex} = data;
    data =
        EditingUtil.insertBefore(value, caretIndex, insertPhrase, beforePhrase);
    const newValue = data.value;
    const newIndex = data.caretIndex;
    this.setEditableValueAndUpdateCaretPosition_(newValue, newIndex);
  }

  /**
   * Sets selection starting at `startPhrase` and ending at `endPhrase`
   * (inclusive). The function operates on the text to the left of the text
   * caret. If multiple instances of `startPhrase` or `endPhrase` are present,
   * the function will use the ones closest to the text caret.
   * @param {string} startPhrase
   * @param {string} endPhrase
   */
  selectBetween(startPhrase, endPhrase) {
    const data = this.getEditableNodeData_();
    if (!data) {
      return;
    }

    const {node, value, caretIndex} = data;
    const selection =
        EditingUtil.selectBetween(value, caretIndex, startPhrase, endPhrase);
    if (!selection) {
      return;
    }

    node.setSelection(selection.start, selection.end);
  }

  /** Moves the text caret to the next sentence. */
  navNextSent() {
    const data = this.getEditableNodeData_();
    if (!data) {
      return;
    }

    const {node, value, caretIndex} = data;
    const newCaretIndex = EditingUtil.navNextSent(value, caretIndex);
    node.setSelection(newCaretIndex, newCaretIndex);
  }

  /** Moves the text caret to the previous sentence. */
  navPrevSent() {
    const data = this.getEditableNodeData_();
    if (!data) {
      return;
    }

    const {node, value, caretIndex} = data;
    const newCaretIndex = EditingUtil.navPrevSent(value, caretIndex);
    node.setSelection(newCaretIndex, newCaretIndex);
  }

  /**
   * @param {string} value
   * @param {number} index
   * @private
   */
  setEditableValueAndUpdateCaretPosition_(value, index) {
    const editableNode = this.focusHandler_.getEditableNode();
    if (!editableNode) {
      return;
    }

    // Set the value first, then update the caret position.
    let handled = false;
    const setSelection = () => {
      if (!handled) {
        // Ensure this listener only runs once.
        editableNode.removeEventListener(
            EventType.VALUE_CHANGED, setSelection, false);
        editableNode.setSelection(index, index);
        handled = true;
      }
    };

    editableNode.addEventListener(EventType.VALUE_CHANGED, setSelection, false);
    editableNode.setValue(value);
  }

  /**
   * Returns the value and caret index of the currently focused editable node.
   * @return {!{node: !AutomationNode, value: string, caretIndex: number}|null}
   * @private
   */
  getEditableNodeData_() {
    const node = this.focusHandler_.getEditableNode();
    if (!node || node.value === undefined || node.textSelStart === undefined ||
        node.textSelStart !== node.textSelEnd) {
      return null;
    }

    return {node, value: node.value, caretIndex: node.textSelStart};
  }
}

/**
 * The IME engine ID for AccessibilityCommon.
 * @const {string}
 */
InputController.IME_ENGINE_ID =
    '_ext_ime_egfdjlfmgnehecnclamagfafdccgfndpdictation';

/**
 * @private {number}
 * @const
 */
InputController.NO_ACTIVE_IME_CONTEXT_ID_ = -1;
