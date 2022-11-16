// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'chrome://resources/cr_elements/cr_button/cr_button.m.js';
import 'chrome://resources/cr_elements/cr_icon_button/cr_icon_button.m.js';
import 'chrome://resources/cr_elements/cr_input/cr_input.m.js';
import 'chrome://resources/cr_elements/cr_icons_css.m.js';
import 'chrome://resources/cr_elements/hidden_style_css.m.js';

import {PolymerElement} from '//resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {CrInputElement} from 'chrome://resources/cr_elements/cr_input/cr_input.m.js';

import {getTemplate} from './cr_input_demo_component.html.js';

interface CrInputDemoComponent {
  $: {
    numberInput: CrInputElement,
  };
}

class CrInputDemoComponent extends PolymerElement {
  static get is() {
    return 'cr-input-demo';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      emailValue_: String,
      numberValue_: String,
      pinValue_: String,
      searchValue_: String,
      textValue_: String,
    };
  }

  private emailValue_: string;
  private numberValue_: string;
  private pinValue_: string;
  private searchValue_: string;
  private textValue_: string;

  private onClearSearchClick_() {
    this.searchValue_ = '';
  }

  private onValidateClick_() {
    this.$.numberInput.validate();
  }
}

customElements.define(CrInputDemoComponent.is, CrInputDemoComponent);
