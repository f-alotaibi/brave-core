/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/brave_renderer_updater.h"

#include <utility>

#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "brave/common/brave_renderer_configuration.mojom.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/de_amp/browser/de_amp_util.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "brave/components/playlist/browser/pref_names.h"
#include "brave/components/playlist/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_process_host.h"
#include "extensions/buildflags/buildflags.h"
#include "ipc/ipc_channel_proxy.h"
#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/extension_registry.h"
#endif

BraveRendererUpdater::BraveRendererUpdater(
    Profile* profile,
    PrefService* local_state)
    : profile_(profile),
      local_state_(local_state) {
  PrefService* pref_service = profile->GetPrefs();
  de_amp_enabled_.Init(de_amp::kDeAmpPrefEnabled, pref_service);
#if BUILDFLAG(ENABLE_TOR)
  onion_only_in_tor_windows_.Init(tor::prefs::kOnionOnlyInTorWindows,
                                  pref_service);
#endif

  pref_change_registrar_.Init(pref_service);
  pref_change_registrar_.Add(
      de_amp::kDeAmpPrefEnabled,
      base::BindRepeating(&BraveRendererUpdater::UpdateAllRenderers,
                          base::Unretained(this)));
#if BUILDFLAG(ENABLE_TOR)
  pref_change_registrar_.Add(
      tor::prefs::kOnionOnlyInTorWindows,
      base::BindRepeating(&BraveRendererUpdater::UpdateAllRenderers,
                          base::Unretained(this)));
#endif

#if BUILDFLAG(ENABLE_WIDEVINE)
  if (local_state_) {
    widevine_enabled_.Init(kWidevineEnabled, local_state_);
    local_state_change_registrar_.Init(local_state_);
    local_state_change_registrar_.Add(
        kWidevineEnabled,
        base::BindRepeating(&BraveRendererUpdater::UpdateAllRenderers,
                            base::Unretained(this)));
  } else {
    CHECK_IS_TEST();
  }
#endif

  pref_change_registrar_.Add(
      playlist::kPlaylistEnabledPref,
      base::BindRepeating(&BraveRendererUpdater::UpdateAllRenderers,
                          base::Unretained(this)));
}

BraveRendererUpdater::~BraveRendererUpdater() = default;

void BraveRendererUpdater::InitializeRenderer(
    content::RenderProcessHost* render_process_host) {
  auto renderer_configuration = GetRendererConfiguration(render_process_host);
  Profile* profile =
      Profile::FromBrowserContext(render_process_host->GetBrowserContext());
  renderer_configuration->SetInitialConfiguration(profile->IsTor());
  UpdateRenderer(&renderer_configuration);
}

std::vector<mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>>
BraveRendererUpdater::GetRendererConfigurations() {
  std::vector<mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>>
      rv;
  for (content::RenderProcessHost::iterator it(
           content::RenderProcessHost::AllHostsIterator());
       !it.IsAtEnd(); it.Advance()) {
    Profile* renderer_profile =
        static_cast<Profile*>(it.GetCurrentValue()->GetBrowserContext());
    if (renderer_profile == profile_ ||
        renderer_profile->GetOriginalProfile() == profile_) {
      auto renderer_configuration =
          GetRendererConfiguration(it.GetCurrentValue());
      if (renderer_configuration) {
        rv.push_back(std::move(renderer_configuration));
      }
    }
  }
  return rv;
}

mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>
BraveRendererUpdater::GetRendererConfiguration(
    content::RenderProcessHost* render_process_host) {
  IPC::ChannelProxy* channel = render_process_host->GetChannel();
  if (!channel) {
    return mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>();
  }

  mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>
      renderer_configuration;
  channel->GetRemoteAssociatedInterface(&renderer_configuration);

  return renderer_configuration;
}

void BraveRendererUpdater::UpdateAllRenderers() {
  auto renderer_configurations = GetRendererConfigurations();
  for (auto& renderer_configuration : renderer_configurations) {
    UpdateRenderer(&renderer_configuration);
  }
}

void BraveRendererUpdater::UpdateRenderer(
    mojo::AssociatedRemote<brave::mojom::BraveRendererConfiguration>*
        renderer_configuration) {

  PrefService* pref_service = profile_->GetPrefs();
  bool de_amp_enabled = de_amp::IsDeAmpEnabled(pref_service);
  bool onion_only_in_tor_windows = true;
#if BUILDFLAG(ENABLE_TOR)
  onion_only_in_tor_windows =
      pref_service->GetBoolean(tor::prefs::kOnionOnlyInTorWindows);
#endif
  bool widevine_enabled = false;
#if BUILDFLAG(ENABLE_WIDEVINE)
  if (local_state_) {
    widevine_enabled = local_state_->GetBoolean(kWidevineEnabled);
  } else {
    CHECK_IS_TEST();
  }
#endif

  const bool playlist_enabled =
      base::FeatureList::IsEnabled(playlist::features::kPlaylist) &&
      pref_service->GetBoolean(playlist::kPlaylistEnabledPref);

  (*renderer_configuration)
      ->SetConfiguration(brave::mojom::DynamicParams::New(
          de_amp_enabled,
          onion_only_in_tor_windows, widevine_enabled, playlist_enabled));
}
