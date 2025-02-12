// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/settings/brave_clear_browsing_data_handler.h"

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

namespace settings {

BraveClearBrowsingDataHandler::BraveClearBrowsingDataHandler(
    content::WebUI* webui,
    Profile* profile)
    : ClearBrowsingDataHandler(webui, profile), profile_(profile) {
  CHECK(profile_);

  pref_change_registrar_.Init(profile_->GetPrefs());
}

BraveClearBrowsingDataHandler::~BraveClearBrowsingDataHandler() = default;

void BraveClearBrowsingDataHandler::RegisterMessages() {
  ClearBrowsingDataHandler::RegisterMessages();
}

}  // namespace settings
