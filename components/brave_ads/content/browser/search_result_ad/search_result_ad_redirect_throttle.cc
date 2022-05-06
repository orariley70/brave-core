/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_redirect_throttle.h"

#include <utility>

#include "brave/components/brave_ads/common/search_result_ad_util.h"
#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_service.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "components/sessions/content/session_tab_helper.h"
#include "services/network/public/cpp/request_mode.h"
#include "services/network/public/cpp/resource_request.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

SessionID GetTabId(content::WebContents* web_contents) {
  DCHECK(web_contents);

  SessionID tab_id = sessions::SessionTabHelper::IdForTab(web_contents);
  content::RenderFrameHost* opener_rfh = web_contents->GetOpener();
  if (!opener_rfh) {
    opener_rfh = web_contents->GetOriginalOpener();
  }
  if (opener_rfh) {
    content::WebContents* original_web_contents =
        content::WebContents::FromRenderFrameHost(opener_rfh);
    tab_id = sessions::SessionTabHelper::IdForTab(original_web_contents);
  }

  return tab_id;
}

}  // namespace

// static
std::unique_ptr<SearchResultAdRedirectThrottle>
SearchResultAdRedirectThrottle::MaybeCreateThrottleFor(
    SearchResultAdService* search_result_ad_service,
    const network::ResourceRequest& request,
    content::WebContents* web_contents) {
  DCHECK(web_contents);

  if (!search_result_ad_service || !request.request_initiator ||
      !request.has_user_gesture || !request.is_outermost_main_frame ||
      request.resource_type !=
          static_cast<int>(blink::mojom::ResourceType::kMainFrame)) {
    return nullptr;
  }

  if (!brave_search::IsAllowedHost(request.request_initiator->GetURL()) ||
      !IsSearchResultAdClickedConfirmationUrl(request.url)) {
    return nullptr;
  }

  SessionID tab_id = GetTabId(web_contents);
  if (!tab_id.is_valid()) {
    return nullptr;
  }

  return std::make_unique<SearchResultAdRedirectThrottle>(
      search_result_ad_service, tab_id);
}

SearchResultAdRedirectThrottle::SearchResultAdRedirectThrottle(
    SearchResultAdService* search_result_ad_service,
    SessionID tab_id)
    : search_result_ad_service_(search_result_ad_service), tab_id_(tab_id) {
  DCHECK(search_result_ad_service_);
  DCHECK(tab_id_.is_valid());
}

SearchResultAdRedirectThrottle::~SearchResultAdRedirectThrottle() = default;

void SearchResultAdRedirectThrottle::WillStartRequest(
    network::ResourceRequest* request,
    bool* defer) {
  DCHECK(request);
  DCHECK(request->has_user_gesture);
  DCHECK(request->is_outermost_main_frame);
  DCHECK_EQ(request->resource_type,
            static_cast<int>(blink::mojom::ResourceType::kMainFrame));
  DCHECK(request->request_initiator);
  DCHECK(brave_search::IsAllowedHost(request->request_initiator->GetURL()));

  const std::string creative_instance_id =
      GetClickedSearchResultAdCreativeInstanceId(*request);
  if (creative_instance_id.empty()) {
    return;
  }

  const absl::optional<GURL> search_result_ad_target_url =
      search_result_ad_service_->MaybeTriggerSearchResultAdClickedEvent(
          creative_instance_id, tab_id_);
  if (!search_result_ad_target_url) {
    return;
  }
  DCHECK(search_result_ad_target_url->is_valid());
  DCHECK(search_result_ad_target_url->SchemeIs(url::kHttpsScheme));

  request->url = *search_result_ad_target_url;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->referrer_policy = net::ReferrerPolicy::NO_REFERRER;
}

}  // namespace brave_ads
