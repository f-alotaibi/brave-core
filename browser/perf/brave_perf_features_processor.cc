// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/perf/brave_perf_features_processor.h"

#include "base/command_line.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/perf/brave_perf_switches.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_switches.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_pref_names.h"
#endif

namespace {
void EnableAdblockCookieList(base::WeakPtr<Profile> profile) {
  if (!profile) {
    return;
  }

  // Obtrusive cookie notices list in cosmetic filters.
  auto* component_service_manager =
      g_brave_browser_process->ad_block_service()->component_service_manager();
  if (!component_service_manager ||
      !component_service_manager->IsFilterListAvailable(
          brave_shields::kCookieListUuid)) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE, base::BindOnce(&EnableAdblockCookieList, profile),
        base::Seconds(1));
    return;
  }

  component_service_manager->EnableFilterList(brave_shields::kCookieListUuid,
                                              true);
}

}  // namespace

namespace perf {

void MaybeEnableBraveFeatureForPerfTesting(Profile* profile) {
  auto* cmd = base::CommandLine::ForCurrentProcess();
  if (!cmd->HasSwitch(switches::kEnableBraveFeaturesForPerfTesting) ||
      !cmd->HasSwitch(::switches::kUserDataDir)) {
    return;
  }

#if BUILDFLAG(ENABLE_SPEEDREADER)
  // Speedreader
  profile->GetPrefs()->SetBoolean(speedreader::kSpeedreaderPrefEnabled, true);
#endif

  // Adblock
  EnableAdblockCookieList(profile->GetWeakPtr());
}

}  // namespace perf
