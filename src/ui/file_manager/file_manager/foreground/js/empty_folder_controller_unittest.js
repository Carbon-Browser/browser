// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


import {loadTimeData} from 'chrome://resources/js/load_time_data.m.js';
import {assertEquals, assertFalse, assertTrue} from 'chrome://webui-test/chai_assert.js';

import {FakeEntryImpl} from '../../common/js/files_app_entry_types.js';
import {installMockChrome} from '../../common/js/mock_chrome.js';
import {util} from '../../common/js/util.js';
import {VolumeManagerCommon} from '../../common/js/volume_manager_types.js';
import {FakeEntry} from '../../externs/files_app_entry_interfaces.js';

import {DirectoryModel} from './directory_model.js';
import {EmptyFolderController} from './empty_folder_controller.js';
import {FileListModel} from './file_list_model.js';
import {MockMetadataModel} from './metadata/mock_metadata.js';
import {createFakeDirectoryModel} from './mock_directory_model.js';


/**
 * @type {!HTMLElement}
 */
let element;

/**
 * @type {!DirectoryModel}
 */
let directoryModel;

/**
 * @type {!FileListModel}
 */
let fileListModel;

/**
 * @type {!FakeEntry}
 */
let recentEntry;

/**
 * @type {!EmptyFolderController}
 */
let emptyFolderController;

export function setUp() {
  // Mock loadTimeData strings.
  loadTimeData.resetForTesting({
    RECENT_EMPTY_AUDIO_FOLDER: 'no audio',
    RECENT_EMPTY_DOCUMENTS_FOLDER: 'no documents',
    RECENT_EMPTY_IMAGES_FOLDER: 'no images',
    RECENT_EMPTY_VIDEOS_FOLDER: 'no videos',
    RECENT_EMPTY_FOLDER: 'no files',
  });

  /**
   * Mock chrome APIs.
   * @type {!Object}
   */
  const mockChrome = {
    fileManagerPrivate: {
      SourceRestriction: {
        ANY_SOURCE: 'any_source',
        NATIVE_SOURCE: 'native_source',
      },
      RecentFileType: {
        ALL: 'all',
        AUDIO: 'audio',
        IMAGE: 'image',
        VIDEO: 'video',
        DOCUMENT: 'document',
      },
    },
  };

  installMockChrome(mockChrome);

  // Create EmptyFolderController instance with dependencies.
  element = /** @type {!HTMLElement} */ (document.createElement('div'));
  util.createChild(element, 'label', 'span');
  directoryModel = createFakeDirectoryModel();
  fileListModel = new FileListModel(new MockMetadataModel({}));
  directoryModel.getFileList = () => fileListModel;
  recentEntry = new FakeEntryImpl(
      'Recent', VolumeManagerCommon.RootType.RECENT,
      chrome.fileManagerPrivate.SourceRestriction.ANY_SOURCE,
      chrome.fileManagerPrivate.RecentFileType.ALL);
  emptyFolderController =
      new EmptyFolderController(element, directoryModel, recentEntry);
}

/**
 * Tests that no files message will be rendered for each filter type.
 * @suppress {accessControls} access private method/property in test.
 */
export function testNoFilesMessage() {
  // Mock current directory to Recent.
  directoryModel.getCurrentRootType = () => VolumeManagerCommon.RootType.RECENT;

  // For all filter.
  emptyFolderController.updateUI_();
  assertFalse(element.hidden);
  assertEquals('no files', emptyFolderController.label_.innerText);
  // For audio filter.
  recentEntry.recentFileType = chrome.fileManagerPrivate.RecentFileType.AUDIO;
  emptyFolderController.updateUI_();
  assertFalse(element.hidden);
  assertEquals('no audio', emptyFolderController.label_.innerText);
  // For document filter.
  recentEntry.recentFileType =
      chrome.fileManagerPrivate.RecentFileType.DOCUMENT;
  emptyFolderController.updateUI_();
  assertFalse(element.hidden);
  assertEquals('no documents', emptyFolderController.label_.innerText);
  // For image filter.
  recentEntry.recentFileType = chrome.fileManagerPrivate.RecentFileType.IMAGE;
  emptyFolderController.updateUI_();
  assertFalse(element.hidden);
  assertEquals('no images', emptyFolderController.label_.innerText);
  // For video filter.
  recentEntry.recentFileType = chrome.fileManagerPrivate.RecentFileType.VIDEO;
  emptyFolderController.updateUI_();
  assertFalse(element.hidden);
  assertEquals('no videos', emptyFolderController.label_.innerText);
}

/**
 * Tests that no files message will be hidden for non-Recent entries.
 * @suppress {accessControls} access private method in test.
 */
export function testHiddenForNonRecent() {
  // Mock current directory to Downloads.
  directoryModel.getCurrentRootType = () =>
      VolumeManagerCommon.RootType.DOWNLOADS;

  emptyFolderController.updateUI_();
  assertTrue(element.hidden);
  assertEquals('', emptyFolderController.label_.innerText);
}

/**
 * Tests that no files message will be hidden if scanning is in progress.
 * @suppress {accessControls} access private method in test.
 */
export function testHiddenForScanning() {
  // Mock current directory to Recent.
  directoryModel.getCurrentRootType = () => VolumeManagerCommon.RootType.RECENT;
  // Mock scanning.
  emptyFolderController.isScanning_ = true;

  emptyFolderController.updateUI_();
  assertTrue(element.hidden);
  assertEquals('', emptyFolderController.label_.innerText);
}

/**
 * Tests that no files message will be hidden if there are files in the list.
 * @suppress {accessControls} access private method in test.
 */
export function testHiddenForFiles() {
  // Mock current directory to Recent.
  directoryModel.getCurrentRootType = () => VolumeManagerCommon.RootType.RECENT;
  // Current file list has 1 item.
  fileListModel.push({name: 'a.txt', isDirectory: false, toURL: () => 'a.txt'});

  emptyFolderController.updateUI_();
  assertTrue(element.hidden);
  assertEquals('', emptyFolderController.label_.innerText);
}
