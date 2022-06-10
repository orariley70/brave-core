/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/favicon/favicon_loader.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/api/favicon/favicon_attributes.h"
#include "components/favicon/core/large_icon_service.h"
#include "components/favicon_base/favicon_types.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#import "ios/chrome/browser/favicon/favicon_loader.h"
#include "ios/chrome/browser/favicon/favicon_service_factory.h"
#include "ios/chrome/browser/favicon/ios_chrome_favicon_loader_factory.h"
#include "ios/chrome/common/ui/favicon/favicon_attributes.h"
#import "ios/chrome/common/ui/favicon/favicon_constants.h"
#include "ios/chrome/common/ui/favicon/favicon_view.h"
#import "net/base/mac/url_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#include "base/task/cancelable_task_tracker.h"
#include "components/keyed_service/core/keyed_service.h"

class GURL;
@class FaviconAttributes;

namespace favicon {
class LargeIconService;
}

// A class that manages asynchronously loading favicons or fallback attributes
// from LargeIconService and caching them, given a URL.
class BraveIOSFaviconLoader : public KeyedService {
 public:
  // Type for completion block for FaviconForURL().
  typedef void (^FaviconAttributesCompletionBlock)(FaviconAttributes*);

  explicit BraveIOSFaviconLoader(favicon::LargeIconService* large_icon_service);

  BraveIOSFaviconLoader(const BraveIOSFaviconLoader&) = delete;
  BraveIOSFaviconLoader& operator=(const BraveIOSFaviconLoader&) = delete;

  ~BraveIOSFaviconLoader() override;

  // Tries to find a FaviconAttributes in |favicon_cache_| with |page_url|:
  // If found, invokes |faviconBlockHandler| and exits.
  // If not found, invokes |faviconBlockHandler| with a default placeholder
  // then invokes it again asynchronously with the favicon fetched by trying
  // following methods:
  //   1. Use |large_icon_service_| to fetch from local DB managed by
  //      HistoryService;
  //   2. Use |large_icon_service_| to fetch from Google Favicon server if
  //      |fallback_to_google_server|=YES (|size_in_points| is ignored when
  //      fetching from the Google server);
  //   3. Create a favicon base on the fallback style from |large_icon_service|.
  void FaviconForPageUrl(const GURL& page_url,
                         float size_in_points,
                         float min_size_in_points,
                         bool fallback_to_google_server,
                         FaviconAttributesCompletionBlock faviconBlockHandler);

  // Tries to find a FaviconAttributes in |favicon_cache_| with |page_url|:
  // If found, invokes |faviconBlockHandler| and exits.
  // If not found, invokes |faviconBlockHandler| with a default placeholder
  // then invokes it again asynchronously with the favicon fetched by trying
  // following methods:
  //   1. Use |large_icon_service_| to fetch from local DB managed by
  //      HistoryService;
  //   2. Create a favicon base on the fallback style from |large_icon_service|.
  void FaviconForPageUrlOrHost(
      const GURL& page_url,
      float size_in_points,
      FaviconAttributesCompletionBlock favicon_block_handler);

  // Tries to find a FaviconAttributes in |favicon_cache_| with |icon_url|:
  // If found, invokes |faviconBlockHandler| and exits.
  // If not found, invokes |faviconBlockHandler| with a default placeholder
  // then invokes it again asynchronously with the favicon fetched by trying
  // following methods:
  //   1. Use |large_icon_service_| to fetch from local DB managed by
  //      HistoryService;
  //   2. Create a favicon base on the fallback style from |large_icon_service|.
  void FaviconForIconUrl(const GURL& icon_url,
                         float size_in_points,
                         float min_size_in_points,
                         FaviconAttributesCompletionBlock faviconBlockHandler);

  // Cancel all incomplete requests.
  void CancellAllRequests();

  // Return a weak pointer to the current object.
  base::WeakPtr<BraveIOSFaviconLoader> AsWeakPtr();

 private:
  // The LargeIconService used to retrieve favicon.
  favicon::LargeIconService* large_icon_service_;

  // Tracks tasks sent to FaviconService.
  base::CancelableTaskTracker cancelable_task_tracker_;

  base::WeakPtrFactory<BraveIOSFaviconLoader> weak_ptr_factory_{this};
};


