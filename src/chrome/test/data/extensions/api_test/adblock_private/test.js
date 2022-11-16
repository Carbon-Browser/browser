// This file is part of eyeo Chromium SDK,
// Copyright (C) 2006-present eyeo GmbH
//
// eyeo Chromium SDK is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3 as
// published by the Free Software Foundation.
//
// eyeo Chromium SDK is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.

function findEnglishEasyList(item) {
  console.log(item.title + ' ' + item.url + ' ' + item.languages);
  return item.title.toLowerCase().includes('easylist') &&
      item.url.toLowerCase().includes('easylist') &&
      item.languages.includes('en');
}

function containsSubscription(subscriptions, url) {
  for (const subscription of subscriptions) {
    if (subscription.url === url) {
      return true;
    }
  }
  return false;
}

function verifySessionStats(sessionStats, expectedSubscription) {
  let index = -1;
  for (let i = 0; i < sessionStats.length; i++) {
    if (sessionStats[i].url === expectedSubscription) {
      index = i;
      break;
    }
  }
  if (index == -1) {
    chrome.test.fail('Failed: Missing entry in sessionStats');
    return;
  }
  if (sessionStats[index].count != 1) {
    chrome.test.fail(
        'Failed: Expected a single subscription hit number, got: ' +
        sessionStats[index].count);
    return;
  }
}

