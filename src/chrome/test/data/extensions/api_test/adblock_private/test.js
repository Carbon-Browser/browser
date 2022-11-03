// This file is part of Adblock Plus <https://adblockplus.org/>,
// Copyright (C) 2006-present eyeo GmbH
//
// Adblock Plus is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3 as
// published by the Free Software Foundation.
//
// Adblock Plus is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.

function findEnglishEasyList(item) {
  console.log(item.title + ' ' + item.url + ' ' + item.languages);
  return item.title.toLowerCase().includes('easylist') &&
      item.url.toLowerCase().includes('easylist') &&
      item.languages.includes('en');
}

var availableTests = [
  function updateConsent() {
    chrome.adblockPrivate.getUpdateConsent(function(consent) {
      if (consent == 'WIFI_ONLY') {
        chrome.adblockPrivate.setUpdateConsent('ALWAYS');
        chrome.adblockPrivate.getUpdateConsent(function(consent) {
          if (consent != 'ALWAYS') {
            chrome.test.fail('Failed: Consent not set');
            return;
          }
          chrome.adblockPrivate.setUpdateConsent('WIFI_ONLY');
          chrome.adblockPrivate.getUpdateConsent(function(consent) {
            if (consent == 'WIFI_ONLY') {
              chrome.test.succeed();
            } else {
              chrome.test.fail('Failed: Consent not set');
            }
          });
        });
      } else if (consent == 'ALWAYS') {
        chrome.adblockPrivate.setUpdateConsent('WIFI_ONLY');
        chrome.adblockPrivate.getUpdateConsent(function(consent) {
          if (consent != 'WIFI_ONLY') {
            chrome.test.fail('Failed: Consent not set');
            return;
          }
          chrome.adblockPrivate.setUpdateConsent('ALWAYS');
          chrome.adblockPrivate.getUpdateConsent(function(consent) {
            if (consent == 'ALWAYS') {
              chrome.test.succeed();
            } else {
              chrome.test.fail('Failed: Consent not set');
            }
          });
        });
      } else {
        chrome.test.fail('Failed: Unknown consent');
      }
    });
  },
  function setEnabled_isEnabled() {
    chrome.adblockPrivate.setEnabled(true)
    chrome.adblockPrivate.isEnabled(function(enabled) {
      if (enabled)
        chrome.test.succeed();
      else
        chrome.test.fail('Failed: ABP should be enabled');
    });
    chrome.adblockPrivate.setEnabled(false)
    chrome.adblockPrivate.isEnabled(function(enabled) {
      if (!enabled)
        chrome.test.succeed();
      else
        chrome.test.fail('Failed: ABP should be enabled');
    });
  },
  function setAAEnabled_isAAEnabled() {
    chrome.adblockPrivate.setAcceptableAdsEnabled(true)
    chrome.adblockPrivate.isAcceptableAdsEnabled(function(enabled) {
      if (enabled)
        chrome.test.succeed();
      else
        chrome.test.fail('Failed: AA should be enabled');
    });
    chrome.adblockPrivate.setAcceptableAdsEnabled(false)
    chrome.adblockPrivate.isAcceptableAdsEnabled(function(enabled) {
      if (!enabled)
        chrome.test.succeed();
      else
        chrome.test.fail('Failed: AA should be enabled');
    });
  },
  function getBuiltInSubscriptions() {
    chrome.adblockPrivate.getBuiltInSubscriptions(function(recommended) {
      let found = recommended.find(findEnglishEasyList);
      if (found) {
        chrome.test.succeed();
      } else {
        chrome.test.fail('Failed: invalid built-in subscriptions');
      }
    });
  },
  function selectBuiltInSubscriptionsInvalidURL() {
    chrome.adblockPrivate.selectBuiltInSubscription('http://', function() {
      if (!chrome.runtime.lastError)
        chrome.test.fail('Failed: invalid input accepted');
      else
        chrome.test.succeed();
    });
  },
  function selectBuiltInSubscriptionsNotBuiltIn() {
    chrome.adblockPrivate.selectBuiltInSubscription(
        'http://foo.bar', function() {
          if (!chrome.runtime.lastError)
            chrome.test.fail('Failed: invalid input accepted');
          else
            chrome.test.succeed();
        });
  },
  function unselectBuiltInSubscriptionsInvalidURL() {
    chrome.adblockPrivate.unselectBuiltInSubscription('http://', function() {
      if (!chrome.runtime.lastError)
        chrome.test.fail('Failed: invalid input accepted');
      else
        chrome.test.succeed();
    });
  },
  function unselectBuiltInSubscriptionsNotBuiltIn() {
    chrome.adblockPrivate.unselectBuiltInSubscription(
        'http://foo.bar', function() {
          if (!chrome.runtime.lastError)
            chrome.test.fail('Failed: invalid input accepted');
          else
            chrome.test.succeed();
        });
  },
  function builtInSubscriptionsManagement() {
    var kTestSubscription = 4;
    chrome.adblockPrivate.getSelectedBuiltInSubscriptions(function(selected) {
      if (selected.length) {
        for (const subscription of selected)
          chrome.adblockPrivate.unselectBuiltInSubscription(subscription);
      }

      chrome.adblockPrivate.getSelectedBuiltInSubscriptions(function(selected) {
        if (selected.length) {
          chrome.test.fail(
              'Failed: There shouldn\'t be any selected subscriptions');
        }

        chrome.adblockPrivate.getBuiltInSubscriptions(function(recommended) {
          if (recommended.length < kTestSubscription) {
            chrome.test.fail('Failed: Too few recommended subscriptions');
            return;
          }
          for (let i = 0; i < kTestSubscription; ++i)
            chrome.adblockPrivate.selectBuiltInSubscription(recommended[i].url);

          chrome.adblockPrivate.getSelectedBuiltInSubscriptions(function(
              selected) {
            if (selected.length != kTestSubscription) {
              chrome.test.fail('Failed: Wrong number selected');
              return;
            }

            for (const subscription of selected)
              chrome.adblockPrivate.unselectBuiltInSubscription(subscription);
            chrome.adblockPrivate.getSelectedBuiltInSubscriptions(function(
                selected) {
              if (selected.length == 0)
                chrome.test.succeed();
              else
                chrome.test.fail('Failed: No subscription should be selected');
            });
          });
        });
      });
    });
  },
  function addCustomSubscriptionInvalidURL() {
    chrome.adblockPrivate.addCustomSubscription('http://', function() {
      if (!chrome.runtime.lastError)
        chrome.test.fail('Failed: invalid input accepted');
      else
        chrome.test.succeed();
    });
  },
  function removeCustomSubscriptionInvalidURL() {
    chrome.adblockPrivate.removeCustomSubscription('http://', function() {
      if (!chrome.runtime.lastError)
        chrome.test.fail('Failed: invalid input accepted');
      else
        chrome.test.succeed();
    });
  },
  function customSubscriptionsManagement() {
    chrome.adblockPrivate.getCustomSubscriptions(function(custom) {
      if (custom.length) {
        for (const subscription of custom)
          chrome.adblockPrivate.removeCustomSubscription(subscription);
      }

      chrome.adblockPrivate.getCustomSubscriptions(function(custom) {
        if (custom.length) {
          chrome.test.fail(
              'Failed: There shouldn\'t be any custom subscriptions');
          return;
        }

        chrome.adblockPrivate.addCustomSubscription('https://foo.bar/');
        chrome.adblockPrivate.addCustomSubscription('https://bar.baz/');
        chrome.adblockPrivate.getCustomSubscriptions(function(custom) {
          if (custom.length != 2) {
            chrome.test.fail('Failed: There should be 2 custom subscriptions');
            return;
          }

          if (custom.indexOf('https://foo.bar/') == -1 ||
              custom.indexOf('https://bar.baz/') == -1) {
            chrome.test.fail(
                'Failed: Didn\'t find expected custom subscriptions');
            return;
          }

          chrome.adblockPrivate.removeCustomSubscription('https://foo.bar/');
          chrome.adblockPrivate.removeCustomSubscription('https://bar.baz/');

          chrome.adblockPrivate.getCustomSubscriptions(function(custom) {
            if (custom.length)
              chrome.test.fail('Failed: Still have custom subscriptions');
            else
              chrome.test.succeed();
          });
        });
      });
    });
  },
  function allowedDomainsManagement() {
    chrome.adblockPrivate.getAllowedDomains(function(domains) {
      if (domains.length) {
        for (const domain of domains)
          chrome.adblockPrivate.removeAllowedDomain(domain);
      }

      chrome.adblockPrivate.getAllowedDomains(function(domains) {
        if (domains.length) {
          chrome.test.fail('Failed: There shouldn\'t be any allowed domains');
          return;
        }

        chrome.adblockPrivate.addAllowedDomain('foo.bar');
        chrome.adblockPrivate.addAllowedDomain('bar.baz');
        chrome.adblockPrivate.getAllowedDomains(function(domains) {
          if (domains.length != 2) {
            chrome.test.fail('Failed: There should be 2 allowed domains');
            return;
          }

          if (domains.indexOf('foo.bar') == -1 ||
              domains.indexOf('bar.baz') == -1) {
            chrome.test.fail('Failed: Didn\'t find expected allowed domains');
            return;
          }

          chrome.adblockPrivate.removeAllowedDomain('foo.bar');
          chrome.adblockPrivate.removeAllowedDomain('bar.baz');

          chrome.adblockPrivate.getAllowedDomains(function(domains) {
            if (domains.length)
              chrome.test.fail('Failed: Still have allowed domains');
            else
              chrome.test.succeed();
          });
        });
      });
    });
  },
  function events() {
    var numBlocked = 0;
    chrome.adblockPrivate.onAdBlocked.addListener(function(e) {
      if (e.url.includes('test.png')) {
        numBlocked++;
      }
    });

    chrome.adblockPrivate.addCustomFilter('test.png');
    let request = new XMLHttpRequest();
    let handler = function() {
      chrome.adblockPrivate.removeCustomFilter('test.png');
      let request = new XMLHttpRequest();
      let handler = function() {
        if (numBlocked == 1) {
          chrome.test.succeed();
        } else {
          chrome.test.fail(
              'Failed: Exactly one block should happen, had ' + numBlocked);
        }
      };
      request.onload = handler;
      request.onerror = handler;
      request.open('GET', 'http://example.com/test.png', true);
      request.send();
    };
    request.onload = handler;
    request.onerror = handler;
    request.open('GET', 'http://example.com/test.png', true);
    request.send();
  },
];

var testToRun = window.location.search.substring(1);
chrome.test.runTests(availableTests.filter(function(op) {
  return op.name == testToRun;
}));
