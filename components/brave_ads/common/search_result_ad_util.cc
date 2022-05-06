/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/common/search_result_ad_util.h"

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/json/json_reader.h"
#include "base/strings/string_piece.h"
#include "base/values.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"
#include "url/third_party/mozilla/url_parse.h"

namespace brave_ads {

namespace {

constexpr auto kSearchResultAdsConfirmationVettedHosts =
    base::MakeFixedFlatSet<base::StringPiece>(
        {"search.anonymous.ads.brave.com",
         "search.anonymous.ads.bravesoftware.com"});
constexpr char kSearchResultAdsViewedPath[] = "/v3/confirmation";
constexpr char kSearchResultAdsClickedPath[] = "/v3/click";
constexpr char kCreativeInstanceIdParameterName[] = "creativeInstanceId";
constexpr char kTypeParameterName[] = "type";
constexpr char kTypeViewParameterValue[] = "view";

bool IsSearchResultAdConfirmationUrl(const GURL& url, base::StringPiece path) {
  if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme) ||
      url.path_piece() != path) {
    return false;
  }

  if (!base::Contains(kSearchResultAdsConfirmationVettedHosts,
                      url.host_piece())) {
    return false;
  }

  return true;
}

std::string GetUploadData(const network::ResourceRequest& request) {
  std::string upload_data;
  if (!request.request_body) {
    return {};
  }
  const auto* elements = request.request_body->elements();
  for (const network::DataElement& element : *elements) {
    if (element.type() == network::mojom::DataElementDataView::Tag::kBytes) {
      const auto& bytes = element.As<network::DataElementBytes>().bytes();
      upload_data.append(bytes.begin(), bytes.end());
    }
  }

  return upload_data;
}

}  // namespace

bool IsSearchResultAdViewedConfirmationUrl(const GURL& url) {
  return IsSearchResultAdConfirmationUrl(url, kSearchResultAdsViewedPath);
}

std::string GetViewedSearchResultAdCreativeInstanceId(
    const network::ResourceRequest& request) {
  if (!IsSearchResultAdViewedConfirmationUrl(request.url) ||
      request.method != net::HttpRequestHeaders::kPostMethod) {
    return {};
  }

  const std::string payload_json = GetUploadData(request);
  const absl::optional<base::Value> payload_value =
      base::JSONReader::Read(payload_json);
  if (!payload_value) {
    return {};
  }

  const base::Value::Dict* payload_dict = payload_value->GetIfDict();
  if (!payload_dict) {
    return {};
  }

  const std::string* type = payload_dict->FindString(kTypeParameterName);
  if (!type || *type != kTypeViewParameterValue) {
    return {};
  }

  const std::string* creative_instance_id =
      payload_dict->FindString(kCreativeInstanceIdParameterName);
  if (!creative_instance_id) {
    return {};
  }

  return *creative_instance_id;
}

bool IsSearchResultAdClickedConfirmationUrl(const GURL& url) {
  return IsSearchResultAdConfirmationUrl(url, kSearchResultAdsClickedPath);
}

std::string GetClickedSearchResultAdCreativeInstanceId(
    const network::ResourceRequest& request) {
  const GURL& url = request.url;
  if (!IsSearchResultAdClickedConfirmationUrl(url) || !url.has_query() ||
      request.method != net::HttpRequestHeaders::kGetMethod) {
    return {};
  }

  base::StringPiece query_str = url.query_piece();
  url::Component query(0, static_cast<int>(query_str.length())), key, value;
  while (url::ExtractQueryKeyValue(query_str.data(), &query, &key, &value)) {
    base::StringPiece key_str = query_str.substr(key.begin, key.len);
    if (key_str == kCreativeInstanceIdParameterName) {
      base::StringPiece value_str = query_str.substr(value.begin, value.len);
      return static_cast<std::string>(value_str);
    }
  }

  return {};
}

}  // namespace brave_ads
