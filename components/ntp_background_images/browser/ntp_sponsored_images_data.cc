/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"

#include <optional>
#include <utility>

#include "base/logging.h"
#include "base/ranges/algorithm.h"
#include "base/strings/stringprintf.h"
#include "base/uuid.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "content/public/common/url_constants.h"

/* Sample photo.json.
{
  "schemaVersion": 1,
  "campaignId": "fb7ee174-5430-4fb9-8e97-29bf14e8d828",
  "logo": {
    "imageUrl": "logo.png",
    "alt": "Visit Brave Software",
    "companyName": "Brave Software",
    "destinationUrl": "https://www.brave.com/"
  },
  "wallpapers": [
    {
      "imageUrl": "background-1.jpg",
      "focalPoint": {
        "x": 1468,
        "y": 720
      }
    },
    {
      "imageUrl": "background-2.jpg",
      "focalPoint": {
        "x": 1650,
        "y": 720
      },
      "viewbox": {
        "x": 1578,
        "y": 1200,
        "height": 600,
        "width": 800
      },
      "backgroundColor": "#FFFFFF",
      "creativeInstanceId": "3e47ee7a-8d2d-445b-8e60-d987fdeea613",
      "logo": {
        "imageUrl": "logo-2.png",
        "alt": "basic attention token",
        "companyName": "BAT",
        "destinationUrl": "https://basicattentiontoken.org/"
      }
    }
  ]
*/

