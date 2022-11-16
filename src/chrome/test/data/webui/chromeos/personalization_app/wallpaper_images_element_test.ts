// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'chrome://personalization/strings.m.js';
import 'chrome://webui-test/mojo_webui_test_support.js';

import {OnlineImageType, PersonalizationRouter, WallpaperImages} from 'chrome://personalization/trusted/personalization_app.js';
import {assertDeepEquals, assertEquals, assertNotEquals, assertTrue} from 'chrome://webui-test/chai_assert.js';
import {waitAfterNextRender} from 'chrome://webui-test/test_util.js';

import {baseSetup, initElement, teardownElement} from './personalization_app_test_utils.js';
import {TestPersonalizationStore} from './test_personalization_store.js';
import {TestWallpaperProvider} from './test_wallpaper_interface_provider.js';

suite('WallpaperImagesTest', function() {
  let wallpaperImagesElement: WallpaperImages|null;
  let wallpaperProvider: TestWallpaperProvider;
  let personalizationStore: TestPersonalizationStore;

  setup(() => {
    const mocks = baseSetup();
    wallpaperProvider = mocks.wallpaperProvider;
    personalizationStore = mocks.personalizationStore;
  });

  teardown(async () => {
    await teardownElement(wallpaperImagesElement);
    wallpaperImagesElement = null;
  });

  async function createWithDefaultData() {
    const collectionId = wallpaperProvider.collections![0]!.id;
    personalizationStore.data.wallpaper = {
      ...personalizationStore.data.wallpaper,
      backdrop: {
        collections: wallpaperProvider.collections,
        images: {[collectionId]: wallpaperProvider.images},
      },
      loading: {
        ...personalizationStore.data.wallpaper.loading,
        collections: false,
        images: {[collectionId]: false},
      },
      currentSelected: wallpaperProvider.currentWallpaper,
    };
    const element = initElement(WallpaperImages, {collectionId});
    await waitAfterNextRender(element);
    return element;
  }

  test('sets aria-selected for current wallpaper asset id', async () => {
    wallpaperImagesElement = await createWithDefaultData();
    const selectedElement: HTMLDivElement =
        wallpaperImagesElement.shadowRoot!.querySelector(
            `.photo-inner-container[aria-selected='true']`)!;

    assertEquals(
        selectedElement!.dataset['assetId'],
        personalizationStore.data.wallpaper.currentSelected.key,
        'aria selected element has correct asset id');

    const notSelectedElements: HTMLDivElement[] =
        Array.from(wallpaperImagesElement.shadowRoot!.querySelectorAll(
            `.photo-inner-container[aria-selected='false']`));

    const uniqueUnitIds =
        new Set(wallpaperProvider.images!.map(img => img.unitId));

    assertEquals(
        uniqueUnitIds.size - 1, notSelectedElements.length,
        'correct number of non-selected elements');

    for (const notSelected of notSelectedElements) {
      assertTrue(
          notSelected.dataset['assetId']!.length > 0,
          'not selected elements have an assetId');

      assertNotEquals(
          wallpaperProvider.currentWallpaper.key,
          notSelected.dataset['assetId'],
          'not selected elements have a different assetId');
    }
  });

  test('displays images for current collectionId', async () => {
    personalizationStore.data.wallpaper.backdrop.images = {
      'id_0': [
        {
          assetId: BigInt(1),
          attribution: ['Image 0-1'],
          unitId: BigInt(1),
          url: {url: 'https://id_0-1/'},
        },
        {
          assetId: BigInt(2),
          attribution: ['Image 0-2'],
          unitId: BigInt(2),
          url: {url: 'https://id_0-2/'},
        },
      ],
      'id_1': [
        {
          assetId: BigInt(10),
          attribution: ['Image 1-10'],
          unitId: BigInt(10),
          url: {url: 'https://id_1-10/'},
        },
        {
          assetId: BigInt(20),
          attribution: ['Image 1-20'],
          unitId: BigInt(20),
          url: {url: 'https://id_1-20/'},
        },
      ],
    };
    personalizationStore.data.wallpaper.backdrop.collections =
        wallpaperProvider.collections;
    personalizationStore.data.wallpaper.loading.images = {
      'id_0': false,
      'id_1': false,
    };
    personalizationStore.data.wallpaper.loading.collections = false;

    wallpaperImagesElement =
        initElement(WallpaperImages, {collectionId: 'id_0'});
    await waitAfterNextRender(wallpaperImagesElement);

    assertDeepEquals(
        ['1', '2'],
        Array
            .from(
                wallpaperImagesElement.shadowRoot!
                    .querySelectorAll<HTMLDivElement>('.photo-inner-container'))
            .map(elem => elem.dataset['assetId']),
        'expected asset ids are displayed for collectionId `id_0`');

    wallpaperImagesElement.collectionId = 'id_1';
    await waitAfterNextRender(wallpaperImagesElement);

    assertDeepEquals(
        ['10', '20'],
        Array
            .from(
                wallpaperImagesElement.shadowRoot!
                    .querySelectorAll<HTMLDivElement>('.photo-inner-container'))
            .map(elem => elem.dataset['assetId']),
        'expected asset ids are displayed for collectionId `id_1`');
  });

  test('displays dark light tile for images with same unitId', async () => {
    wallpaperImagesElement = await createWithDefaultData();

    const elements =
        Array.from(wallpaperImagesElement.shadowRoot!.querySelectorAll(
            '.photo-inner-container'));

    assertDeepEquals(
        ['Image 0 light', 'Image 2'], elements.map(elem => elem.ariaLabel),
        'elements have correct aria labels for light mode');

    assertDeepEquals(
        [
          'chrome://image/?https://images.googleusercontent.com/1',
          'chrome://image/?https://images.googleusercontent.com/0',
        ],
        Array.from(elements[0]!.querySelectorAll('img')).map(img => img.src),
        'dark/light mode image has dark light variant urls');

    assertEquals(
        OnlineImageType.kLight,
        wallpaperProvider.images!
            .find(image => image.url.url.endsWith('.com/1'))!.type,
        'light image is first');

    assertDeepEquals(
        ['chrome://image/?https://images.googleusercontent.com/2'],
        Array.from(elements[1]!.querySelectorAll('img')).map(img => img.src),
        'image 2 does not have dark light mode variants');
  });

  test('selects an image when clicked', async () => {
    wallpaperImagesElement = await createWithDefaultData();
    wallpaperImagesElement.shadowRoot!
        .querySelector<HTMLDivElement>(
            `.photo-inner-container[data-asset-id='2']`)!.click();
    const [assetId, previewMode] =
        await wallpaperProvider.whenCalled('selectWallpaper');
    assertEquals(2n, assetId, 'correct asset id is passed');
    assertEquals(
        wallpaperProvider.isInTabletModeResponse, previewMode,
        'preview mode is same as tablet mode');
  });

  test('redirects to wallpaper page if no images', async () => {
    const reloadOriginal = PersonalizationRouter.reloadAtWallpaper;
    const reloadPromise = new Promise<void>(resolve => {
      PersonalizationRouter.reloadAtWallpaper = resolve;
    });
    wallpaperImagesElement = await createWithDefaultData();
    // Set all collections to have null images.
    personalizationStore.data.wallpaper.backdrop.images =
        Object.keys(personalizationStore.data.wallpaper.backdrop.images)
            .reduce((result, next) => {
              result[next] = null;
              return result;
            }, {} as Record<string, null>);
    personalizationStore.notifyObservers();

    await reloadPromise;

    PersonalizationRouter.reloadAtWallpaper = reloadOriginal;
  });
});
