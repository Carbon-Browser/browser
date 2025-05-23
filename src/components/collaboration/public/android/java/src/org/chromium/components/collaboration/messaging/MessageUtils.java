// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.collaboration.messaging;

import androidx.annotation.Nullable;

import org.chromium.base.Token;
import org.chromium.components.data_sharing.GroupMember;

/** Provides functions to safely read fields out of messages, performing null checks. */
public class MessageUtils {

    /** No instantiation. */
    private MessageUtils() {}

    /** Returns the id of the tab, or -1. */
    public static int extractTabId(@Nullable PersistentMessage message) {
        return message == null
                        || message.attribution == null
                        || message.attribution.tabMetadata == null
                ? TabMessageMetadata.INVALID_TAB_ID
                : message.attribution.tabMetadata.localTabId;
    }

    /** Returns the id of the group, or null. */
    public static @Nullable Token extractTabGroupId(@Nullable PersistentMessage message) {
        return message == null ? null : extractTabGroupId(message.attribution);
    }

    /** Returns the id of the group, or null. */
    public static @Nullable Token extractTabGroupId(@Nullable InstantMessage message) {
        return message == null ? null : extractTabGroupId(message.attribution);
    }

    /** Returns the sync id of the group, or null. */
    public static @Nullable String extractSyncTabGroupId(@Nullable InstantMessage message) {
        return message == null
                        || message.attribution == null
                        || message.attribution.tabGroupMetadata == null
                ? null
                : message.attribution.tabGroupMetadata.syncTabGroupId;
    }

    /** Returns the given name or the empty string. */
    public static String extractGivenName(@Nullable InstantMessage message) {
        GroupMember member = extractMember(message);
        return member == null ? "" : member.givenName;
    }

    /** Returns the tab title or the empty string. */
    public static String extractTabTitle(@Nullable InstantMessage message) {
        return message == null
                        || message.attribution == null
                        || message.attribution.tabMetadata == null
                ? ""
                : message.attribution.tabMetadata.lastKnownTitle;
    }

    /** Returns the tab group title or the empty string. */
    public static String extractTabGroupTitle(@Nullable InstantMessage message) {
        return message == null
                        || message.attribution == null
                        || message.attribution.tabGroupMetadata == null
                ? ""
                : message.attribution.tabGroupMetadata.lastKnownTitle;
    }

    private static @Nullable Token extractTabGroupId(@Nullable MessageAttribution attribution) {
        return attribution == null
                        || attribution.tabGroupMetadata == null
                        || attribution.tabGroupMetadata.localTabGroupId == null
                ? null
                : attribution.tabGroupMetadata.localTabGroupId.tabGroupId;
    }

    /** Returns the collaboration id or null. */
    public static String extractCollaborationId(@Nullable InstantMessage message) {
        return message == null || message.attribution == null
                ? null
                : message.attribution.collaborationId;
    }

    /** Returns a GroupMember associated with the message, prioritizing affected over triggering. */
    public static GroupMember extractMember(@Nullable InstantMessage message) {
        if (message == null || message.attribution == null) {
            return null;
        } else if (message.attribution.affectedUser != null) {
            return message.attribution.affectedUser;
        } else {
            return message.attribution.triggeringUser;
        }
    }

    /** Returns the url of the tab or null. */
    public static String extractTabUrl(@Nullable InstantMessage message) {
        return message == null
                        || message.attribution == null
                        || message.attribution.tabMetadata == null
                ? null
                : message.attribution.tabMetadata.lastKnownUrl;
    }
}
