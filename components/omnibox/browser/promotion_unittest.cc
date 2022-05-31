/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_search_conversion/features.h"
#include "brave/components/brave_search_conversion/types.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/omnibox/browser/promotion_provider.h"
#include "brave/components/omnibox/browser/promotion_utils.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_result.h"
#include "components/omnibox/browser/mock_autocomplete_provider_client.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using brave_search_conversion::ConversionType;
using brave_search_conversion::GetPromoURL;
using brave_search_conversion::RegisterPrefs;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

class LocaleHelperMock : public brave_l10n::LocaleHelper {
 public:
  MOCK_CONST_METHOD0(GetLocale, std::string());
};

}  // namespace

class OmniboxPromotionTest : public testing::Test {
 public:
  OmniboxPromotionTest() = default;
  ~OmniboxPromotionTest() override = default;

  void SetUp() override {
    SetMockLocale("en-US");
    SetMockClient();
    provider_ = new PromotionProvider(client_mock_.get());
  }

  void SetMockLocale(const std::string& locale) {
    // Set promotion supported locale.
    locale_helper_mock_ = std::make_unique<NiceMock<LocaleHelperMock>>();
    brave_l10n::LocaleHelper::GetInstance()->set_for_testing(
        locale_helper_mock_.get());
    ON_CALL(*locale_helper_mock_, GetLocale()).WillByDefault(Return(locale));
  }

  void SetMockClient() {
    // Set non brave search provider to get promotion match.
    auto bing_data = TemplateURLDataFromPrepopulatedEngine(
        TemplateURLPrepopulateData::brave_bing);
    auto bing_template_url = std::make_unique<TemplateURL>(*bing_data);
    auto template_url_service =
        std::make_unique<TemplateURLService>(nullptr, 0);
    template_url_service->Load();
    template_url_service->SetUserSelectedDefaultSearchProvider(
        bing_template_url.get());

    client_mock_ = std::make_unique<MockAutocompleteProviderClient>();
    client_mock_->set_template_url_service(std::move(template_url_service));
  }

  TestSchemeClassifier classifier_;
  std::unique_ptr<MockAutocompleteProviderClient> client_mock_;
  std::unique_ptr<LocaleHelperMock> locale_helper_mock_;
  scoped_refptr<PromotionProvider> provider_;
};

TEST_F(OmniboxPromotionTest, PromotionProviderTest) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      brave_search_conversion::features::kOmniboxButton);

  EXPECT_TRUE(provider_->matches().empty());
  AutocompleteInput input(u"brave", metrics::OmniboxEventProto::OTHER,
                          classifier_);
  provider_->Start(input, false);
  // Get one result.
  EXPECT_EQ(1UL, provider_->matches().size());

  // Check promotion match is not added.
  feature_list.Reset();
  provider_->Start(input, false);
  EXPECT_TRUE(provider_->matches().empty());
}

TEST_F(OmniboxPromotionTest, AutocompleteResultTest) {
  ACMatches matches;
  AutocompleteInput input(u"brave", metrics::OmniboxEventProto::OTHER,
                          classifier_);

  // Make first item is search query with default provider.
  AutocompleteMatch match(nullptr, 800, true,
                          AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED);
  matches.push_back(match);
  match.type = AutocompleteMatchType::NAVSUGGEST;
  matches.push_back(match);
  matches.push_back(match);
  // Put promo match as 4th match.
  match.destination_url = GetPromoURL(input.text());
  matches.push_back(match);

  match.destination_url = GURL();
  matches.push_back(match);

  AutocompleteResult result;
  result.AppendMatches(input, matches);
  SortBraveSearchPromotionMatch(&result, input, ConversionType::kNone);
  // Match position is not changed for kNone type.
  EXPECT_TRUE(IsBraveSearchPromotionMatch(*result.match_at(3), input.text()));

  // Check match is re-positioned as a second match for button type.
  SortBraveSearchPromotionMatch(&result, input, ConversionType::kButton);
  EXPECT_TRUE(IsBraveSearchPromotionMatch(*result.match_at(1), input.text()));

  // Check match is re-positioned as a last match for banner type.
  SortBraveSearchPromotionMatch(&result, input, ConversionType::kBanner);
  EXPECT_TRUE(IsBraveSearchPromotionMatch(*result.match_at(4), input.text()));

  // Make first match is not search query with default provider.
  result.begin()->type = AutocompleteMatchType::NAVSUGGEST;
  // Check promotion match is deleted from |result|.
  SortBraveSearchPromotionMatch(&result, input, ConversionType::kButton);
  for (const auto& match : result) {
    EXPECT_FALSE(IsBraveSearchPromotionMatch(match, input.text()));
  }
}
