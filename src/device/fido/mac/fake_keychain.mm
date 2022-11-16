// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/mac/fake_keychain.h"

#import <Foundation/Foundation.h>
#import <Security/Security.h>

#include "base/check_op.h"
#include "base/mac/foundation_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/notreached.h"
#include "device/fido/mac/credential_store.h"
#include "device/fido/mac/keychain.h"

namespace device {
namespace fido {
namespace mac {

FakeKeychain::FakeKeychain(const std::string& keychain_access_group)
    : Keychain(),
      keychain_access_group_(
          CFStringCreateWithCString(kCFAllocatorDefault,
                                    keychain_access_group.data(),
                                    kCFStringEncodingUTF8)) {}
FakeKeychain::~FakeKeychain() = default;

base::ScopedCFTypeRef<SecKeyRef> FakeKeychain::KeyCreateRandomKey(
    CFDictionaryRef params,
    CFErrorRef* error) {
  // Validate certain fields that we always expect to be set.
  DCHECK(base::mac::GetValueFromDictionary<CFStringRef>(params, kSecAttrLabel));
  DCHECK(base::mac::GetValueFromDictionary<CFDataRef>(
      params, kSecAttrApplicationLabel));
  DCHECK(base::mac::GetValueFromDictionary<CFStringRef>(
      params, kSecAttrApplicationTag));
  DCHECK_EQ(
      base::mac::GetValueFromDictionary<CFStringRef>(params, kSecAttrTokenID),
      kSecAttrTokenIDSecureEnclave);
  DCHECK(CFEqual(base::mac::GetValueFromDictionary<CFStringRef>(
                     params, kSecAttrAccessGroup),
                 keychain_access_group_));

  // Call Keychain services to create a key pair, but first drop all parameters
  // that aren't appropriate in tests.
  base::ScopedCFTypeRef<CFMutableDictionaryRef> params_copy(
      CFDictionaryCreateMutableCopy(kCFAllocatorDefault, /*capacity=*/0,
                                    params));
  // Don't create a Secure Enclave key.
  CFDictionaryRemoveValue(params_copy, kSecAttrTokenID);
  // Don't bind to a keychain-access-group, which would require an entitlement.
  CFDictionaryRemoveValue(params_copy, kSecAttrAccessGroup);

  base::ScopedCFTypeRef<CFMutableDictionaryRef> private_key_params(
      CFDictionaryCreateMutableCopy(
          kCFAllocatorDefault, /*capacity=*/0,
          base::mac::GetValueFromDictionary<CFDictionaryRef>(
              params_copy, kSecPrivateKeyAttrs)));
  DCHECK(CFEqual(base::mac::GetValueFromDictionary<CFBooleanRef>(
                     private_key_params, kSecAttrIsPermanent),
                 @YES));
  CFDictionarySetValue(private_key_params, kSecAttrIsPermanent, @NO);
  // Don't put access control on the key (e.g. requiring local user
  // authentication for key use).
  DCHECK(CFEqual(base::mac::GetValueFromDictionary<SecAccessControlRef>(
                     private_key_params, kSecAttrAccessControl),
                 TouchIdCredentialStore::DefaultAccessControl()));
  CFDictionaryRemoveValue(private_key_params, kSecAttrAccessControl);
  CFDictionaryRemoveValue(private_key_params, kSecUseAuthenticationContext);
  CFDictionarySetValue(params_copy, kSecPrivateKeyAttrs, private_key_params);
  base::ScopedCFTypeRef<SecKeyRef> private_key(
      SecKeyCreateRandomKey(params_copy, error));
  if (!private_key) {
    return base::ScopedCFTypeRef<SecKeyRef>();
  }

  // Stash everything in `items_` so it can be  retrieved in with
  // `ItemCopyMatching. This uses the original `params` rather than the modified
  // copy so that `ItemCopyMatching()` will correctly filter on
  // kSecAttrAccessGroup.
  base::ScopedCFTypeRef<CFMutableDictionaryRef> keychain_item(
      CFDictionaryCreateMutableCopy(kCFAllocatorDefault, /*capacity=*/0,
                                    params));
  CFDictionarySetValue(keychain_item, kSecValueRef, private_key.get());
  items_.emplace_back(
      base::ScopedCFTypeRef<CFDictionaryRef>(keychain_item.release()));

  return private_key;
}

OSStatus FakeKeychain::ItemCopyMatching(CFDictionaryRef query,
                                        CFTypeRef* result) {
  // In practice we don't need to care about limit queries, or leaving out the
  // SecKeyRef or attributes from the result set.
  DCHECK_EQ(
      base::mac::GetValueFromDictionary<CFBooleanRef>(query, kSecReturnRef),
      kCFBooleanTrue);
  DCHECK_EQ(base::mac::GetValueFromDictionary<CFBooleanRef>(
                query, kSecReturnAttributes),
            kCFBooleanTrue);
  DCHECK_EQ(
      base::mac::GetValueFromDictionary<CFStringRef>(query, kSecMatchLimit),
      kSecMatchLimitAll);

  // Filter the items based on `query`.
  base::ScopedCFTypeRef<CFMutableArrayRef> items(
      CFArrayCreateMutable(NULL, items_.size(), &kCFTypeArrayCallBacks));
  for (auto& item : items_) {
    // Each `Keychain` instance is expected to operate only on items of a single
    // keychain-access-group, which is tied to the `Profile`.
    CFStringRef keychain_access_group =
        base::mac::GetValueFromDictionary<CFStringRef>(query,
                                                       kSecAttrAccessGroup);
    DCHECK(CFEqual(keychain_access_group,
                   base::mac::GetValueFromDictionary<CFStringRef>(
                       item, kSecAttrAccessGroup)) &&
           CFEqual(keychain_access_group, keychain_access_group_));

    // Match fields present in `query`.
    CFStringRef label =
        base::mac::GetValueFromDictionary<CFStringRef>(query, kSecAttrLabel);
    CFDataRef application_label = base::mac::GetValueFromDictionary<CFDataRef>(
        query, kSecAttrApplicationLabel);
    CFStringRef application_tag =
        base::mac::GetValueFromDictionary<CFStringRef>(query,
                                                       kSecAttrApplicationTag);
    if ((label &&
         !CFEqual(label, base::mac::GetValueFromDictionary<CFStringRef>(
                             item, kSecAttrLabel))) ||
        (application_label &&
         !CFEqual(application_label,
                  base::mac::GetValueFromDictionary<CFStringRef>(
                      item, kSecAttrApplicationLabel))) ||
        (application_tag &&
         !CFEqual(application_tag,
                  base::mac::GetValueFromDictionary<CFStringRef>(
                      item, kSecAttrApplicationTag)))) {
      continue;
    }
    base::ScopedCFTypeRef<CFDictionaryRef> item_copy(
        CFDictionaryCreateCopy(kCFAllocatorDefault, item));
    CFArrayAppendValue(items, item_copy);
  }
  if (!items) {
    return errSecItemNotFound;
  }
  *result = items.release();
  return errSecSuccess;
}

OSStatus FakeKeychain::ItemDelete(CFDictionaryRef query) {
  // Validate certain fields that we always expect to be set.
  DCHECK_EQ(base::mac::GetValueFromDictionary<CFStringRef>(query, kSecClass),
            kSecClassKey);
  DCHECK(CFEqual(base::mac::GetValueFromDictionary<CFStringRef>(
                     query, kSecAttrAccessGroup),
                 keychain_access_group_));
  // Only supporting deletion via `kSecAttrApplicationLabel` (credential ID) for
  // now (see `TouchIdCredentialStore::DeleteCredentialById()`).
  CFDataRef query_credential_id = base::mac::GetValueFromDictionary<CFDataRef>(
      query, kSecAttrApplicationLabel);
  DCHECK(query_credential_id);
  for (auto it = items_.begin(); it != items_.end(); ++it) {
    const base::ScopedCFTypeRef<CFDictionaryRef>& item = *it;
    CFDataRef item_credential_id = base::mac::GetValueFromDictionary<CFDataRef>(
        item, kSecAttrApplicationLabel);
    DCHECK(item_credential_id);
    if (CFEqual(query_credential_id, item_credential_id)) {
      items_.erase(it);  // N.B. `it` becomes invalid
      return errSecSuccess;
    }
  }
  // We only delete known items by credential ID currently, so not finding one
  // would be odd.
  NOTREACHED();
  return errSecItemNotFound;
}

ScopedFakeKeychain::ScopedFakeKeychain(const std::string& keychain_access_group)
    : FakeKeychain(keychain_access_group) {
  SetInstanceOverride(this);
}

ScopedFakeKeychain::~ScopedFakeKeychain() {
  ClearInstanceOverride();
}

}  // namespace mac
}  // namespace fido
}  // namespace device
