/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */


#define ShouldTrackURLForRestore ShouldTrackURLForRestore_ChromiumImpl

#include "src/chrome/browser/sessions/session_common_utils.cc"

#undef ShouldTrackURLForRestore

bool ShouldTrackURLForRestore(const GURL& url) {
  return ShouldTrackURLForRestore_ChromiumImpl(url);
}