#include "base/bind.h"
#import "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "components/favicon/core/fallback_url_util.h"
#include "components/favicon/core/large_icon_service.h"
#include "components/favicon_base/fallback_icon_style.h"
#include "components/favicon_base/favicon_callback.h"
#include "components/favicon_base/favicon_types.h"
#import "ios/chrome/browser/ui/util/uikit_ui_util.h"
#import "ios/chrome/common/ui/favicon/favicon_attributes.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "skia/ext/skia_utils_ios.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kFallbackIconDefaultTextColor = 0xAAAAAA;

// NetworkTrafficAnnotationTag for fetching favicon from a Google server.
const net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("favicon_loader_get_large_icon", R"(
        semantics {
        sender: "BraveIOSFaviconLoader"
        description:
            "Sends a request to a Google server to retrieve the favicon bitmap."
        trigger:
            "A request can be sent if Chrome does not have a favicon."
        data: "Page URL and desired icon size."
        destination: GOOGLE_OWNED_SERVICE
        }
        policy {
        cookies_allowed: NO
        setting: "This feature cannot be disabled by settings."
        policy_exception_justification: "Not implemented."
        }
        )");
}  // namespace

BraveIOSFaviconLoader::BraveIOSFaviconLoader(favicon::LargeIconService* large_icon_service)
    : large_icon_service_(large_icon_service) {}
BraveIOSFaviconLoader::~BraveIOSFaviconLoader() {}

// TODO(pinkerton): How do we update the favicon if it's changed on the web?
// We can possibly just rely on this class being purged or the app being killed
// to reset it, but then how do we ensure the FaviconService is updated?
void BraveIOSFaviconLoader::FaviconForPageUrl(
    const GURL& page_url,
    float size_in_points,
    float min_size_in_points,
    bool fallback_to_google_server,  // retrieve favicon from Google Server if
                                     // GetLargeIconOrFallbackStyle() doesn't
                                     // return valid favicon.
    FaviconAttributesCompletionBlock faviconBlockHandler) {
  DCHECK(faviconBlockHandler);
  FaviconAttributes* value = nullptr;
  if (value) {
    faviconBlockHandler(value);
    return;
  }

  const CGFloat scale = UIScreen.mainScreen.scale;
  GURL block_page_url(page_url);
  auto favicon_block = ^(const favicon_base::LargeIconResult& result) {
    // GetLargeIconOrFallbackStyle() either returns a valid favicon (which can
    // be the default favicon) or fallback attributes.
    if (result.bitmap.is_valid()) {
      scoped_refptr<base::RefCountedMemory> data =
          result.bitmap.bitmap_data.get();
      // The favicon code assumes favicons are PNG-encoded.
      UIImage* favicon = [UIImage
          imageWithData:[NSData dataWithBytes:data->front() length:data->size()]
                  scale:scale];
      FaviconAttributes* attributes =
          [FaviconAttributes attributesWithImage:favicon];

      DCHECK(favicon.size.width <= size_in_points &&
             favicon.size.height <= size_in_points);
      faviconBlockHandler(attributes);
      return;
    } else if (fallback_to_google_server) {
      void (^favicon_loaded_from_server_block)(
          favicon_base::GoogleFaviconServerRequestStatus status) =
          ^(const favicon_base::GoogleFaviconServerRequestStatus status) {
            // Update the time when the icon was last requested - postpone thus
            // the automatic eviction of the favicon from the favicon database.
            large_icon_service_->TouchIconFromGoogleServer(block_page_url);

            // Favicon should be loaded to the db that backs LargeIconService
            // now.  Fetch it again. Even if the request was not successful, the
            // fallback style will be used.
            FaviconForPageUrl(
                block_page_url, size_in_points, min_size_in_points,
                /*continueToGoogleServer=*/false, faviconBlockHandler);
          };

      large_icon_service_
          ->GetLargeIconOrFallbackStyleFromGoogleServerSkippingLocalCache(
              block_page_url,
              /*may_page_url_be_private=*/true,
              /*should_trim_page_url_path=*/false, kTrafficAnnotation,
              base::BindRepeating(favicon_loaded_from_server_block));
      return;
    }

    // Did not get valid favicon back and are not attempting to retrieve one
    // from a Google Server.
    DCHECK(result.fallback_icon_style);
    FaviconAttributes* attributes = [FaviconAttributes
        attributesWithMonogram:base::SysUTF16ToNSString(
                                   favicon::GetFallbackIconText(block_page_url))
                     textColor:UIColorFromRGB(kFallbackIconDefaultTextColor)
               backgroundColor:UIColor.clearColor
        defaultBackgroundColor:result.fallback_icon_style->
                               is_default_background_color];
    faviconBlockHandler(attributes);
  };

  // First, synchronously return a fallback image.
  faviconBlockHandler([FaviconAttributes attributesWithDefaultImage]);

  // Now fetch the image synchronously.
  DCHECK(large_icon_service_);
  large_icon_service_->GetLargeIconRawBitmapOrFallbackStyleForPageUrl(
      page_url, scale * min_size_in_points, scale * size_in_points,
      base::BindRepeating(favicon_block), &cancelable_task_tracker_);
}

