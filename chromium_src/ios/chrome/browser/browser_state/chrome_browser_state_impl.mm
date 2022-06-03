/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/browser_state/chrome_browser_state_impl.h"

#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/off_the_record_chrome_browser_state_impl.h"
#include "ios/chrome/browser/policy/browser_policy_connector_ios.h"
#include "ios/chrome/browser/policy/browser_state_policy_connector.h"
#include "ios/chrome/browser/policy/browser_state_policy_connector_factory.h"

#define BrowserPolicyConnectorIOS \
  if (false) {                    \
  BrowserPolicyConnectorIOS
#define BRAVE_CHROME_BROWSER_STATE_IMPL_CHROME_BROWSER_STATE_IMPL_CLOSE_IF }
#define GetPolicyConnector GetPolicyConnector_ChromiumImpl

#include "src/ios/chrome/browser/browser_state/chrome_browser_state_impl.mm"

#undef GetPolicyConnector
#undef BRAVE_CHROME_BROWSER_STATE_IMPL_CHROME_BROWSER_STATE_IMPL_CLOSE_IF
#undef BrowserPolicyConnectorIOS

BrowserStatePolicyConnector* ChromeBrowserStateImpl::GetPolicyConnector() {
  return nullptr;
}
