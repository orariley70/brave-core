/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/favicon/favicon_driver.h"
#include "components/favicon/core/favicon_service.h"
#include "components/favicon/ios/web_favicon_driver.h"
#include "components/keyed_service/core/service_access_type.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/browser/favicon/favicon_service_factory.h"
#import "ios/web/public/web_state.h"

#import "ios/web/public/favicon/favicon_url.h"
#import "net/base/mac/url_conversions.h"

#include "third_party/abseil-cpp/absl/types/optional.h"
#import "ios/web/public/js_messaging/script_message.h"
#import "ios/web/js_messaging/web_view_js_utils.h"
#import "ios/web/favicon/favicon_util.h"
#include "net/base/url_util.h"
#import "components/favicon/core/core_favicon_service.h"

#import "ios/web/navigation/navigation_item_impl.h"
#import "ios/web/public/favicon/favicon_status.h"
#import "components/favicon/ios/favicon_url_util.h"
#include "base/bind.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#import "ios/web/navigation/navigation_manager_impl.h"
#include "skia/ext/skia_utils_ios.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {
struct FaviconStatus;
}

namespace favicon {
class CoreFaviconService;


class BraveWebFaviconDriver : public FaviconDriverImpl,
                              public base::SupportsUserData::Data {
public:
  BraveWebFaviconDriver(const BraveWebFaviconDriver&) = delete;
  BraveWebFaviconDriver& operator=(const BraveWebFaviconDriver&) = delete;

  ~BraveWebFaviconDriver() override;

  static void CreateFromBrowserState(ChromeBrowserState* browser_state,
                                     CoreFaviconService* favicon_service);
  static BraveWebFaviconDriver* FromBrowserState(ChromeBrowserState* browser_state);

  // FaviconDriver implementation.
  gfx::Image GetFavicon() const override;
  bool FaviconIsValid() const override;
  GURL GetActiveURL() override;

  // FaviconHandler::Delegate implementation.
  int DownloadImage(const GURL& url,
                    int max_image_size,
                    ImageDownloadCallback callback) override;
  void DownloadManifest(const GURL& url,
                        ManifestDownloadCallback callback) override;
  bool IsOffTheRecord() override;
  void OnFaviconUpdated(
      const GURL& page_url,
      FaviconDriverObserver::NotificationIconType notification_icon_type,
      const GURL& icon_url,
      bool icon_url_changed,
      const gfx::Image& image) override;
  void OnFaviconDeleted(const GURL& page_url,
                        FaviconDriverObserver::NotificationIconType
                            notification_icon_type) override;
  
  void DidStartNavigation(ChromeBrowserState* browser_state, WKWebView* webView);
  void DidFinishNavigation(ChromeBrowserState* browser_state, WKWebView* webView);
  void FaviconUrlUpdated(const std::vector<web::FaviconURL>& candidates);
  
private:
  BraveWebFaviconDriver(ChromeBrowserState* browser_state,
                     CoreFaviconService* favicon_service);
  void SetFaviconStatus(
        const GURL& page_url,
        const web::FaviconStatus& favicon_status,
        FaviconDriverObserver::NotificationIconType notification_icon_type,
        bool icon_url_changed);
  
  // Image Fetcher used to fetch favicon.
  image_fetcher::IOSImageDataFetcherWrapper image_fetcher_;
  ChromeBrowserState* browser_state_ = nullptr;
  std::vector<std::unique_ptr<web::NavigationItemImpl>> items;
};

BraveWebFaviconDriver* BraveWebFaviconDriver::FromBrowserState(ChromeBrowserState* browser_state) {
  //DCHECK_CURRENTLY_ON(web::WebThread::UI);
  DCHECK(browser_state);

  return static_cast<BraveWebFaviconDriver*>(
          browser_state->GetUserData("kBraveWebFaviconDriver"));
}

void BraveWebFaviconDriver::CreateFromBrowserState(ChromeBrowserState* browser_state,
                                                   CoreFaviconService* favicon_service) {
  if (FromBrowserState(browser_state)) {
    return;
  }
  
  browser_state->SetUserData("kBraveWebFaviconDriver",
                             base::WrapUnique(new BraveWebFaviconDriver(browser_state, favicon_service)));
}

// FaviconDriver implementation.
gfx::Image BraveWebFaviconDriver::GetFavicon() const {
  static const web::FaviconStatus missing_favicon_status;
  web::NavigationItemImpl* item = items.size() > 0 ? items[items.size() - 1].get() : nullptr;
  return (item ? item->GetFaviconStatus() : missing_favicon_status).image;
}

bool BraveWebFaviconDriver::FaviconIsValid() const {
  static const web::FaviconStatus missing_favicon_status;
  web::NavigationItemImpl* item = items.size() > 0 ? items[items.size() - 1].get() : nullptr;
  return (item ? item->GetFaviconStatus() : missing_favicon_status).valid;
}

GURL BraveWebFaviconDriver::GetActiveURL() {
  web::NavigationItemImpl* item = items.size() > 0 ? items[items.size() - 1].get() : nullptr;
  return item ? item->GetURL() : GURL();
}

// FaviconHandler::Delegate implementation.

int BraveWebFaviconDriver::DownloadImage(const GURL& url,
                                    int max_image_size,
                                    ImageDownloadCallback callback) {
  static int downloaded_image_count = 0;
  int local_download_id = ++downloaded_image_count;

  GURL local_url(url);
  __block ImageDownloadCallback local_callback = std::move(callback);

  image_fetcher::ImageDataFetcherBlock ios_callback =
      ^(NSData* data, const image_fetcher::RequestMetadata& metadata) {
        if (metadata.http_response_code ==
            image_fetcher::RequestMetadata::RESPONSE_CODE_INVALID)
          return;

        std::vector<SkBitmap> frames;
        std::vector<gfx::Size> sizes;
        if (data) {
          frames = skia::ImageDataToSkBitmapsWithMaxSize(data, max_image_size);
          for (const auto& frame : frames) {
            sizes.push_back(gfx::Size(frame.width(), frame.height()));
          }
          DCHECK_EQ(frames.size(), sizes.size());
        }
        std::move(local_callback)
            .Run(local_download_id, metadata.http_response_code, local_url,
                 frames, sizes);
      };
  image_fetcher_.FetchImageDataWebpDecoded(url, ios_callback);

  return downloaded_image_count;
}

void BraveWebFaviconDriver::DownloadManifest(const GURL& url,
                                        ManifestDownloadCallback callback) {
  NOTREACHED();
}

bool BraveWebFaviconDriver::IsOffTheRecord() {
  DCHECK(browser_state_);
  return browser_state_->IsOffTheRecord();
}

void BraveWebFaviconDriver::OnFaviconUpdated(
    const GURL& page_url,
    FaviconDriverObserver::NotificationIconType notification_icon_type,
    const GURL& icon_url,
    bool icon_url_changed,
    const gfx::Image& image) {
  web::FaviconStatus favicon_status;
  favicon_status.valid = true;
  favicon_status.image = image;
  favicon_status.url = icon_url;

  SetFaviconStatus(page_url, favicon_status, notification_icon_type,
                   icon_url_changed);
}

void BraveWebFaviconDriver::OnFaviconDeleted(
    const GURL& page_url,
    FaviconDriverObserver::NotificationIconType notification_icon_type) {
  SetFaviconStatus(page_url, web::FaviconStatus(), notification_icon_type,
                   /*icon_url_changed=*/true);
}

// Constructor / Destructor

BraveWebFaviconDriver::BraveWebFaviconDriver(ChromeBrowserState* browser_state,
                                   CoreFaviconService* favicon_service)
    : FaviconDriverImpl(favicon_service),
      image_fetcher_(browser_state->GetSharedURLLoaderFactory()),
      browser_state_(browser_state) {
        
}

BraveWebFaviconDriver::~BraveWebFaviconDriver() {
  browser_state_ = nullptr;
  DCHECK(!browser_state_);
}

// Notifications

void BraveWebFaviconDriver::DidStartNavigation(ChromeBrowserState* browser_state, WKWebView* webView) {
  items = std::vector<std::unique_ptr<web::NavigationItemImpl>>();
  
  auto item = std::make_unique<web::NavigationItemImpl>();
  item->SetOriginalRequestURL(net::GURLWithNSURL(webView.URL));
  item->SetURL(net::GURLWithNSURL(webView.URL));
  item->SetTransitionType(ui::PageTransition::PAGE_TRANSITION_TYPED);
  item->SetNavigationInitiationType(web::NavigationInitiationType::BROWSER_INITIATED);
  //item->SetUpgradedToHttps();
  items.push_back(std::move(item));
}

void BraveWebFaviconDriver::DidFinishNavigation(ChromeBrowserState* browser_state, WKWebView* webView) {
  
  web::NavigationItemImpl* item = items.size() > 0 ? items[items.size() - 1].get() : nullptr;
  if (!item) {
    printf("BAD!");
    return;
  }
  
  // Fetch the fav-icon
  FetchFavicon(item->GetURL(), /*IsSameDocument*/false);
}

void BraveWebFaviconDriver::FaviconUrlUpdated(const std::vector<web::FaviconURL>& candidates) {
  DCHECK(!candidates.empty());
  OnUpdateCandidates(GetActiveURL(), FaviconURLsFromWebFaviconURLs(candidates),
                     GURL());
}

void BraveWebFaviconDriver::SetFaviconStatus(
    const GURL& page_url,
    const web::FaviconStatus& favicon_status,
    FaviconDriverObserver::NotificationIconType notification_icon_type,
    bool icon_url_changed) {
  
  web::NavigationItemImpl* item = items.size() > 0 ? items[items.size() - 1].get() : nullptr;
  if (!item || item->GetURL() != page_url) {
    return;
  }

  item->SetFaviconStatus(favicon_status);
  NotifyFaviconUpdatedObservers(notification_icon_type, favicon_status.url,
                                icon_url_changed, favicon_status.image);
}

}


