/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_pages.h"

#include "base/strings/strcat.h"
#include "brave/browser/ui/webui/webcompat_reporter/webcompat_reporter_dialog.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/sidebar/browser/constants.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/common/webui_url_constants.h"
#include "url/gurl.h"

namespace brave {

void ShowBraveAdblock(Browser* browser) {
  ShowSingletonTabOverwritingNTP(browser, GURL(kBraveUIAdblockURL));
}

void ShowSync(Browser* browser) {
  ShowSingletonTabOverwritingNTP(
      browser, chrome::GetSettingsUrl(chrome::kSyncSetupSubPage));
}

void ShowBraveNewsConfigure(Browser* browser) {
  ShowSingletonTabOverwritingNTP(
      browser, GURL("brave://newtab/?openSettings=BraveNews"));
}

void ShowShortcutsPage(Browser* browser) {
  ShowSingletonTabOverwritingNTP(browser, GURL(kShortcutsURL));
}

void ShowWebcompatReporter(Browser* browser) {
  content::WebContents* web_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  if (!web_contents) {
    return;
  }

  webcompat_reporter::OpenReporterDialog(
      web_contents, webcompat_reporter::UISource::kAppMenu);
}

void ShowExtensionSettings(Browser* browser) {
  ShowSingletonTabOverwritingNTP(browser, GURL(kExtensionSettingsURL));
}

void ShowAppsPage(Browser* browser) {
  ShowSingletonTabOverwritingNTP(browser, GURL(chrome::kChromeUIAppsURL));
}

}  // namespace brave