void BraveIOSFaviconLoader::FaviconForPageUrlOrHost(
    const GURL& page_url,
    float size_in_points,
    FaviconAttributesCompletionBlock favicon_block_handler) {
  DCHECK(favicon_block_handler);
  FaviconAttributes* value = nullptr;
  if (value) {
    favicon_block_handler(value);
    return;
  }

  const CGFloat scale = UIScreen.mainScreen.scale;
  GURL block_page_url(page_url);
  auto favicon_block = ^(const favicon_base::LargeIconResult& result) {
    // GetLargeIconOrFallbackStyle() either returns a valid favicon (which can
    // be the default favicon) or fallback attributes.
    if (result.bitmap.is_valid()) {
      scoped_refptr<base::RefCountedMemory> data =
          result.bitmap.bitmap_data.get();
      // The favicon code assumes favicons are PNG-encoded.
      UIImage* favicon = [UIImage
          imageWithData:[NSData dataWithBytes:data->front() length:data->size()]
                  scale:scale];
      FaviconAttributes* attributes =
          [FaviconAttributes attributesWithImage:favicon];

      DCHECK(favicon.size.width <= size_in_points &&
             favicon.size.height <= size_in_points);
      favicon_block_handler(attributes);
      return;
    }

    // Did not get valid favicon back and are not attempting to retrieve one
    // from a Google Server.
    DCHECK(result.fallback_icon_style);
    FaviconAttributes* attributes = [FaviconAttributes
        attributesWithMonogram:base::SysUTF16ToNSString(
                                   favicon::GetFallbackIconText(block_page_url))
                     textColor:UIColorFromRGB(kFallbackIconDefaultTextColor)
               backgroundColor:UIColor.clearColor
        defaultBackgroundColor:result.fallback_icon_style->
                               is_default_background_color];
    favicon_block_handler(attributes);
  };

  // First, synchronously return a fallback image.
  favicon_block_handler([FaviconAttributes attributesWithDefaultImage]);

  // Now fetch the image synchronously.
  DCHECK(large_icon_service_);
  large_icon_service_->GetIconRawBitmapOrFallbackStyleForPageUrl(
      page_url, scale * size_in_points, base::BindRepeating(favicon_block),
      &cancelable_task_tracker_);
}