// MARK: - Implementation

@interface BraveFaviconDriver()
{
  ChromeBrowserState* browser_state_;
}
@end

@implementation BraveFaviconDriver
- (instancetype)initWithPrivateBrowsingMode:(bool)privateMode {
  if ((self = [super init])) {
    ios::ChromeBrowserStateManager* browser_state_manager =
        GetApplicationContext()->GetChromeBrowserStateManager();
    CHECK(browser_state_manager);

    ChromeBrowserState* browser_state =
        browser_state_manager->GetLastUsedBrowserState();
    CHECK(browser_state);

    if (privateMode) {
      browser_state = browser_state->GetOffTheRecordChromeBrowserState();
      CHECK(browser_state);
    }
    
    browser_state_ = browser_state;

    ChromeBrowserState* original_browser_state =
        ChromeBrowserState::FromBrowserState(browser_state);
    favicon::BraveWebFaviconDriver::CreateFromBrowserState(
        browser_state,
        ios::FaviconServiceFactory::GetForBrowserState(
            original_browser_state, ServiceAccessType::EXPLICIT_ACCESS));
  }
  return self;
}

- (void)webView:(WKWebView*)webView didStartProvisionalNavigation:(WKNavigation *)navigation {
  GURL webViewURL = net::GURLWithNSURL(webView.URL);
  
  favicon::BraveWebFaviconDriver* driver = favicon::BraveWebFaviconDriver::FromBrowserState(browser_state_);
  
  driver->DidStartNavigation(browser_state_, webView);
}

