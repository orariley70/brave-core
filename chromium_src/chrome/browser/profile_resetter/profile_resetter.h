/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILE_RESETTER_PROFILE_RESETTER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILE_RESETTER_PROFILE_RESETTER_H_

#define ResetDefaultSearchEngine      \
  virtual ResetDefaultSearchEngine(); \
  friend class BraveProfileResetter;  \
  void UnUsedMethod

#include "src/chrome/browser/profile_resetter/profile_resetter.h"

#undef ResetDefaultSearchEngine

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILE_RESETTER_PROFILE_RESETTER_H_
