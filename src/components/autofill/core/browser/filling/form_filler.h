// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_FILLING_FORM_FILLER_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_FILLING_FORM_FILLER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "components/autofill/core/browser/data_model/autofill_profile.h"
#include "components/autofill/core/browser/data_model/credit_card.h"
#include "components/autofill/core/browser/filling/field_filling_skip_reason.h"
#include "components/autofill/core/browser/filling/filling_product.h"
#include "components/autofill/core/browser/filling/form_autofill_history.h"
#include "components/autofill/core/browser/foundations/autofill_client.h"
#include "components/autofill/core/browser/foundations/autofill_driver.h"
#include "components/autofill/core/common/autofill_constants.h"

namespace autofill {

class BrowserAutofillManager;

// Denotes the reason for triggering a refill attempt.
// These values are persisted to UMA logs. Entries should not be renumbered and
// numeric values should never be reused. Keep this enum up to date with the one
// in tools/metrics/histograms/metadata/autofill/enums.xml.
enum class RefillTriggerReason {
  kFormChanged = 0,
  kSelectOptionsChanged = 1,
  kExpirationDateFormatted = 2,
  kMaxValue = kExpirationDateFormatted
};

using AutofillAiFillingPayload = base::flat_map<FieldGlobalId, std::u16string>;
using FillingPayload = absl::variant<const AutofillProfile*,
                                     const CreditCard*,
                                     AutofillAiFillingPayload>;

// Helper class responsible for [re]filling forms and fields.
//
// It is privately owned by the BrowserAutofillManager, which is the only
// component that talks to it.
//
// It receives cached data and is responsible for either filling directly or
// triggering a refill (which eventually results in a filling operation), and
// then sending the filled form to the renderer (via AutofillDriver).
//
// Additionally, it provides an API to determine which fields will be
// filled/skipped based on the given context.
//
// The class is directly responsible for modifying the cached fields in the form
// cache (BrowserAutofillManager::form_structures_) since it receives references
// to cached fields and modifies some attributes during filling.
//
// The class is also indirectly responsible for modifying blink forms, since
// after filling FormData objects it sends them to the renderer, which is
// directly responsible for filling the web forms.
//
// It holds any state that is only relevant for [re]filling.
class FormFiller {
 public:
  explicit FormFiller(BrowserAutofillManager& manager);

  FormFiller(const FormFiller&) = delete;
  FormFiller& operator=(const FormFiller&) = delete;

  virtual ~FormFiller();

  // Given `field`, the corresponding `autofill_field` to fill, and the
  // `trigger_field`, return the set of all reasons for that field to be skipped
  // for filling. If the field should not be skipped, an empty set is returned
  // (and not {FieldFillingSkipReason::kNotSkipped}).
  // `type_count` tracks the number of times a type of field has been filled.
  // `type_group_originally_filled` denotes, in case of a refill, what groups
  // where filled in the initial filling.
  // `filling_product` is the type of filling calling this function.
  // TODO(crbug.com/40281552): Make `type_groups_originally_filled` also a
  // FieldTypeSet.
  // TODO(crbug.com/40227496): Keep only one of 'field' and 'autofill_field'.
  static DenseSet<FieldFillingSkipReason> GetFillingSkipReasonsForField(
      const FormFieldData& field,
      const AutofillField& autofill_field,
      const AutofillField& trigger_field,
      base::flat_map<FieldType, size_t>& type_count,
      std::optional<DenseSet<FieldTypeGroup>> type_group_originally_filled,
      FillingProduct filling_product,
      bool is_refill = false);

  // Resets states that FormFiller holds and maintains.
  void Reset();

  base::TimeDelta get_limit_before_refill() { return limit_before_refill_; }

  // Given a `form`, returns a map from each field's id to the skip reason for
  // that field. See additional comments in GetFieldFillingSkipReason.
  // TODO(crbug.com/40227496): Keep only one of 'form' and 'form_structure'.
  // TODO(crbug.com/40281552): Make `type_groups_originally_filled` also a
  // FieldTypeSet.
  base::flat_map<FieldGlobalId, DenseSet<FieldFillingSkipReason>>
  GetFieldFillingSkipReasons(
      base::span<const FormFieldData> fields,
      const FormStructure& form_structure,
      const AutofillField& trigger_field,
      std::optional<DenseSet<FieldTypeGroup>> type_groups_originally_filled,
      FillingProduct filling_product,
      bool is_refill) const;

  // Reverts the last autofill operation on `form` that affected
  // `trigger_field`. `renderer_action` denotes whether this is an actual
  // filling or a preview operation on the renderer side. Returns the filling
  // product of the operation being undone.
  FillingProduct UndoAutofill(mojom::ActionPersistence action_persistence,
                              FormData form,
                              FormStructure& form_structure,
                              const FormFieldData& trigger_field);

  // Records filling information if possible and routes back to the renderer.
  void FillOrPreviewField(mojom::ActionPersistence action_persistence,
                          mojom::FieldActionType action_type,
                          const FormFieldData& field,
                          AutofillField* autofill_field,
                          const std::u16string& value,
                          FillingProduct filling_product,
                          std::optional<FieldType> field_type_used);

  // Fills or previews the data from `filling_payload` into `form`.
  // TODO(crbug.com/40227071): Clean up the API.
  void FillOrPreviewForm(
      mojom::ActionPersistence action_persistence,
      const FormData& form,
      const FillingPayload& filling_payload,
      FormStructure& form_structure,
      AutofillField& autofill_field,
      DenseSet<FieldFillingSkipReason> ignorable_skip_reasons,
      AutofillTriggerSource trigger_source,
      bool is_refill = false);

