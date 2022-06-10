/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_DRIVER_H_
#define BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_DRIVER_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <Webkit/Webkit.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(FaviconLoader.Driver)
@interface BraveFaviconDriver : NSObject
- (instancetype)initWithPrivateBrowsingMode:(bool)privateMode;

- (void)webView:(WKWebView*)webView didStartProvisionalNavigation:(WKNavigation *)navigation;
- (void)webView:(WKWebView*)webView decidePolicyForNavigationAction:(WKNavigationAction*)action;

- (void)webView:(WKWebView*)webView didReceiveServerRedirectForProvisionalNavigation:(WKNavigation*)navigation;

- (void)webView:(WKWebView*)webView
    didFailProvisionalNavigation:(WKNavigation*)navigation
      withError:(NSError*)error;
- (void)webView:(WKWebView*)webView didCommitNavigation:(WKNavigation*)navigation;
- (void)webView:(WKWebView*)webView didFinishNavigation:(WKNavigation*)navigation;

- (void)webView:(WKWebView*)webView onFaviconURLsUpdated:(WKScriptMessage*)scriptMessage;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_DRIVER_H_
