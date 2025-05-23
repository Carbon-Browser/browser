// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/accessibility/ax_node_data.h"

#include <set>
#include <string>
#include <unordered_set>
#include <utility>

#include "base/containers/contains.h"
#include "base/test/gtest_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/accessibility/ax_enum_util.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_role_properties.h"
#include "ui/accessibility/ax_text_attributes.h"

namespace ui {

TEST(AXNodeDataTest, GetAndSetCheckedState) {
  AXNodeData root;
  EXPECT_EQ(ax::mojom::CheckedState::kNone, root.GetCheckedState());
  EXPECT_FALSE(root.HasIntAttribute(ax::mojom::IntAttribute::kCheckedState));

  root.SetCheckedState(ax::mojom::CheckedState::kMixed);
  EXPECT_EQ(ax::mojom::CheckedState::kMixed, root.GetCheckedState());
  EXPECT_TRUE(root.HasIntAttribute(ax::mojom::IntAttribute::kCheckedState));

  root.SetCheckedState(ax::mojom::CheckedState::kFalse);
  EXPECT_EQ(ax::mojom::CheckedState::kFalse, root.GetCheckedState());
  EXPECT_TRUE(root.HasIntAttribute(ax::mojom::IntAttribute::kCheckedState));

  root.SetCheckedState(ax::mojom::CheckedState::kNone);
  EXPECT_EQ(ax::mojom::CheckedState::kNone, root.GetCheckedState());
  EXPECT_FALSE(root.HasIntAttribute(ax::mojom::IntAttribute::kCheckedState));
}

TEST(AXNodeDataTest, TextAttributes) {
  AXNodeData node_1;
  node_1.AddFloatAttribute(ax::mojom::FloatAttribute::kFontSize, 1.5);

  AXNodeData node_2;
  node_2.AddFloatAttribute(ax::mojom::FloatAttribute::kFontSize, 1.5);
  EXPECT_TRUE(node_1.GetTextAttributes() == node_2.GetTextAttributes());

  node_2.AddIntAttribute(ax::mojom::IntAttribute::kColor, 100);
  EXPECT_TRUE(node_1.GetTextAttributes() != node_2.GetTextAttributes());

  node_1.AddIntAttribute(ax::mojom::IntAttribute::kColor, 100);
  EXPECT_TRUE(node_1.GetTextAttributes() == node_2.GetTextAttributes());

  node_2.RemoveIntAttribute(ax::mojom::IntAttribute::kColor);
  EXPECT_TRUE(node_1.GetTextAttributes() != node_2.GetTextAttributes());

  node_2.AddIntAttribute(ax::mojom::IntAttribute::kColor, 100);
  EXPECT_TRUE(node_1.GetTextAttributes() == node_2.GetTextAttributes());

  node_1.AddStringAttribute(ax::mojom::StringAttribute::kFontFamily,
                            "test font");
  EXPECT_TRUE(node_1.GetTextAttributes() != node_2.GetTextAttributes());

  node_2.AddStringAttribute(ax::mojom::StringAttribute::kFontFamily,
                            "test font");
  EXPECT_TRUE(node_1.GetTextAttributes() == node_2.GetTextAttributes());

  node_2.RemoveStringAttribute(ax::mojom::StringAttribute::kFontFamily);
  EXPECT_TRUE(node_1.GetTextAttributes() != node_2.GetTextAttributes());

  node_2.AddStringAttribute(ax::mojom::StringAttribute::kFontFamily,
                            "test font");
  EXPECT_TRUE(node_1.GetTextAttributes() == node_2.GetTextAttributes());

  node_2.AddStringAttribute(ax::mojom::StringAttribute::kFontFamily,
                            "different font");
  EXPECT_TRUE(node_1.GetTextAttributes() != node_2.GetTextAttributes());

  node_2.AddStringAttribute(ax::mojom::StringAttribute::kTooltip,
                            "test tooltip");
  EXPECT_TRUE(node_2.HasStringAttribute(ax::mojom::StringAttribute::kTooltip));
  const std::string& tooltip =
      node_2.GetStringAttribute(ax::mojom::StringAttribute::kTooltip);

  EXPECT_EQ(tooltip, "test tooltip");

  AXTextAttributes node1_attributes = node_1.GetTextAttributes();
  AXTextAttributes moved_attributes = std::move(node1_attributes);
  EXPECT_TRUE(node1_attributes != moved_attributes);
  EXPECT_TRUE(moved_attributes == node_1.GetTextAttributes());

  AXNodeData node_3;
  AXTextAttributes node3_attributes = node_3.GetTextAttributes();
  EXPECT_TRUE(node3_attributes.IsUnset());
  node_3.AddIntAttribute(ax::mojom::IntAttribute::kColor,
                         static_cast<int>(0xFFFF0000));
  node3_attributes = node_3.GetTextAttributes();
  EXPECT_FALSE(node3_attributes.IsUnset());

  AXNodeData node_4;
  AXTextAttributes node4_attributes = node_4.GetTextAttributes();
  EXPECT_TRUE(node4_attributes.IsUnset());
  const float kFontSize = 1001.5;
  node_4.AddFloatAttribute(ax::mojom::FloatAttribute::kFontSize, kFontSize);
  node4_attributes = node_4.GetTextAttributes();
  EXPECT_EQ(node4_attributes.font_size, kFontSize);
}

TEST(AXNodeDataTest, IsButtonPressed) {
  // A non-button element with CheckedState::kTrue should not return true for
  // IsButtonPressed.
  AXNodeData non_button_pressed;
  non_button_pressed.role = ax::mojom::Role::kGenericContainer;
  non_button_pressed.SetCheckedState(ax::mojom::CheckedState::kTrue);
  EXPECT_FALSE(IsButton(non_button_pressed.role));
  EXPECT_FALSE(non_button_pressed.IsButtonPressed());

  // A button element with CheckedState::kTrue should return true for
  // IsButtonPressed.
  AXNodeData button_pressed;
  button_pressed.role = ax::mojom::Role::kButton;
  button_pressed.SetCheckedState(ax::mojom::CheckedState::kTrue);
  EXPECT_TRUE(IsButton(button_pressed.role));
  EXPECT_TRUE(button_pressed.IsButtonPressed());

  button_pressed.role = ax::mojom::Role::kPopUpButton;
  EXPECT_TRUE(IsButton(button_pressed.role));
  EXPECT_TRUE(button_pressed.IsButtonPressed());

  button_pressed.role = ax::mojom::Role::kToggleButton;
  EXPECT_TRUE(IsButton(button_pressed.role));
  EXPECT_TRUE(button_pressed.IsButtonPressed());

  // A button element does not have CheckedState::kTrue should return false for
  // IsButtonPressed.
  AXNodeData button_not_pressed;
  button_not_pressed.role = ax::mojom::Role::kButton;
  button_not_pressed.SetCheckedState(ax::mojom::CheckedState::kNone);
  EXPECT_TRUE(IsButton(button_not_pressed.role));
  EXPECT_FALSE(button_not_pressed.IsButtonPressed());

  button_not_pressed.role = ax::mojom::Role::kPopUpButton;
  button_not_pressed.SetCheckedState(ax::mojom::CheckedState::kFalse);
  EXPECT_TRUE(IsButton(button_not_pressed.role));
  EXPECT_FALSE(button_not_pressed.IsButtonPressed());

  button_not_pressed.role = ax::mojom::Role::kToggleButton;
  button_not_pressed.SetCheckedState(ax::mojom::CheckedState::kMixed);
  EXPECT_TRUE(IsButton(button_not_pressed.role));
  EXPECT_FALSE(button_not_pressed.IsButtonPressed());
}

TEST(AXNodeDataTest, IsClickable) {
  // Test for ax node data attribute with a custom default action verb.
  AXNodeData data_default_action_verb;

  for (int action_verb_idx =
           static_cast<int>(ax::mojom::DefaultActionVerb::kMinValue);
       action_verb_idx <=
       static_cast<int>(ax::mojom::DefaultActionVerb::kMaxValue);
       action_verb_idx++) {
    data_default_action_verb.SetDefaultActionVerb(
        static_cast<ax::mojom::DefaultActionVerb>(action_verb_idx));
    bool is_clickable = data_default_action_verb.IsClickable();

    SCOPED_TRACE(testing::Message()
                 << "ax::mojom::DefaultActionVerb="
                 << ToString(data_default_action_verb.GetDefaultActionVerb())
                 << ", Actual: isClickable=" << is_clickable
                 << ", Expected: isClickable=" << !is_clickable);

    if (data_default_action_verb.GetDefaultActionVerb() ==
            ax::mojom::DefaultActionVerb::kClickAncestor ||
        data_default_action_verb.GetDefaultActionVerb() ==
            ax::mojom::DefaultActionVerb::kNone)
      EXPECT_FALSE(is_clickable);
    else
      EXPECT_TRUE(is_clickable);
  }

  // Test for iterating through all roles and validate if a role is clickable.
  std::set<ax::mojom::Role> roles_expected_is_clickable = {
      ax::mojom::Role::kButton,
      ax::mojom::Role::kCheckBox,
      ax::mojom::Role::kColorWell,
      ax::mojom::Role::kComboBoxMenuButton,
      ax::mojom::Role::kComboBoxSelect,
      ax::mojom::Role::kDate,
      ax::mojom::Role::kDateTime,
      ax::mojom::Role::kDisclosureTriangle,
      ax::mojom::Role::kDisclosureTriangleGrouped,
      ax::mojom::Role::kDocBackLink,
      ax::mojom::Role::kDocBiblioRef,
      ax::mojom::Role::kDocGlossRef,
      ax::mojom::Role::kDocNoteRef,
      ax::mojom::Role::kImeCandidate,
      ax::mojom::Role::kInputTime,
      ax::mojom::Role::kLink,
      ax::mojom::Role::kListBox,
      ax::mojom::Role::kListBoxOption,
      ax::mojom::Role::kMenuItem,
      ax::mojom::Role::kMenuItemCheckBox,
      ax::mojom::Role::kMenuItemRadio,
      ax::mojom::Role::kMenuListOption,
      ax::mojom::Role::kPdfActionableHighlight,
      ax::mojom::Role::kPopUpButton,
      ax::mojom::Role::kRadioButton,
      ax::mojom::Role::kSearchBox,
      ax::mojom::Role::kSpinButton,
      ax::mojom::Role::kSwitch,
      ax::mojom::Role::kTab,
      ax::mojom::Role::kTextField,
      ax::mojom::Role::kTextFieldWithComboBox,
      ax::mojom::Role::kToggleButton};

  AXNodeData data;

  for (int role_idx = static_cast<int>(ax::mojom::Role::kMinValue);
       role_idx <= static_cast<int>(ax::mojom::Role::kMaxValue); role_idx++) {
    data.role = static_cast<ax::mojom::Role>(role_idx);
    bool is_clickable = data.IsClickable();

    SCOPED_TRACE(testing::Message()
                 << "ax::mojom::Role=" << ToString(data.role)
                 << ", Actual: isClickable=" << is_clickable
                 << ", Expected: isClickable=" << !is_clickable);

    EXPECT_EQ(base::Contains(roles_expected_is_clickable, data.role),
              is_clickable);
  }
}

TEST(AXNodeDataTest, IsInvocable) {
  // Test for iterating through all roles and validate if a role is invocable.
  // A role is invocable if it is clickable and supports neither expand collapse
  // nor toggle. A link should always be invocable, regardless of whether it is
  // clickable or supports expand/collapse or toggle.
  AXNodeData data;
  for (int role_idx = static_cast<int>(ax::mojom::Role::kMinValue);
       role_idx <= static_cast<int>(ax::mojom::Role::kMaxValue); role_idx++) {
    data.role = static_cast<ax::mojom::Role>(role_idx);
    bool is_activatable = data.IsActivatable();
    const bool supports_expand_collapse = data.SupportsExpandCollapse();
    const bool supports_toggle = SupportsToggle(data.role);
    const bool is_clickable = data.IsClickable();
    const bool is_invocable = data.IsInvocable();

    SCOPED_TRACE(testing::Message()
                 << "ax::mojom::Role=" << ToString(data.role)
                 << ", isClickable=" << is_clickable << ", isActivatable="
                 << is_activatable << ", supportsToggle=" << supports_toggle
                 << ", supportsExpandCollapse=" << supports_expand_collapse
                 << ", Actual: isInvocable=" << is_invocable
                 << ", Expected: isInvocable=" << !is_invocable);

    if (IsLink(data.role) || (is_clickable && !is_activatable &&
                              !supports_toggle && !supports_expand_collapse)) {
      EXPECT_TRUE(is_invocable);
    } else {
      EXPECT_FALSE(is_invocable);
    }
  }
}

TEST(AXNodeDataTest, IsMenuButton) {
  // A non button element should return false for IsMenuButton.
  AXNodeData non_button;
  non_button.role = ax::mojom::Role::kGenericContainer;
  EXPECT_FALSE(IsButton(non_button.role));
  EXPECT_FALSE(non_button.IsMenuButton());

  // Only button element with HasPopup::kMenu should return true for
  // IsMenuButton. All other ax::mojom::HasPopup types should return false.
  AXNodeData button_with_popup;

  button_with_popup.role = ax::mojom::Role::kButton;

  for (int has_popup_idx = static_cast<int>(ax::mojom::HasPopup::kMinValue);
       has_popup_idx <= static_cast<int>(ax::mojom::HasPopup::kMaxValue);
       has_popup_idx++) {
    button_with_popup.SetHasPopup(
        static_cast<ax::mojom::HasPopup>(has_popup_idx));
    bool is_menu_button = button_with_popup.IsMenuButton();

    SCOPED_TRACE(testing::Message()
                 << "ax::mojom::Role=" << ToString(button_with_popup.role)
                 << ", hasPopup=" << button_with_popup.GetHasPopup()
                 << ", Actual: isMenuButton=" << is_menu_button
                 << ", Expected: isMenuButton=" << !is_menu_button);

    if (IsButton(button_with_popup.role) &&
        button_with_popup.GetHasPopup() == ax::mojom::HasPopup::kMenu)
      EXPECT_TRUE(is_menu_button);
    else
      EXPECT_FALSE(is_menu_button);
  }
}

TEST(AXNodeDataTest, SupportsExpandCollapse) {
  // Test for iterating through all hasPopup attributes and validate if a
  // hasPopup attribute supports expand collapse.
  AXNodeData data_has_popup;

  for (int has_popup_idx = static_cast<int>(ax::mojom::HasPopup::kMinValue);
       has_popup_idx <= static_cast<int>(ax::mojom::HasPopup::kMaxValue);
       has_popup_idx++) {
    data_has_popup.SetHasPopup(static_cast<ax::mojom::HasPopup>(has_popup_idx));
    bool supports_expand_collapse = data_has_popup.SupportsExpandCollapse();

    SCOPED_TRACE(testing::Message() << "ax::mojom::HasPopup="
                                    << ToString(data_has_popup.GetHasPopup())
                                    << ", Actual: supportsExpandCollapse="
                                    << supports_expand_collapse
                                    << ", Expected: supportsExpandCollapse="
                                    << !supports_expand_collapse);

    if (data_has_popup.GetHasPopup() == ax::mojom::HasPopup::kFalse)
      EXPECT_FALSE(supports_expand_collapse);
    else
      EXPECT_TRUE(supports_expand_collapse);
  }

  // Test for iterating through all states and validate if a state supports
  // expand collapse.
  AXNodeData data_state;

  for (int state_idx = static_cast<int>(ax::mojom::State::kMinValue);
       state_idx <= static_cast<int>(ax::mojom::State::kMaxValue);
       state_idx++) {
    ax::mojom::State state = static_cast<ax::mojom::State>(state_idx);

    // skipping kNone here because AXNodeData::AddState, RemoveState forbids
    // kNone to be added/removed and would fail DCHECK.
    if (state == ax::mojom::State::kNone)
      continue;

    data_state.AddState(state);

    bool supports_expand_collapse = data_state.SupportsExpandCollapse();

    SCOPED_TRACE(testing::Message() << "ax::mojom::State=" << ToString(state)
                                    << ", Actual: supportsExpandCollapse="
                                    << supports_expand_collapse
                                    << ", Expected: supportsExpandCollapse="
                                    << !supports_expand_collapse);

    if (data_state.HasState(ax::mojom::State::kExpanded) ||
        data_state.HasState(ax::mojom::State::kCollapsed))
      EXPECT_TRUE(supports_expand_collapse);
    else
      EXPECT_FALSE(supports_expand_collapse);

    data_state.RemoveState(state);
  }

  // Test for iterating through all roles and validate if a role supports expand
  // collapse.
  AXNodeData data;

  std::unordered_set<ax::mojom::Role> roles_expected_supports_expand_collapse =
      {ax::mojom::Role::kComboBoxGrouping,
       ax::mojom::Role::kComboBoxMenuButton,
       ax::mojom::Role::kComboBoxSelect,
       ax::mojom::Role::kDisclosureTriangle,
       ax::mojom::Role::kDisclosureTriangleGrouped,
       ax::mojom::Role::kTextFieldWithComboBox,
       ax::mojom::Role::kTreeItem};

  for (int role_idx = static_cast<int>(ax::mojom::Role::kMinValue);
       role_idx <= static_cast<int>(ax::mojom::Role::kMaxValue); role_idx++) {
    data.role = static_cast<ax::mojom::Role>(role_idx);
    bool supports_expand_collapse = data.SupportsExpandCollapse();

    SCOPED_TRACE(testing::Message() << "ax::mojom::Role=" << ToString(data.role)
                                    << ", Actual: supportsExpandCollapse="
                                    << supports_expand_collapse
                                    << ", Expected: supportsExpandCollapse="
                                    << !supports_expand_collapse);

    if (roles_expected_supports_expand_collapse.find(data.role) !=
        roles_expected_supports_expand_collapse.end())
      EXPECT_TRUE(supports_expand_collapse);
    else
      EXPECT_FALSE(supports_expand_collapse);
  }
}

TEST(AXNodeDataTest, SetName) {
  AXNodeData data;
  // SetName should not be called on a role of kUnknown. That role means the
  // role has not yet been set, and we need a role to identify the NameFrom on
  // objects where it has not been set. This is enforced by a DCHECK.
  EXPECT_DCHECK_DEATH(data.SetName("no role yet"));

  // SetName should not be called on a role of kNone. That role is used for
  // presentational objects which should not be included in the accessibility
  // tree. This is enforced by a DCHECK.
  data.role = ax::mojom::Role::kNone;
  EXPECT_DCHECK_DEATH(data.SetName("role is presentational"));

  // For roles other than text, setting the name should result in the NameFrom
  // source being kAttribute.
  data.role = ax::mojom::Role::kButton;
  data.SetName("foo");
  EXPECT_EQ("foo", data.GetStringAttribute(ax::mojom::StringAttribute::kName));
  EXPECT_EQ(data.GetNameFrom(), ax::mojom::NameFrom::kAttribute);

  // TODO(accessibility): The static text role should have a NameFrom source of
  // kContents. But nothing clears the NameFrom if the role of an existing
  // object changes because currently there is no AXNodeData::SetRole method.
  data.role = ax::mojom::Role::kStaticText;
  data.SetName("bar");
  EXPECT_EQ("bar", data.GetStringAttribute(ax::mojom::StringAttribute::kName));
  EXPECT_EQ(data.GetNameFrom(), ax::mojom::NameFrom::kAttribute);

  data.RemoveIntAttribute(ax::mojom::IntAttribute::kNameFrom);
  data.SetName("baz");
  EXPECT_EQ("baz", data.GetStringAttribute(ax::mojom::StringAttribute::kName));
  EXPECT_EQ(data.GetNameFrom(), ax::mojom::NameFrom::kContents);

  // Setting the name to the empty string should not be done by
  // `SetNameChecked`, which enforces that expectation with a DCHECK.
  EXPECT_DCHECK_DEATH(data.SetNameChecked(""));

  data.SetNameExplicitlyEmpty();
  EXPECT_EQ("", data.GetStringAttribute(ax::mojom::StringAttribute::kName));
  EXPECT_EQ(data.GetNameFrom(), ax::mojom::NameFrom::kAttributeExplicitlyEmpty);

  data.SetName("foo");
  EXPECT_EQ("foo", data.GetStringAttribute(ax::mojom::StringAttribute::kName));
  EXPECT_EQ(data.GetNameFrom(), ax::mojom::NameFrom::kContents);
}

TEST(AXNodeDataTest, SetDescription) {
  AXNodeData data;
  data.role = ax::mojom::Role::kButton;

  // Initially there is no description and no DescriptionFrom.
  EXPECT_EQ("",
            data.GetStringAttribute(ax::mojom::StringAttribute::kDescription));
  EXPECT_EQ(data.GetDescriptionFrom(), ax::mojom::DescriptionFrom::kNone);

  // When the DescriptionFrom is not specified it defaults to kAriaDescription.
  data.SetDescription("foo");
  EXPECT_EQ("foo",
            data.GetStringAttribute(ax::mojom::StringAttribute::kDescription));
  EXPECT_EQ(data.GetDescriptionFrom(),
            ax::mojom::DescriptionFrom::kAriaDescription);

  // Setting the description explicitly empty should both clear the string
  // and update the DescriptionFrom.
  data.SetDescriptionExplicitlyEmpty();
  EXPECT_EQ("",
            data.GetStringAttribute(ax::mojom::StringAttribute::kDescription));
  EXPECT_EQ(data.GetDescriptionFrom(),
            ax::mojom::DescriptionFrom::kAttributeExplicitlyEmpty);

  // We currently do not enforce that the description gets set prior to the
  // DescriptionFrom. As a result it is possible to have a DescriptionFrom
  // value other than kNone and kAttributeExplicitlyEmpty while the description
  // is an empty string.
  data.SetDescriptionFrom(ax::mojom::DescriptionFrom::kTitle);
  EXPECT_EQ("",
            data.GetStringAttribute(ax::mojom::StringAttribute::kDescription));
  EXPECT_EQ(data.GetDescriptionFrom(), ax::mojom::DescriptionFrom::kTitle);

  // Setting the description to the empty string should not be done by
  // SetDescription, which enforces that expectation with a DCHECK.
  EXPECT_DCHECK_DEATH(data.SetDescription(""));
}

TEST(AXNodeDataTest, BitFieldsConfidenceCheck) {
  EXPECT_LT(static_cast<size_t>(ax::mojom::State::kMaxValue),
            sizeof(AXNodeData::state) * 8);
  EXPECT_LT(static_cast<size_t>(ax::mojom::Action::kMaxValue),
            sizeof(AXNodeData::actions) * 8);
}

}  // namespace ui