void BraveIOSFaviconLoader::FaviconForIconUrl(
    const GURL& icon_url,
    float size_in_points,
    float min_size_in_points,
    FaviconAttributesCompletionBlock faviconBlockHandler) {
  DCHECK(faviconBlockHandler);
  FaviconAttributes* value = nullptr;
  if (value) {
    faviconBlockHandler(value);
    return;
  }

  const CGFloat scale = UIScreen.mainScreen.scale;
  const CGFloat favicon_size_in_pixels = scale * size_in_points;
  const CGFloat min_favicon_size_in_pixels = scale * min_size_in_points;
  GURL block_icon_url(icon_url);
  auto favicon_block = ^(const favicon_base::LargeIconResult& result) {
    // GetLargeIconOrFallbackStyle() either returns a valid favicon (which can
    // be the default favicon) or fallback attributes.
    if (result.bitmap.is_valid()) {
      scoped_refptr<base::RefCountedMemory> data =
          result.bitmap.bitmap_data.get();
      // The favicon code assumes favicons are PNG-encoded.
      UIImage* favicon = [UIImage
          imageWithData:[NSData dataWithBytes:data->front() length:data->size()]
                  scale:scale];
      FaviconAttributes* attributes =
          [FaviconAttributes attributesWithImage:favicon];
      faviconBlockHandler(attributes);
      return;
    }
    // Did not get valid favicon back and are not attempting to retrieve one
    // from a Google Server
    DCHECK(result.fallback_icon_style);
    FaviconAttributes* attributes = [FaviconAttributes
        attributesWithMonogram:base::SysUTF16ToNSString(
                                   favicon::GetFallbackIconText(block_icon_url))
                     textColor:UIColorFromRGB(kFallbackIconDefaultTextColor)
               backgroundColor:UIColor.clearColor
        defaultBackgroundColor:result.fallback_icon_style->
                               is_default_background_color];
    faviconBlockHandler(attributes);
  };

  // First, return a fallback synchronously.
  faviconBlockHandler([FaviconAttributes
      attributesWithImage:[UIImage imageNamed:@"default_world_favicon"]]);

  // Now call the service for a better async icon.
  DCHECK(large_icon_service_);
  large_icon_service_->GetLargeIconRawBitmapOrFallbackStyleForIconUrl(
      icon_url, min_favicon_size_in_pixels, favicon_size_in_pixels,
      base::BindRepeating(favicon_block), &cancelable_task_tracker_);
}

void BraveIOSFaviconLoader::CancellAllRequests() {
  cancelable_task_tracker_.TryCancelAll();
}

base::WeakPtr<BraveIOSFaviconLoader> BraveIOSFaviconLoader::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

#include <memory>

#include "base/no_destructor.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

class ChromeBrowserState;
class BraveIOSFaviconLoader;

// Singleton that owns all BraveIOSFaviconLoaders and associates them with
// ChromeBrowserState.
class BraveIOSChromeBraveIOSFaviconLoaderFactory : public BrowserStateKeyedServiceFactory {
 public:
  static BraveIOSFaviconLoader* GetForBrowserState(ChromeBrowserState* browser_state);
  static BraveIOSFaviconLoader* GetForBrowserStateIfExists(
      ChromeBrowserState* browser_state);
  static BraveIOSChromeBraveIOSFaviconLoaderFactory* GetInstance();
  // Returns the default factory used to build BraveIOSFaviconLoader. Can be registered
  // with SetTestingFactory to use the FaviconService instance during testing.
  static TestingFactory GetDefaultFactory();

  BraveIOSChromeBraveIOSFaviconLoaderFactory(const BraveIOSChromeBraveIOSFaviconLoaderFactory&) = delete;
  BraveIOSChromeBraveIOSFaviconLoaderFactory& operator=(
      const BraveIOSChromeBraveIOSFaviconLoaderFactory&) = delete;

 private:
  friend class base::NoDestructor<BraveIOSChromeBraveIOSFaviconLoaderFactory>;

  BraveIOSChromeBraveIOSFaviconLoaderFactory();
  ~BraveIOSChromeBraveIOSFaviconLoaderFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
};

#include "base/no_destructor.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/favicon/favicon_loader.h"
#include "ios/chrome/browser/favicon/ios_chrome_large_icon_service_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

std::unique_ptr<KeyedService> BuildBraveIOSFaviconLoader(web::BrowserState* context) {
  ChromeBrowserState* browser_state =
      ChromeBrowserState::FromBrowserState(context);
  return std::make_unique<BraveIOSFaviconLoader>(
      IOSChromeLargeIconServiceFactory::GetForBrowserState(browser_state));
}

}  // namespace

BraveIOSFaviconLoader* BraveIOSChromeBraveIOSFaviconLoaderFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<BraveIOSFaviconLoader*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

BraveIOSFaviconLoader* BraveIOSChromeBraveIOSFaviconLoaderFactory::GetForBrowserStateIfExists(
    ChromeBrowserState* browser_state) {
  return static_cast<BraveIOSFaviconLoader*>(
      GetInstance()->GetServiceForBrowserState(browser_state, false));
}

BraveIOSChromeBraveIOSFaviconLoaderFactory* BraveIOSChromeBraveIOSFaviconLoaderFactory::GetInstance() {
  static base::NoDestructor<BraveIOSChromeBraveIOSFaviconLoaderFactory> instance;
  return instance.get();
}

