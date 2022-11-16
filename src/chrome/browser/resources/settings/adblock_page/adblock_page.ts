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
import 'chrome://resources/cr_elements/cr_button/cr_button.m.js';
import 'chrome://resources/cr_elements/shared_style_css.m.js';
import 'chrome://resources/polymer/v3_0/iron-collapse/iron-collapse.js';
import 'chrome://resources/polymer/v3_0/iron-flex-layout/iron-flex-layout-classes.js';
import 'chrome://resources/polymer/v3_0/iron-icon/iron-icon.js';
import '../controls/controlled_button.js';
import '../controls/settings_toggle_button.js';
import '../settings_shared.css.js';


import { I18nMixin } from 'chrome://resources/js/i18n_mixin.js';
import { BaseMixin } from '../base_mixin.js';
import { html, PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import { PrefsMixin } from '../prefs/prefs_mixin.js';
import { CrSettingsPrefs } from '../prefs/prefs_types.js';
import { CrCheckboxElement } from 'chrome://resources/cr_elements/cr_checkbox/cr_checkbox.m.js';
import { getTemplate } from './adblock_page.html.js';

const SettingsAdblockPageElementBase =
   I18nMixin(PrefsMixin(BaseMixin((PolymerElement))));

export class SettingsAdblockPageElement extends
   SettingsAdblockPageElementBase {
   static get is() {
      return 'settings-adblock-page';
   }

   static get template() {
      return getTemplate();
   }

   private syncSubscriptions() {
      let prefSubscriptions = this.getPref("adblock.subscriptions").value;
      this.subscriptions.forEach((subscription, i) => {
         if (prefSubscriptions.includes(subscription['url'])) {
            this.set("subscriptions." + i.toString() + ".enabled", true)
         } else {
            this.set("subscriptions." + i.toString() + ".enabled", false)
         }
      })
   }

   private syncCustomSubscriptions() {
      let prefCustomSubscriptions = this.getPref("adblock.custom_subscriptions").value;
      this.customSubscriptions = [];
      prefCustomSubscriptions.forEach((subscription: string) => {
         this.customSubscriptions.push(subscription);
      })
   }

   private syncCustomFilters() {
      let prefCustomFilters = this.getPref("adblock.custom_filters").value;
      this.customFilters = [];
      prefCustomFilters.forEach((filter: string) => {
         this.customFilters.push(filter);
      })
   }

   private syncAllowedDomains() {
      let prefAllowedDomains = this.getPref("adblock.allowed_domains").value;
      this.allowedDomains = [];
      prefAllowedDomains.forEach((subscription: string) => {
         this.allowedDomains.push(subscription);
      })
   }

   public override ready() {
      super.ready();
      // when prefs are ready
      CrSettingsPrefs.initialized.then(() => {
         this.syncSubscriptions();
         this.syncCustomSubscriptions();
         this.syncCustomFilters();
         this.syncAllowedDomains();
      });
   }

   // input fields updated by html
   public customSubscriptionInput: string;
   public customFilterInput: string;
   public allowedDomainInput: string;

   // models that will fill templates lists in html
   public customSubscriptions: Array<string> = [];
   public customFilters: Array<string> = [];
   public allowedDomains: Array<string> = [];
   public subscriptions = [
      {
         title: "ABPindo+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/abpindo+easylist.txt"
      },
      {
         title: "ABPVN List+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/abpvn+easylist.txt"
      },
      {
         title: "Bulgarian list+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/bulgarian_list+easylist.txt"
      },
      {
         title: "Dandelion Sprout's Nordic Filters+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/dandelion_sprouts_nordic_filters+easylist.txt"
      },
      {
         title: "EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/easylist.txt"
      },
      {
         title: "EasyList China+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/easylistchina+easylist.txt"
      },
      {
         title: "EasyList Czech and Slovak+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/easylistczechslovak+easylist.txt"
      },
      {
         title: "EasyList Dutch+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/easylistdutch+easylist.txt"
      },
      {
         title: "EasyList Germany+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/easylistgermany+easylist.txt"
      },
      {
         title: "EasyList Hebrew+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/israellist+easylist.txt"
      },
      {
         title: "EasyList Italy+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/easylistitaly+easylist.txt"
      },
      {
         title: "EasyList Lithuania+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/easylistlithuania+easylist.txt"
      },
      {
         title: "EasyList Polish+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/easylistpolish+easylist.txt"
      },
      {
         title: "EasyList Portuguese+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/easylistportuguese+easylist.txt"
      },
      {
         title: "EasyList Spanish+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/easylistspanish+easylist.txt"
      },
      {
         title: "IndianList+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/indianlist+easylist.txt"
      },
      {
         title: "KoreanList+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/koreanlist+easylist.txt"
      },
      {
         title: "Latvian List+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/latvianlist+easylist.txt"
      },
      {
         title: "Liste AR+Liste FR+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/liste_ar+liste_fr+easylist.txt"
      },
      {
         title: "Liste FR+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/liste_fr+easylist.txt"
      },
      {
         title: "ROList+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/rolist+easylist.txt"
      },
      {
         title: "RuAdList+EasyList",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/ruadlist+easylist.txt"
      },
      {
         title: "ABP filters",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/abp-filters-anti-cv.txt"
      },
      {
         title: "I don't care about cookies",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/i_dont_care_about_cookies.txt"
      },
      {
         title: "Fanboy's Notifications Blocking List",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/fanboy-notifications.txt"
      },
      {
         title: "EasyPrivacy",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/easyprivacy.txt"
      },
      {
         title: "Fanboy's Social Blocking List",
         enabled: false,
         url: "https://easylist-downloads.adblockplus.org/fanboy-social.txt"
      }
   ];

   private cleanUrl(url: string) : string {
      let cleanedUrl : string = "";
      try {
         cleanedUrl = new URL(url).host;
      } catch (err) {
         try {
            // one last try by adding schema
            cleanedUrl = new URL("https://" + url).host;
         }
         catch (err) {
            console.log("malformed url " + url);
         }
      }
      return cleanedUrl;
   }

   private selectRecommendedSubscription(e: Event) {
      const url = ((e.target as CrCheckboxElement).id);
      const enabled = ((e.target as CrCheckboxElement).checked);
      if (enabled) {
         this.appendPrefListItem("adblock.subscriptions", url);
      } else {
         this.deletePrefListItem("adblock.subscriptions", url);
      }
   }

   private removeCustomFilter(e: Event) {
      const filter = ((e.target as HTMLElement).id);
      this.deletePrefListItem("adblock.custom_filters", filter);
      const i = this.customFilters.indexOf(filter);
      this.splice('customFilters', i, 1);
   }

   private addCustomFilter() {
      if (this.customFilterInput == undefined) return;
      this.appendPrefListItem("adblock.custom_filters", this.customFilterInput);
      this.push('customFilters', this.customFilterInput);
   }

   private removeAllowedDomain(e: Event) {
      const allowedDomain = ((e.target as HTMLElement).id);
      this.deletePrefListItem("adblock.allowed_domains", allowedDomain);
      const i = this.allowedDomains.indexOf(allowedDomain);
      this.splice('allowedDomains', i, 1);
   }

   private addAllowedDomain() {
      if (this.allowedDomainInput == undefined) return;
      const cleanedUrl = this.cleanUrl(this.allowedDomainInput);
      if (cleanedUrl == "") return;
      this.appendPrefListItem("adblock.allowed_domains", cleanedUrl);
      this.push('allowedDomains', cleanedUrl);
   }

   private removeCustomSubscription(e: Event) {
      const subscription = ((e.target as HTMLElement).id);
      this.deletePrefListItem("adblock.custom_subscriptions", subscription);
      const i = this.customSubscriptions.indexOf(subscription);
      this.splice('customSubscriptions', i, 1);
   }

   private addCustomSubscription() {
      if (this.customSubscriptionInput == undefined) return;
      this.appendPrefListItem("adblock.custom_subscriptions", this.customSubscriptionInput);
      this.push('customSubscriptions', this.customSubscriptionInput);
   }
}


declare global {
   interface HTMLElementTagNameMap {
      'settings-adblock-page': SettingsAdblockPageElement;
   }
}

customElements.define(
   SettingsAdblockPageElement.is, SettingsAdblockPageElement);
