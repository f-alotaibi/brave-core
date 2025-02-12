/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ntp_background/ntp_p3a_helper_impl.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/metrics/histogram_functions.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/p3a/features.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/p3a_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace ntp_background_images {

namespace {

constexpr int kCountBuckets[] = {0, 1, 2, 3, 8, 12, 16};

constexpr char kCreativeViewEventKey[] = "views";
constexpr char kCreativeLandEventKey[] = "lands";

constexpr char kCreativeTotalCountHistogramName[] =
    "creativeInstanceId.total.count";

std::string BuildCreativeHistogramName(const std::string& creative_instance_id,
                                       const std::string& event_type) {
  return base::StrCat(
      {p3a::kCreativeMetricPrefix, creative_instance_id, ".", event_type});
}

std::string BuildCampaignHistogramName(const std::string& campaign_id,
                                       const std::string& event_type) {
  return base::StrCat(
      {p3a::kCampaignMetricPrefix, campaign_id, ".", event_type});
}

}  // namespace

NTPP3AHelperImpl::NTPP3AHelperImpl(
    PrefService* local_state,
    p3a::P3AService* p3a_service,
    ntp_background_images::NTPBackgroundImagesService*
        ntp_background_images_service,
    PrefService* prefs,
    bool use_uma_for_testing)
    : local_state_(local_state),
      p3a_service_(p3a_service),
      prefs_(prefs),
      is_json_deprecated_(
          p3a::features::IsJSONDeprecated(p3a::MetricLogType::kExpress)),
      use_uma_for_testing_(use_uma_for_testing) {
  DCHECK(local_state);
  DCHECK(p3a_service);
  DCHECK(prefs);
  metric_sent_subscription_ =
      p3a_service->RegisterMetricCycledCallback(base::BindRepeating(
          &NTPP3AHelperImpl::OnP3AMetricCycled, base::Unretained(this)));
  rotation_subscription_ =
      p3a_service->RegisterRotationCallback(base::BindRepeating(
          &NTPP3AHelperImpl::OnP3ARotation, base::Unretained(this)));
  if (ntp_background_images_service) {
    if (const auto* sr_data =
            ntp_background_images_service->GetBrandedImagesData(
                /*super_referral=*/true)) {
      CheckLoadedCampaigns(*sr_data);
    }
    if (const auto* si_data =
            ntp_background_images_service->GetBrandedImagesData(
                /*super_referral=*/false)) {
      CheckLoadedCampaigns(*si_data);
    }
    ntp_background_images_service_observation_.Observe(
        ntp_background_images_service);
  }
}

NTPP3AHelperImpl::~NTPP3AHelperImpl() = default;

void NTPP3AHelperImpl::RecordView(const std::string& creative_instance_id,
                                  const std::string& campaign_id) {
  if (!p3a_service_->IsP3AEnabled()) {
    return;
  }

  UpdateMetricCount(creative_instance_id, kCreativeViewEventKey);
}

void NTPP3AHelperImpl::SetLastTabURL(const GURL& url) {
  last_tab_hostname_ = url.host();
}

void NTPP3AHelperImpl::OnP3ARotation(p3a::MetricLogType log_type,
                                     bool is_constellation) {
  if (log_type != p3a::MetricLogType::kExpress) {
    return;
  }

  CleanOldCampaignsAndCreatives();

  if (!p3a_service_->IsP3AEnabled()) {
    return;
  }

  size_t total_active_creatives = 0;
  // Always send the creative total if ads are disabled (as per spec),
  // or send the total if there were outstanding events sent
  if (total_active_creatives > 0) {
    RecordCreativeMetric(kCreativeTotalCountHistogramName,
                         total_active_creatives, is_constellation);
  }
}

void NTPP3AHelperImpl::OnP3AMetricCycled(const std::string& histogram_name,
                                         bool is_constellation) {
  if (!histogram_name.starts_with(p3a::kCreativeMetricPrefix)) {
    return;
  }

  std::vector<std::string> histogram_name_tokens = base::SplitString(
      histogram_name, ".", base::WhitespaceHandling::KEEP_WHITESPACE,
      base::SplitResult::SPLIT_WANT_ALL);
  if (histogram_name_tokens.size() != 3) {
    return;
  }

  const std::string& creative_instance_id = histogram_name_tokens[1];
  const std::string& event_type = histogram_name_tokens[2];

  RemoveMetricIfInstanceDoesNotExist(histogram_name, event_type,
                                     creative_instance_id);
}

void NTPP3AHelperImpl::CleanOldCampaignsAndCreatives() {
  if (!p3a_service_->IsP3AEnabled()) {
    return;
  }
}

void NTPP3AHelperImpl::RecordCreativeMetric(const std::string& histogram_name,
                                            int count,
                                            bool is_constellation) {
  const int* it_count =
      std::lower_bound(kCountBuckets, std::end(kCountBuckets), count);
  int answer = it_count - kCountBuckets;
  if (use_uma_for_testing_) {
    if (!is_constellation) {
      base::UmaHistogramExactLinear(histogram_name, answer,
                                    std::size(kCountBuckets) + 1);
    }
    return;
  }
  p3a_service_->UpdateMetricValueForSingleFormat(histogram_name, answer,
                                                 is_constellation);
}

void NTPP3AHelperImpl::RemoveMetricIfInstanceDoesNotExist(
    const std::string& histogram_name,
    const std::string& event_type,
    const std::string& creative_instance_id) {
    p3a_service_->RemoveDynamicMetric(histogram_name);
}

void NTPP3AHelperImpl::UpdateMetricCount(
    const std::string& creative_instance_id,
    const std::string& event_type) {
  const std::string histogram_name =
      BuildCreativeHistogramName(creative_instance_id, event_type);

  p3a_service_->RegisterDynamicMetric(histogram_name,
                                      p3a::MetricLogType::kExpress);

}

void NTPP3AHelperImpl::UpdateCampaignMetric(const std::string& campaign_id,
                                            const std::string& event_type) {
  const std::string histogram_name =
      BuildCampaignHistogramName(campaign_id, event_type);

  p3a_service_->RegisterDynamicMetric(histogram_name,
                                      p3a::MetricLogType::kExpress);
  base::UmaHistogramBoolean(histogram_name, true);
}

void NTPP3AHelperImpl::OnLandingStartCheck(
    const std::string& creative_instance_id) {
  if (!last_tab_hostname_.has_value()) {
    return;
  }
}

void NTPP3AHelperImpl::OnLandingEndCheck(
    const std::string& creative_instance_id,
    const std::string& expected_hostname) {
  if (!last_tab_hostname_.has_value() ||
      last_tab_hostname_ != expected_hostname) {
    return;
  }
  UpdateMetricCount(creative_instance_id, kCreativeLandEventKey);
}

void NTPP3AHelperImpl::OnUpdated(NTPSponsoredImagesData* data) {
  CheckLoadedCampaigns(*data);
}

void NTPP3AHelperImpl::OnUpdated(NTPBackgroundImagesData* data) {}

void NTPP3AHelperImpl::OnSuperReferralEnded() {}

void NTPP3AHelperImpl::CheckLoadedCampaigns(
    const NTPSponsoredImagesData& data) {
  if (!p3a_service_->IsP3AEnabled()) {
    return;
  }
}

}  // namespace ntp_background_images
