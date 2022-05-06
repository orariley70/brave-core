/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/common/search_result_ad_util.h"
#include "base/strings/string_piece.h"
#include "services/network/public/cpp/resource_request.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_ads {

TEST(SearchResultAdUtilTest, CheckSearchResultAdViewedConfirmationUrl) {
  EXPECT_TRUE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.ads.brave.com/v3/confirmation")));
  EXPECT_TRUE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.ads.bravesoftware.com/v3/confirmation")));

  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("http://search.anonymous.ads.brave.com/v3/confirmation")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("http://search.anonymous.ads.bravesoftware.com/v3/confirmation")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.ads.brave.com/v4/confirmation")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.ads.bravesoftware.com/v4/confirmation")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.ads.brave.com/v3/non")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.ads.bravesoftware.com/v3/non")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.brave.com/v3/confirmation")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.bravesoftware.com/v3/confirmation")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.ads.brave.com/v3")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.ads.bravesoftware.com/v3")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.ads.brave.com")));
  EXPECT_FALSE(IsSearchResultAdViewedConfirmationUrl(
      GURL("https://search.anonymous.ads.bravesoftware.com")));
}

TEST(SearchResultAdUtilTest, CheckGetViewedSearchResultAdCreativeInstanceId) {
  network::ResourceRequest request;
  request.method = "POST";
  request.url = GURL("https://search.anonymous.ads.brave.com/v3/confirmation");
  base::StringPiece json_payload = R"({
    "type": "view",
    "creativeInstanceId": "creative-instance-id"
  })";
  request.request_body = network::ResourceRequestBody::CreateFromBytes(
      json_payload.data(), json_payload.size());
  EXPECT_EQ("creative-instance-id",
            GetViewedSearchResultAdCreativeInstanceId(request));

  network::ResourceRequest bad_request = request;
  bad_request.method = "GET";
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  bad_request.url = GURL("https://search.brave.com/v3/confirmation");
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  bad_request.request_body.reset();
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  json_payload = "";
  bad_request.request_body = network::ResourceRequestBody::CreateFromBytes(
      json_payload.data(), json_payload.size());
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  json_payload = "abc";
  bad_request.request_body = network::ResourceRequestBody::CreateFromBytes(
      json_payload.data(), json_payload.size());
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  json_payload = R"({
    "type": "view"
    "creativeInstanceId": "creative-instance-id"
  })";
  bad_request.request_body = network::ResourceRequestBody::CreateFromBytes(
      json_payload.data(), json_payload.size());
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  json_payload = R"([{
    "type": "view",
    "creativeInstanceId": "creative-instance-id"
  }])";
  bad_request.request_body = network::ResourceRequestBody::CreateFromBytes(
      json_payload.data(), json_payload.size());
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  json_payload = R"({
    "no-type": "view",
    "creativeInstanceId": "creative-instance-id"
  })";
  bad_request.request_body = network::ResourceRequestBody::CreateFromBytes(
      json_payload.data(), json_payload.size());
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  json_payload = R"({
    "not-type": "view",
    "creativeInstanceId": "creative-instance-id"
  })";
  bad_request.request_body = network::ResourceRequestBody::CreateFromBytes(
      json_payload.data(), json_payload.size());
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  json_payload = R"({
    "type": "click",
    "not-creativeInstanceId": "creative-instance-id"
  })";
  bad_request.request_body = network::ResourceRequestBody::CreateFromBytes(
      json_payload.data(), json_payload.size());
  EXPECT_TRUE(GetViewedSearchResultAdCreativeInstanceId(bad_request).empty());
}

TEST(SearchResultAdUtilTest, CheckSearchResultAdClickedConfirmationUrl) {
  EXPECT_TRUE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.brave.com/v3/click")));
  EXPECT_TRUE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.bravesoftware.com/v3/click")));

  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("http://search.anonymous.ads.brave.com/v3/click")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("http://search.anonymous.ads.bravesoftware.com/v3/click")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.brave.com/v4/click")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.bravesoftware.com/v4/click")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.brave.com/v3/non")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.bravesoftware.com/v3/non")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.brave.com/v3/confirmation")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.bravesoftware.com/v3/click")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.brave.com/v3")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.bravesoftware.com/v3")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.brave.com")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.bravesoftware.com")));
}

TEST(SearchResultAdUtilTest, CheckGetClickedSearchResultAdCreativeInstanceId) {
  network::ResourceRequest request;
  request.method = "GET";
  request.url = GURL("https://search.anonymous.ads.brave.com/v3/click?"
                     "creativeInstanceId=creative-instance-id");
  EXPECT_EQ("creative-instance-id",
            GetClickedSearchResultAdCreativeInstanceId(request));

  network::ResourceRequest bad_request = request;
  bad_request.method = "POST";
  EXPECT_TRUE(GetClickedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  bad_request.url = GURL("https://search.ads.brave.com/v3/click?"
                         "creativeInstanceId=creative-instance-id");
  EXPECT_TRUE(GetClickedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  bad_request.url = GURL("https://search.anonymous.ads.brave.com/v3/click?"
                         "creativeInstanceId=");
  EXPECT_TRUE(GetClickedSearchResultAdCreativeInstanceId(bad_request).empty());
  LOG(ERROR) << GetClickedSearchResultAdCreativeInstanceId(bad_request);

  bad_request = request;
  bad_request.url = GURL("https://search.ads.brave.com/v3/click?"
                         "creativeInstance=creative-instance-id");
  EXPECT_TRUE(GetClickedSearchResultAdCreativeInstanceId(bad_request).empty());

  bad_request = request;
  bad_request.url = GURL("https://search.anonymous.ads.brave.com/v3/click");
  EXPECT_TRUE(GetClickedSearchResultAdCreativeInstanceId(bad_request).empty());
}

}  // namespace brave_ads