// static
BrowserStateKeyedServiceFactory::TestingFactory
BraveIOSChromeBraveIOSFaviconLoaderFactory::GetDefaultFactory() {
  return base::BindRepeating(&BuildBraveIOSFaviconLoader);
}

BraveIOSChromeBraveIOSFaviconLoaderFactory::BraveIOSChromeBraveIOSFaviconLoaderFactory()
    : BrowserStateKeyedServiceFactory(
          "BraveIOSFaviconLoader",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(IOSChromeLargeIconServiceFactory::GetInstance());
}

BraveIOSChromeBraveIOSFaviconLoaderFactory::~BraveIOSChromeBraveIOSFaviconLoaderFactory() {}

std::unique_ptr<KeyedService>
BraveIOSChromeBraveIOSFaviconLoaderFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  return BuildBraveIOSFaviconLoader(context);
}

web::BrowserState* BraveIOSChromeBraveIOSFaviconLoaderFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

bool BraveIOSChromeBraveIOSFaviconLoaderFactory::ServiceIsNULLWhileTesting() const {
  return true;
}














// MARK: - Constants
BraveFaviconLoaderSize const BraveFaviconLoaderSizeMin =
    static_cast<NSInteger>(kMinFaviconSizePt);
BraveFaviconLoaderSize const BraveFaviconLoaderSizeDesiredSmall =
    static_cast<NSInteger>(kDesiredSmallFaviconSizePt);
BraveFaviconLoaderSize const BraveFaviconLoaderSizeDesiredMedium =
    static_cast<NSInteger>(kDesiredMediumFaviconSizePt);

// MARK: - Implementation

@interface BraveFaviconAttributes (Private)
- (instancetype)initWithAttributes:(FaviconAttributes*)attributes;
@end

@interface BraveFaviconLoader () {
  BraveIOSFaviconLoader* favicon_loader_;
}
@end

@implementation BraveFaviconLoader
- (instancetype)initWithBrowserState:(ChromeBrowserState*)browserState {
  if ((self = [super init])) {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
      BraveIOSChromeBraveIOSFaviconLoaderFactory::GetInstance();
    });
    
    favicon_loader_ =
      BraveIOSChromeBraveIOSFaviconLoaderFactory::GetForBrowserState(browserState);
    DCHECK(favicon_loader_);
  }
  return self;
}

+ (instancetype)getForPrivateMode:(bool)privateMode {
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

  return [[BraveFaviconLoader alloc] initWithBrowserState:browser_state];
}

- (void)faviconForPageURL:(NSURL*)url
              sizeInPoints:(BraveFaviconLoaderSize)sizeInPoints
           minSizeInPoints:(BraveFaviconLoaderSize)minSizeInPoints
    fallbackToGoogleServer:(bool)fallbackToGoogleServer
                completion:
                    (void (^)(BraveFaviconAttributes* attributes))completion {
  favicon_loader_->FaviconForPageUrl(
      net::GURLWithNSURL(url), sizeInPoints, minSizeInPoints,
      fallbackToGoogleServer, ^(FaviconAttributes* attributes) {
        completion(
            [[BraveFaviconAttributes alloc] initWithAttributes:attributes]);
      });
}

- (void)faviconForPageURLOrHost:(NSURL*)url
                   sizeInPoints:(BraveFaviconLoaderSize)sizeInPoints
                     completion:(void (^)(BraveFaviconAttributes* attributes))
                                    completion {
  favicon_loader_->FaviconForPageUrlOrHost(
      net::GURLWithNSURL(url), sizeInPoints, ^(FaviconAttributes* attributes) {
        completion(
            [[BraveFaviconAttributes alloc] initWithAttributes:attributes]);
      });
}

- (void)faviconForIconURL:(NSURL*)url
             sizeInPoints:(BraveFaviconLoaderSize)sizeInPoints
          minSizeInPoints:(BraveFaviconLoaderSize)minSizeInPoints
               completion:
                   (void (^)(BraveFaviconAttributes* attributes))completion {
  favicon_loader_->FaviconForIconUrl(
      net::GURLWithNSURL(url), sizeInPoints, minSizeInPoints,
      ^(FaviconAttributes* attributes) {
        completion(
            [[BraveFaviconAttributes alloc] initWithAttributes:attributes]);
      });
}
@end
