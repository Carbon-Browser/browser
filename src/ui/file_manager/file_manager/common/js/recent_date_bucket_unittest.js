// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {loadTimeData} from 'chrome://resources/js/load_time_data.m.js';
import {assertEquals} from 'chrome://webui-test/chai_assert.js';

import {installMockChrome} from '../../common/js/mock_chrome.js';

import {getRecentDateBucket} from './recent_date_bucket.js';

export function setUp() {
  loadTimeData.resetForTesting({
    WEEK_START_FROM: 1,
  });

  const mockChrome = {
    fileManagerPrivate: {
      RecentDateBucket: {
        TODAY: 'today',
        YESTERDAY: 'yesterday',
        EARLIER_THIS_WEEK: 'earlier_this_week',
        EARLIER_THIS_MONTH: 'earlier_this_month',
        EARLIER_THIS_YEAR: 'earlier_this_year',
        OLDER: 'older',
      },
    },
  };
  installMockChrome(mockChrome);
}

export function testGetRecentDateBucket() {
  assertEquals(
      getRecentDateBucket(undefined, new Date()),
      chrome.fileManagerPrivate.RecentDateBucket.OLDER);

  // May 9, 2022, Monday, 12:00pm Local time.
  const middleOfMonth = new Date(2022, 4, 9, 12, 0, 0);
  // Mar 1, 2022, Tuesday, 8:00am Local time.
  const startOfMonth = new Date(2022, 2, 1, 8, 0, 0);
  // Jan 1, 2022, Saturday, 4:00pm Local time.
  const startOfYear = new Date(2022, 0, 1, 16, 0, 0);

  const testSamples = [
    {
      today: middleOfMonth,
      tests: [
        {
          // Future date: June 1, 2022, Wednesday, 12:00pm Local time.
          date: new Date(2022, 5, 1, 12, 0, 0),
          bucket: chrome.fileManagerPrivate.RecentDateBucket.TODAY,
        },
        {
          // May 9, 2022, Monday, 08:30am Local time.
          date: new Date(2022, 4, 9, 8, 30, 0),
          bucket: chrome.fileManagerPrivate.RecentDateBucket.TODAY,
        },
        {
          // May 8, 2022, Sunday, 10:30am Local time.
          date: new Date(2022, 4, 8, 10, 30, 0),
          bucket: chrome.fileManagerPrivate.RecentDateBucket.YESTERDAY,
        },
        {
          // May 7, 2022, Saturday, 07:30am Local time.
          date: new Date(2022, 4, 7, 7, 30, 0),
          bucket: chrome.fileManagerPrivate.RecentDateBucket.EARLIER_THIS_MONTH,
        },
        {
          // May 1, 2022, Monday, 10:30am Local time.
          date: new Date(2022, 4, 1, 10, 30, 0),
          bucket: chrome.fileManagerPrivate.RecentDateBucket.EARLIER_THIS_MONTH,
        },
        {
          // April 28, 2022, Thursday, 10:30am Local time.
          date: new Date(2022, 3, 28, 10, 30, 0),
          bucket: chrome.fileManagerPrivate.RecentDateBucket.EARLIER_THIS_YEAR,
        },
      ],
    },

    {
      today: startOfMonth,
      tests: [
        {
          // Future date: Mar 1, 2022, Tuesday, 12:00pm Local time.
          date: new Date(2022, 2, 1, 12, 0, 0),
          bucket: chrome.fileManagerPrivate.RecentDateBucket.TODAY,
        },
        {
          // Mar 1, 2022, Tuesday, 3:00am Local time.
          date: new Date(2022, 2, 1, 3, 0, 0),
          bucket: chrome.fileManagerPrivate.RecentDateBucket.TODAY,
        },
        {
          // Feb 28, 2022, Monday, 08:30am Local time.
          date: new Date(2022, 1, 28, 8, 30, 0),
          bucket: chrome.fileManagerPrivate.RecentDateBucket.YESTERDAY,
        },
        {
          // Feb 27, 2022, Sunday, 10:30am Local time.
          date: new Date(2022, 1, 27, 10, 30, 0),
          bucket: chrome.fileManagerPrivate.RecentDateBucket.EARLIER_THIS_YEAR,
        },
        {
          // Feb 10, 2022, Thursday, 10:30am Local time.
          date: new Date(2022, 1, 10, 10, 30, 0),
          bucket: chrome.fileManagerPrivate.RecentDateBucket.EARLIER_THIS_YEAR,
        },
      ],
    },

    {
      today: startOfYear,
      tests: [
        {

          // Future date: Jan 2, 2022, Sunday, 12:00pm Local time.
          date: new Date(2022, 0, 2, 12, 0, 0),
          bucket: chrome.fileManagerPrivate.RecentDateBucket.TODAY,
        },
        {
          // Jan 1, 2022, Saturday, 08:30am Local time.
          date: new Date(2022, 0, 1, 8, 30, 0),
          bucket: chrome.fileManagerPrivate.RecentDateBucket.TODAY,
        },
        {
          // Dec 31, 2021, Friday, 10:30am Local time.
          date: new Date(2021, 11, 31, 10, 30, 0),
          bucket: chrome.fileManagerPrivate.RecentDateBucket.YESTERDAY,
        },
        {
          // Dec 27, 2021, Monday, 07:30am Local time.
          date: new Date(2021, 11, 27, 7, 30, 0),
          bucket: chrome.fileManagerPrivate.RecentDateBucket.EARLIER_THIS_WEEK,
        },
        {
          // Dec 2, 2021, Thursday, 10:30am Local time.
          date: new Date(2021, 11, 2, 10, 30, 0),
          bucket: chrome.fileManagerPrivate.RecentDateBucket.OLDER,
        },
        {
          // Nov 28, 2021, Sunday, 10:30am Local time.
          date: new Date(2021, 10, 28, 10, 30, 0),
          bucket: chrome.fileManagerPrivate.RecentDateBucket.OLDER,
        },
      ],
    },

  ];

  for (const testSample of testSamples) {
    for (const test of testSample.tests) {
      assertEquals(
          getRecentDateBucket(test.date, testSample.today), test.bucket);
    }
  }
}