- (void)webView:(WKWebView*)webView decidePolicyForNavigationAction:(WKNavigationAction*)action {
  GURL requestURL = net::GURLWithNSURL(action.request.URL);

}

- (void)webView:(WKWebView*)webView didReceiveServerRedirectForProvisionalNavigation:(WKNavigation*)navigation {
  GURL webViewURL = net::GURLWithNSURL(webView.URL);
  
  
}

- (void)webView:(WKWebView*)webView
    didFailProvisionalNavigation:(WKNavigation*)navigation
      withError:(NSError*)error {
  
  
}

- (void)webView:(WKWebView*)webView didCommitNavigation:(WKNavigation*)navigation {
  
  
}

- (void)webView:(WKWebView*)webView didFinishNavigation:(WKNavigation*)navigation {
//  favicon::BraveWebFaviconDriver* driver = favicon::BraveWebFaviconDriver::FromBrowserState(browser_state_);
  
  //driver->DidFinishNavigation(browser_state_, webView);
}

- (void)webView:(WKWebView*)webView
    didFailNavigation:(WKNavigation*)navigation
      withError:(NSError*)error {
  
}

- (void)webView:(WKWebView*)webView onFaviconURLsUpdated:(WKScriptMessage*)scriptMessage {
  NSURL* ns_url = scriptMessage.frameInfo.request.URL;
  absl::optional<GURL> url;
  if (ns_url) {
    url = net::GURLWithNSURL(ns_url);
  }

  web::ScriptMessage message(web::ValueResultFromWKResult(scriptMessage.body),
                        false,
                        scriptMessage.frameInfo.mainFrame, url);
  
  {
    const GURL url = message.request_url().value();

    std::vector<web::FaviconURL> urls;
    if (!ExtractFaviconURL(message.body()->GetListDeprecated(), url, &urls))
      return;

    if (!urls.empty()) {
      favicon::BraveWebFaviconDriver* driver = favicon::BraveWebFaviconDriver::FromBrowserState(browser_state_);
      
      driver->DidFinishNavigation(browser_state_, webView);
      driver->FaviconUrlUpdated(urls);
    }
  }
}
@end
