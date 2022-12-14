// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_UI_SAVED_PASSWORDS_PRESENTER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_UI_SAVED_PASSWORDS_PRESENTER_H_

#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/strings/string_piece_forward.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/browser/password_store_consumer.h"
#include "components/password_manager/core/browser/ui/credential_ui_entry.h"

namespace password_manager {

class PasswordUndoHelper;

// This interface provides a way for clients to obtain a list of all saved
// passwords and register themselves as observers for changes. In contrast to
// simply registering oneself as an observer of a password store directly, this
// class possibly responds to changes in multiple password stores, such as the
// local and account store used for passwords for account store users.
// Furthermore, this class exposes a direct mean to edit a password, and
// notifies its observers about this event. An example use case for this is the
// bulk check settings page, where an edit operation in that page should result
// in the new password to be checked, whereas other password edit operations
// (such as visiting a change password form and then updating the password in
// Chrome) should not trigger a check.
class SavedPasswordsPresenter : public PasswordStoreInterface::Observer,
                                public PasswordStoreConsumer {
 public:
  using SavedPasswordsView = base::span<const PasswordForm>;

  // Observer interface. Clients can implement this to get notified about
  // changes to the list of saved passwords or if a given password was edited
  // Clients can register and de-register themselves, and are expected to do so
  // before the presenter gets out of scope.
  class Observer : public base::CheckedObserver {
   public:
    // Notifies the observer when a password is edited or the list of saved
    // passwords changed.
    //
    // OnEdited() will be invoked synchronously if EditPassword() is invoked
    // with a password that was present in |passwords_|.
    // |password.password_value| will be equal to |new_password| in this case.
    virtual void OnEdited(const PasswordForm& password) {}
    // OnSavedPasswordsChanged() gets invoked asynchronously after a change to
    // the underlying password store happens. This might be due to a call to
    // EditPassword(), but can also happen if passwords are added or removed due
    // to other reasons.
    virtual void OnSavedPasswordsChanged(SavedPasswordsView passwords) {}
  };

  // Result of EditSavedCredentials.
  enum EditResult {
    // Some credentials were successfully updated.
    kSuccess,
    // New credential matches the old one so nothing was changed.
    kNothingChanged,
    // Credential couldn't be found in the store.
    kNotFound,
    // Credentials with the same username and sign on realm already exists.
    kAlreadyExisits,
    // Password was empty.
    kEmptyPassword,
    kMaxValue = kEmptyPassword,
  };

  explicit SavedPasswordsPresenter(
      scoped_refptr<PasswordStoreInterface> profile_store,
      scoped_refptr<PasswordStoreInterface> account_store = nullptr);
  ~SavedPasswordsPresenter() override;

  // Initializes the presenter and makes it issue the first request for all
  // saved passwords.
  void Init();

  // Removes the credential and all its duplicates from the store.
  // TODO(crbug.com/1330906): Remove in favor of EditSavedCredentials.
  void RemovePassword(const PasswordForm& form);
  bool RemoveCredential(const CredentialUIEntry& credential);

  // Cancels the last removal operation.
  void UndoLastRemoval();

  // Adds the |credential| to the specified store. Returns true if the password
  // was added, false if |credential|'s data is not valid (invalid url/empty
  // password), or an entry with such signon_realm and username already exists
  // in any (profile or account) store.
  bool AddCredential(const CredentialUIEntry& credential);

  // Tries to edit |password|. After checking whether |form| is present in
  // |passwords_|, this will ask the password store to change the underlying
  // password_value to |new_password| in case it was found. This will also
  // notify clients that an edit event happened in case |form| was present
  // in |passwords_|.
  // TODO(crbug.com/1330906): Remove in favor of EditSavedCredentials.
  bool EditPassword(const PasswordForm& form, std::u16string new_password);

  // Modifies the provided password form and its duplicates
  // with `new_username` and `new_password`.
  //
  // Note: this will also change duplicates of 'form' in all stores.
  // TODO(crbug.com/1330906): Remove in favor of EditSavedCredentials.
  bool EditSavedPasswords(const PasswordForm& form,
                          const std::u16string& new_username,
                          const std::u16string& new_password);

  // Modifies all the saved credentials matching |original_credential| to
  // |updated_credential|. Only username, password, notes and password issues
  // are modifiable.
  EditResult EditSavedCredentials(const CredentialUIEntry& original_credential,
                                  const CredentialUIEntry& updated_credential);

  // Returns a list of the currently saved credentials.
  SavedPasswordsView GetSavedPasswords() const;

  // Returns a list of unique passwords which includes normal credentials,
  // federated credentials and blocked forms. If a same form is present both on
  // account and profile stores it will be represented as a single entity.
  // Uniqueness is determined using site name, username, password. For Android
  // credentials package name is also taken into account and for Federated
  // credentials federation origin.
  // TODO(crbug.com/1330906): Replace all API to work with CredentialUIEntry.
  std::vector<PasswordForm> GetUniquePasswordForms() const;
  std::vector<CredentialUIEntry> GetSavedCredentials() const;

  // Returns PasswordForms corresponding to |credential|.
  std::vector<PasswordForm> GetCorrespondingPasswordForms(
      const CredentialUIEntry& credential) const;

  // Allows clients and register and de-register themselves.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
  using DuplicatePasswordsMap = std::multimap<std::string, PasswordForm>;
  // PasswordStoreInterface::Observer
  void OnLoginsChanged(PasswordStoreInterface* store,
                       const PasswordStoreChangeList& changes) override;
  void OnLoginsRetained(
      PasswordStoreInterface* store,
      const std::vector<PasswordForm>& retained_passwords) override;

  // PasswordStoreConsumer:
  void OnGetPasswordStoreResults(
      std::vector<std::unique_ptr<PasswordForm>> results) override;
  void OnGetPasswordStoreResultsFrom(
      PasswordStoreInterface* store,
      std::vector<std::unique_ptr<PasswordForm>> results) override;

  // Notify observers about changes in the compromised credentials.
  void NotifyEdited(const PasswordForm& password);
  void NotifySavedPasswordsChanged();

  // Returns the `profile_store_` or `account_store_` if `form` is stored in the
  // profile store or the account store accordingly. This function should be
  // used only for credential stored in a single store.
  PasswordStoreInterface& GetStoreFor(const PasswordForm& form);

  // The password stores containing the saved passwords.
  scoped_refptr<PasswordStoreInterface> profile_store_;
  scoped_refptr<PasswordStoreInterface> account_store_;

  std::unique_ptr<PasswordUndoHelper> undo_helper_;

  // Cache of the most recently obtained saved passwords.
  std::vector<PasswordForm> passwords_;

  // Structure used to deduplicate list of passwords.
  DuplicatePasswordsMap sort_key_to_password_forms_;

  base::ObserverList<Observer, /*check_empty=*/true> observers_;

  base::WeakPtrFactory<SavedPasswordsPresenter> weak_ptr_factory_{this};
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_UI_SAVED_PASSWORDS_PRESENTER_H_