namespace ntp_background_images {

namespace {

constexpr int kExpectedSchemaVersion = 1;

Logo GetLogoFromValue(const base::FilePath& installed_dir,
                      const std::string& url_prefix,
                      const base::Value::Dict& value) {
  Logo logo;

  if (auto* url = value.FindString(kImageURLKey)) {
    logo.image_file = installed_dir.AppendASCII(*url);
    logo.image_url = url_prefix + *url;
  }

  if (auto* alt_text = value.FindString(kAltKey)) {
    logo.alt_text = *alt_text;
  }

  if (auto* name = value.FindString(kCompanyNameKey)) {
    logo.company_name = *name;
  }

  if (auto* url = value.FindString(kDestinationURLKey)) {
    logo.destination_url = *url;
  }

  return logo;
}

}  // namespace

TopSite::TopSite() = default;
TopSite::TopSite(const std::string& i_name,
                 const std::string& i_destination_url,
                 const std::string& i_image_path,
                 const base::FilePath& i_image_file)
    : name(i_name),
      destination_url(i_destination_url),
      image_path(i_image_path),
      image_file(i_image_file) {}
TopSite& TopSite::operator=(const TopSite& data) = default;
TopSite::TopSite(const TopSite& data) = default;
TopSite::~TopSite() = default;

bool TopSite::IsValid() const {
  return !name.empty() && !destination_url.empty() && !image_file.empty();
}

Logo::Logo() = default;
Logo::Logo(const Logo&) = default;
Logo::~Logo() = default;

SponsoredBackground::SponsoredBackground() = default;
SponsoredBackground::SponsoredBackground(
    const base::FilePath& file_path,
    const gfx::Point& point,
    const Logo& test_logo,
    const std::string& creative_instance_id)
    : file_path(file_path),
      focal_point(point),
      creative_instance_id(creative_instance_id),
      logo(test_logo) {}
SponsoredBackground::SponsoredBackground(const SponsoredBackground&) = default;
SponsoredBackground::~SponsoredBackground() = default;

Campaign::Campaign() = default;
Campaign::~Campaign() = default;
Campaign::Campaign(const Campaign&) = default;
Campaign& Campaign::operator=(const Campaign&) = default;

bool Campaign::IsValid() const {
  return !backgrounds.empty();
}

NTPSponsoredImagesData::NTPSponsoredImagesData() = default;
NTPSponsoredImagesData::NTPSponsoredImagesData(
    const base::Value::Dict& data,
    const base::FilePath& installed_dir)
    : NTPSponsoredImagesData() {
  std::optional<int> incomingSchemaVersion = data.FindInt(kSchemaVersionKey);
  const bool schemaVersionIsValid =
      incomingSchemaVersion && *incomingSchemaVersion == kExpectedSchemaVersion;
  if (!schemaVersionIsValid) {
    DVLOG(2) << __func__ << "Incoming NTP background images data was not valid."
             << " Schema version was "
             << (incomingSchemaVersion ? std::to_string(*incomingSchemaVersion)
                                       : "missing")
             << ", but we expected " << kExpectedSchemaVersion;
    return;
  }

  url_prefix = base::StringPrintf("%s://%s/", content::kChromeUIScheme,
                                  kBrandedWallpaperHost);
  if (auto* name = data.FindString(kThemeNameKey)) {
    theme_name = *name;
    url_prefix += kSuperReferralPath;
  } else {
    url_prefix += kSponsoredImagesPath;
  }

  // SmartNTTs are targeted locally by the browser and are only shown to users
  // if the configured conditions match. Non-smart capable browsers that predate
  // the introduction of this feature should never show these NTTs. To enforce
  // this, the existing `campaigns` array in `photo.json` never includes
  // SmartNTTs. A new `campaigns2` array is included in `photo.json`. This
  // includes all NTTs, including smart ones. SmartNTT capable browsers read the
  // `campaigns2` array, fall back to `campaigns`, and then fall back to the
  // root `campaign` for backward compatibility. Non-smart capable browsers
  // continue to read the `campaigns` array.
  if (auto* campaigns2_value = data.FindList(kCampaigns2Key)) {
    ParseCampaignsList(*campaigns2_value, installed_dir);
  } else if (auto* campaigns_value = data.FindList(kCampaignsKey)) {
    ParseCampaignsList(*campaigns_value, installed_dir);
  } else {
    // Get a global campaign directly if the campaign list doesn't exist.
    const auto campaign = GetCampaignFromValue(data, installed_dir);
    if (campaign.IsValid())
      campaigns.push_back(campaign);
  }

  ParseSRProperties(data, installed_dir);

  PrintCampaignsParsingResult();
}

NTPSponsoredImagesData& NTPSponsoredImagesData::operator=(
    const NTPSponsoredImagesData& data) = default;
NTPSponsoredImagesData::NTPSponsoredImagesData(
    const NTPSponsoredImagesData& data) = default;
NTPSponsoredImagesData::~NTPSponsoredImagesData() = default;

void NTPSponsoredImagesData::ParseCampaignsList(
    const base::Value::List& campaigns_value,
    const base::FilePath& installed_dir) {
  for (const auto& campaign_value : campaigns_value) {
    DCHECK(campaign_value.is_dict());
    const auto campaign =
        GetCampaignFromValue(campaign_value.GetDict(), installed_dir);
    if (campaign.IsValid())
      campaigns.push_back(campaign);
  }
}

Campaign NTPSponsoredImagesData::GetCampaignFromValue(
    const base::Value::Dict& value,
    const base::FilePath& installed_dir) {
  Campaign campaign;

  if (const std::string* campaign_id = value.FindString(kCampaignIdKey)) {
    campaign.campaign_id = *campaign_id;
  }

  Logo default_logo;
  if (auto* logo = value.FindDict(kLogoKey)) {
    default_logo = GetLogoFromValue(installed_dir, url_prefix, *logo);
  }

  if (auto* wallpapers = value.FindList(kWallpapersKey)) {
    for (const auto& entry : *wallpapers) {
      const auto& wallpaper = entry.GetDict();
      const std::string* image_url = wallpaper.FindString(kImageURLKey);
      if (!image_url) {
        continue;
      }

      SponsoredBackground background;
      background.file_path = installed_dir.AppendASCII(*image_url);

      if (auto* focal_point = wallpaper.FindDict(kWallpaperFocalPointKey)) {
        background.focal_point = {focal_point->FindInt(kXKey).value_or(0),
                                  focal_point->FindInt(kYKey).value_or(0)};
      }

      if (auto* viewbox = wallpaper.FindDict(kViewboxKey)) {
        gfx::Rect rect(viewbox->FindInt(kXKey).value_or(0),
                       viewbox->FindInt(kYKey).value_or(0),
                       viewbox->FindInt(kWidthKey).value_or(0),
                       viewbox->FindInt(kHeightKey).value_or(0));
        background.viewbox.emplace(rect);
      }
      if (auto* background_color = wallpaper.FindString(kBackgroundColorKey)) {
        background.background_color = *background_color;
      }
      if (auto* creative_instance_id =
              wallpaper.FindString(kCreativeInstanceIDKey)) {
        background.creative_instance_id = *creative_instance_id;
      }
      if (auto* wallpaper_logo = wallpaper.FindDict(kLogoKey)) {
        background.logo =
            GetLogoFromValue(installed_dir, url_prefix, *wallpaper_logo);
      } else {
        background.logo = default_logo;
      }
      campaign.backgrounds.push_back(background);
    }
  }

  return campaign;
}

void NTPSponsoredImagesData::ParseSRProperties(
    const base::Value::Dict& value,
    const base::FilePath& installed_dir) {
  if (theme_name.empty()) {
    DVLOG(2) << __func__ << ": Don't have NTP SR properties";
    return;
  }

  DVLOG(2) << __func__ << ": Theme name: " << theme_name;

  if (auto* sites = value.FindList(kTopSitesKey)) {
    for (const auto& item : *sites) {
      const auto& top_site_dict = item.GetDict();
      TopSite site;
      if (auto* name = top_site_dict.FindString(kTopSiteNameKey)) {
        site.name = *name;
      }

      if (auto* url = top_site_dict.FindString(kDestinationURLKey)) {
        site.destination_url = *url;
      }

      if (auto* color = top_site_dict.FindString(kBackgroundColorKey)) {
        site.background_color = *color;
      }

      if (auto* url = top_site_dict.FindString(kTopSiteIconURLKey)) {
        site.image_path = url_prefix + *url;
        site.image_file = installed_dir.AppendASCII(*url);
      }

      // TopSite should have all properties.
      DCHECK(site.IsValid());
      top_sites.push_back(site);
    }
  }
}

bool NTPSponsoredImagesData::IsValid() const {
  return !campaigns.empty();
}

bool NTPSponsoredImagesData::IsSuperReferral() const {
  return IsValid() && !theme_name.empty();
}

std::optional<base::Value::Dict> NTPSponsoredImagesData::GetBackgroundAt(
    size_t campaign_index,
    size_t background_index) {
  DCHECK(campaign_index < campaigns.size() && background_index >= 0 &&
         background_index < campaigns[campaign_index].backgrounds.size());

  const auto campaign = campaigns[campaign_index];
  if (!campaign.IsValid())
    return std::nullopt;

  base::Value::Dict data;
  data.Set(kThemeNameKey, theme_name);
  data.Set(kIsSponsoredKey, !IsSuperReferral());
  data.Set(kIsBackgroundKey, false);
  data.Set(kWallpaperIDKey, base::Uuid::GenerateRandomV4().AsLowercaseString());

  const auto background_file_path =
      campaign.backgrounds[background_index].file_path;
  const std::string wallpaper_image_url =
      url_prefix + background_file_path.BaseName().AsUTF8Unsafe();

  data.Set(kWallpaperImageURLKey, wallpaper_image_url);
  data.Set(kWallpaperImagePathKey, background_file_path.AsUTF8Unsafe());
  data.Set(kWallpaperFocalPointXKey,
           campaign.backgrounds[background_index].focal_point.x());
  data.Set(kWallpaperFocalPointYKey,
           campaign.backgrounds[background_index].focal_point.y());

  data.Set(kCampaignIdKey, campaign.campaign_id);
  data.Set(kCreativeInstanceIDKey,
           campaign.backgrounds[background_index].creative_instance_id);

  base::Value::Dict logo_data;
  Logo logo = campaign.backgrounds[background_index].logo;
  logo_data.Set(kImageKey, logo.image_url);
  logo_data.Set(kImagePathKey, logo.image_file.AsUTF8Unsafe());
  logo_data.Set(kCompanyNameKey, logo.company_name);
  logo_data.Set(kAltKey, logo.alt_text);
  logo_data.Set(kDestinationURLKey, logo.destination_url);
  data.Set(kLogoKey, std::move(logo_data));
  return data;
}

void NTPSponsoredImagesData::PrintCampaignsParsingResult() const {
  VLOG(2) << __func__ << ": This is "
          << (IsSuperReferral() ? " NTP SR Data" : " NTP SI Data");

  for (const auto& campaign : campaigns) {
    const auto& backgrounds = campaign.backgrounds;
    for (size_t j = 0; j < backgrounds.size(); ++j) {
      const auto& background = backgrounds[j];
      VLOG(2) << __func__ << ": background(" << j << " - "
              << background.logo.company_name
              << ") - id: " << background.creative_instance_id;
    }
  }
}

}  // namespace ntp_background_images