  // Whether there should be an attempts to refill the form. Returns true if all
  // the following are satisfied:
  // - There have been no refills on this page yet.
  // - A non-empty form name was recorded in a previous fill
  // - That form name matched the currently parsed form name
  // - It's been less than kLimitBeforeRefill since the original fill.
  // - `refill_trigger_reason != kFormChanged`, or `form_structure` and the
  //   previously filled form have different structures.
  bool ShouldTriggerRefill(const FormStructure& form_structure,
                           RefillTriggerReason refill_trigger_reason);

  // Schedules a call of TriggerRefill. Virtual for testing.
  virtual void ScheduleRefill(const FormData& form,
                              const FormStructure& form_structure,
                              AutofillTriggerSource trigger_source,
                              RefillTriggerReason refill_trigger_reason);

  // Attempts to refill the form that was changed dynamically. Should only be
  // called if ShouldTriggerRefill returns true.
  void TriggerRefill(const FormData& form,
                     AutofillTriggerSource trigger_source,
                     RefillTriggerReason refill_trigger_reason);

  // This function is called by JavaScriptChangedAutofilledValue and may trigger
  // a refill in case the website used JavaScript to reformat an expiration date
  // like "05/2023" into "05 / 20" (i.e. it broke the year by cutting the last
  // two digits instead of stripping the first two digits).
  void MaybeTriggerRefillForExpirationDate(
      const FormData& form,
      const FormFieldData& field,
      const FormStructure& form_structure,
      const std::u16string& old_value,
      AutofillTriggerSource trigger_source);

 private:
  friend class FormFillerTestApi;

  // Keeps track of the filling context for a form, used to make refill
  // attempts.
  struct RefillContext {
    // |filling_payload| contains the data used to perform the initial filling
    // operation.
    RefillContext(const AutofillField& field,
                  const FillingPayload& filling_payload);
    ~RefillContext();

    // Whether a refill attempt was made.
    bool attempted_refill = false;
    // The profile or credit card that was used for the initial fill. This is
    // slightly different from `filling_payload` that is used by the filling
    // function: This contains actual objects because this needs to survive
    // potential storage mutation, and this only contains payloads that support
    // refills.
    absl::variant<CreditCard, AutofillProfile> profile_or_credit_card;
    // Possible identifiers of the field that was focused when the form was
    // initially filled. A refill shall be triggered from the same field.
    const FieldGlobalId filled_field_id;
    const FieldSignature filled_field_signature;
    // The security origin from which the field was filled.
    url::Origin filled_origin;
    // The time at which the initial fill occurred.
    // TODO(crbug.com/41490871): Remove in favor of
    // FormStructure::last_filling_timestamp_.
    const base::TimeTicks original_fill_time;
    // The timer used to trigger a refill.
    base::OneShotTimer on_refill_timer;
    // The field type groups that were initially filled.
    DenseSet<FieldTypeGroup> type_groups_originally_filled;
    // If populated, this map determines which values will be filled into a
    // field (it does not matter whether the field already contains a value).
    // TODO(crbug.com/40947225): Investigate removing when
    // `AutofillFixCachingOnJavaScriptChanges` launches.
    std::map<FieldGlobalId, std::u16string> forced_fill_values;
    // The form filled in the first attempt for filling. Used to check whether
    // a refill should be attempted upon parsing an updated FormData.
    std::optional<FormData> filled_form;
  };

  void SetRefillContext(FormGlobalId form_id,
                        std::unique_ptr<RefillContext> context);

  RefillContext* GetRefillContext(FormGlobalId form_id);

  // Stores the value to be filled into a field, along with its field type and
  // if it's an override.
  struct FieldFillingData {
    std::u16string value_to_fill;
    std::optional<FieldType> field_type;
    bool value_is_an_override;
  };

  // Returns the value to fill along with the field type and if the value is an
  // override.
  FieldFillingData GetFieldFillingData(
      const AutofillField& autofill_field,
      const FillingPayload& filling_payload,
      const std::map<FieldGlobalId, std::u16string>& forced_fill_values,
      const FormFieldData& field_data,
      mojom::ActionPersistence action_persistence,
      std::string* failure_to_fill);

  // Fills `field_data` and modifies `autofill_field` given all other states.
  // Returns true if the field has been filled, false otherwise. This is
  // independent of whether the field was filled or autofilled before.
  // TODO(crbug.com/40227071): Cleanup API and logic.
  bool FillField(
      AutofillField& autofill_field,
      const FillingPayload& filling_payload,
      const std::map<FieldGlobalId, std::u16string>& forced_fill_values,
      FormFieldData& field_data,
      mojom::ActionPersistence action_persistence,
      std::string* failure_to_fill);

  LogManager* log_manager();

  // Container holding the history of Autofill filling operations. Used to undo
  // some of the filling operations.
  FormAutofillHistory form_autofill_history_;

  // A map from FormGlobalId to RefillContext instances used to make refill
  // attempts for dynamic forms.
  std::map<FormGlobalId, std::unique_ptr<RefillContext>> refill_context_;

  // The maximum amount of time between a change in the form and the original
  // fill that triggers a refill. This value is only changed in browser tests,
  // where time cannot be mocked, to avoid flakiness.
  base::TimeDelta limit_before_refill_ = kLimitBeforeRefill;

  const raw_ref<BrowserAutofillManager> manager_;

  base::WeakPtrFactory<FormFiller> weak_ptr_factory_{this};
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_FILLING_FORM_FILLER_H_