function verifyEventData(event, expectedSubscription) {
  if (event.tab_id < 1) {
    chrome.test.fail('Failed: Wrong tab_id value');
  }
  if (event.window_id < 1) {
    chrome.test.fail('Failed: Wrong window_id value');
  }
  if (event.content_type !== 'XMLHTTPREQUEST') {
    chrome.test.fail('Failed: Wrong content_type value: ' + event.content_type);
  }
  if (event.subscription !== expectedSubscription) {
    chrome.test.fail('Failed: Wrong subscription value: ' + event.subscription);
  }
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
    chrome.adblockPrivate.setEnabled(true);
    chrome.adblockPrivate.isEnabled(function(enabled) {
      if (enabled)
        chrome.test.succeed();
      else
        chrome.test.fail('Failed: Adblocking should be enabled');
    });
    chrome.adblockPrivate.setEnabled(false);
    chrome.adblockPrivate.isEnabled(function(enabled) {
      if (!enabled)
        chrome.test.succeed();
      else
        chrome.test.fail('Failed: Adblocking should be enabled');
    });
  },
  function setAAEnabled_isAAEnabled() {
    chrome.adblockPrivate.setAcceptableAdsEnabled(true);
    chrome.adblockPrivate.getSelectedBuiltInSubscriptions(function(selected) {
      if (!containsSubscription(
              selected,
              'https://easylist-downloads.adblockplus.org/exceptionrules.txt')) {
        chrome.test.fail('Failed: AA subscription should be on the list');
      }
    });
    chrome.adblockPrivate.isAcceptableAdsEnabled(function(enabled) {
      if (enabled)
        chrome.test.succeed();
      else
        chrome.test.fail('Failed: AA should be enabled');
    });
    chrome.adblockPrivate.setAcceptableAdsEnabled(false);
    chrome.adblockPrivate.getSelectedBuiltInSubscriptions(function(selected) {
      if (containsSubscription(
              selected,
              'https://easylist-downloads.adblockplus.org/exceptionrules.txt')) {
        chrome.test.fail('Failed: AA subscription should NOT be on the list');
      }
    });
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
  function selectedSubscriptionsDataSchema() {
    chrome.adblockPrivate.getSelectedBuiltInSubscriptions(function(selected) {
      for (const subscription of selected) {
        if (!subscription.current_version)
          chrome.test.fail('Failed: Must contain "current_version" property');
        if (!subscription.installation_state)
          chrome.test.fail(
              'Failed: Must contain "installation_state" property');
        if (!subscription.last_installation_time)
          chrome.test.fail(
              'Failed: Must contain "last_installation_time" property');
        if (!subscription.title)
          chrome.test.fail('Failed: Must contain "title" property');
        if (!subscription.url)
          chrome.test.fail('Failed: Must contain "url" property');
        chrome.test.succeed();
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
    const urlParams = new URLSearchParams(window.location.search);
    let kEasylist = urlParams.get('easylist');
    let kExceptionrules = urlParams.get('exceptions');
    let kABPFilters = urlParams.get('snippets');

    function containsDefaultSubscriptions(subscriptions) {
      return containsSubscription(subscriptions, kEasylist) &&
          containsSubscription(subscriptions, kExceptionrules) &&
          containsSubscription(subscriptions, kABPFilters);
    }

    chrome.adblockPrivate.isAcceptableAdsEnabled(function(enabled) {
      if (!enabled)
        chrome.test.fail('Failed: AA should be enabled');
    });

    chrome.adblockPrivate.getSelectedBuiltInSubscriptions(function(selected) {
      if (selected.length) {
        if (!containsDefaultSubscriptions(selected)) {
          chrome.test.fail('Failed: Should contain all default subscriptions');
        }
        for (const subscription of selected) {
          chrome.adblockPrivate.unselectBuiltInSubscription(subscription.url);
        }
      } else {
        chrome.test.fail('Failed: Should contain default subscriptions');
      }
      chrome.adblockPrivate.getSelectedBuiltInSubscriptions(function(selected) {
        if (selected.length) {
          chrome.test.fail(
              'Failed: There shouldn\'t be any selected subscriptions');
        }

        chrome.adblockPrivate.isAcceptableAdsEnabled(function(enabled) {
          if (enabled)
            chrome.test.fail('Failed: AA should NOT be enabled');
        });

        chrome.adblockPrivate.selectBuiltInSubscription(kEasylist);
        chrome.adblockPrivate.selectBuiltInSubscription(kExceptionrules);
        chrome.adblockPrivate.selectBuiltInSubscription(kABPFilters);

        chrome.adblockPrivate.isAcceptableAdsEnabled(function(enabled) {
          if (!enabled)
            chrome.test.fail('Failed: AA should be enabled');
        });

        chrome.adblockPrivate.getSelectedBuiltInSubscriptions(function(
            selected) {
          if (selected.length) {
            if (!containsDefaultSubscriptions(selected)) {
              chrome.test.fail(
                  'Failed: Should contain all default subscriptions');
            }
            chrome.test.succeed();
          } else {
            chrome.test.fail('Failed: Should contain default subscriptions');
          }
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
          if (!containsSubscription(custom, 'https://foo.bar/') ||
              !containsSubscription(custom, 'https://bar.baz/')) {
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
  function adBlockedEvents() {
    const urlParams = new URLSearchParams(window.location.search);
    let expectedSubscription = urlParams.get('subscription_url');
    chrome.adblockPrivate.onSubscriptionUpdated.addListener(function(
        subscription) {
      if (subscription !== expectedSubscription)
        chrome.test.fail('Failed: Unexpected subscription: ' + subscription);

      chrome.adblockPrivate.onAdBlocked.addListener(function(e) {
        verifyEventData(e, expectedSubscription);
        if (e.url.includes('test1.png')) {
          chrome.test.succeed();
        }
      });
      let request = new XMLHttpRequest();
      let handler = function() {};
      request.onload = handler;
      request.onerror = handler;
      request.open('GET', 'https://example.com/test1.png', true);
      request.send();
    });
    chrome.adblockPrivate.addCustomSubscription(expectedSubscription);
  },
  function adAllowedEvents() {
    const urlParams = new URLSearchParams(window.location.search);
    let expectedSubscription = urlParams.get('subscription_url');
    chrome.adblockPrivate.onSubscriptionUpdated.addListener(function(
        subscription) {
      if (subscription !== expectedSubscription)
        chrome.test.fail('Failed: Unexpected subscription: ' + subscription);

      chrome.adblockPrivate.onAdAllowed.addListener(function(e) {
        verifyEventData(e, expectedSubscription);
        if (e.url.includes('test2.png')) {
          chrome.test.succeed();
        }
      });
      let request = new XMLHttpRequest();
      let handler = function() {};
      request.onload = handler;
      request.onerror = handler;
      request.open('GET', 'https://example.com/test2.png', true);
      request.send();
    });
    chrome.adblockPrivate.addCustomSubscription(expectedSubscription);
  },
  function sessionStats() {
    const urlParams = new URLSearchParams(window.location.search);
    let expectedSubscription = urlParams.get('subscription_url');
    // Verify that subscription was downloaded and update event triggered,
    // then verify correct number of a session blocked and allowed ads count
    // for this subscription.
    chrome.adblockPrivate.onSubscriptionUpdated.addListener(function(
        subscription) {
      if (subscription !== expectedSubscription)
        chrome.test.fail('Failed: Unexpected subscription: ' + subscription);

      let handler = function() {
        chrome.adblockPrivate.getSessionBlockedAdsCount(function(sessionStats) {
          verifySessionStats(sessionStats, expectedSubscription);
          let handler = function() {
            chrome.adblockPrivate.getSessionAllowedAdsCount(function(
                sessionStats) {
              verifySessionStats(sessionStats, expectedSubscription);
              chrome.test.succeed();
            });
          };
          let request = new XMLHttpRequest();
          request.onload = handler;
          request.onerror = handler;
          request.open('GET', 'https://example.com/test4.png', true);
          request.send();
        });
      };
      let request = new XMLHttpRequest();
      request.onload = handler;
      request.onerror = handler;
      request.open('GET', 'https://example.com/test3.png', true);
      request.send();
    });
    chrome.adblockPrivate.addCustomSubscription(expectedSubscription);
  },
];

var testToRun;
if (window.location.search.indexOf('&') > 1)
  testToRun =
      window.location.search.substring(1, window.location.search.indexOf('&'))
else
  testToRun = window.location.search.substring(1);
chrome.test.runTests(availableTests.filter(function(op) {
  return op.name == testToRun;
}));
