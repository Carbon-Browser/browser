// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

class TestOperation {
  async run(data) {
    {{RUN_FUNCTION_BODY}}
  }
}

register("test-operation", TestOperation);
