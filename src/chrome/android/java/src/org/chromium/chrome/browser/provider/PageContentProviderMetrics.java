// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.provider;

import androidx.annotation.IntDef;

import org.chromium.base.metrics.RecordHistogram;

public class PageContentProviderMetrics {

    @IntDef({
        PageContentProviderEvent.GET_CONTENT_URI_FAILED,
        PageContentProviderEvent.QUERY,
        PageContentProviderEvent.QUERY_FAILED_CURRENT_TAB_CHANGED,
        PageContentProviderEvent.QUERY_FAILED_INVALID_URL,
        PageContentProviderEvent.QUERY_FAILED_INVALID_ID,
        PageContentProviderEvent.QUERY_FAILED_TO_GET_CURRENT_TAB,
        PageContentProviderEvent.QUERY_SUCCEEDED_ALREADY_EXTRACTING,
        PageContentProviderEvent.QUERY_SUCCEEDED_RETURNED_EXTRACTED,
        PageContentProviderEvent.QUERY_SUCCEEDED_STARTED_EXTRACTION,
        PageContentProviderEvent.TEXT_EXTRACTION_FAILED_EMPTY_RESULT,
        PageContentProviderEvent.TEXT_EXTRACTION_FAILED_TAB_CHANGED,
        PageContentProviderEvent.TEXT_EXTRACTION_SUCCEEDED,
        PageContentProviderEvent.TIMEOUT,
    })
    public static @interface PageContentProviderEvent {

        int GET_CONTENT_URI_FAILED = 0;
        int QUERY = 1;
        int QUERY_FAILED_CURRENT_TAB_CHANGED = 2;
        int QUERY_FAILED_INVALID_URL = 3;
        int QUERY_FAILED_INVALID_ID = 4;
        int QUERY_FAILED_TO_GET_CURRENT_TAB = 5;
        int QUERY_SUCCEEDED_ALREADY_EXTRACTING = 6;
        int QUERY_SUCCEEDED_RETURNED_EXTRACTED = 7;
        int QUERY_SUCCEEDED_STARTED_EXTRACTION = 8;
        int TEXT_EXTRACTION_FAILED_EMPTY_RESULT = 9;
        int TEXT_EXTRACTION_FAILED_TAB_CHANGED = 10;
        int TEXT_EXTRACTION_SUCCEEDED = 11;
        int TIMEOUT = 12;
        int NUM_ENTRIES = 13;
    }

    public static void recordPageProviderEvent(@PageContentProviderEvent int event) {
        RecordHistogram.recordEnumeratedHistogram(
                "Android.AssistContent.WebPageContentProvider.Events",
                event,
                PageContentProviderEvent.NUM_ENTRIES);
    }

    public static void recordUrlAttachedToAssistContent(boolean urlAttached) {
        RecordHistogram.recordBooleanHistogram("Android.AssistContent.AttachedUrl", urlAttached);
    }

    public static void recordPdfStructuredDataAttachedToAssistContent(
            boolean pdfStructuredDataAttached) {
        RecordHistogram.recordBooleanHistogram(
                "Android.AssistContent.StructuredDataAttachedSuccess.Pdf",
                pdfStructuredDataAttached);
    }

    public static void recordWebStructuredDataAttachedToAssistContent(
            boolean webStructuredDataAttached) {
        RecordHistogram.recordBooleanHistogram(
                "Android.AssistContent.StructuredDataAttachedSuccess.WebPage",
                webStructuredDataAttached);
    }
}
