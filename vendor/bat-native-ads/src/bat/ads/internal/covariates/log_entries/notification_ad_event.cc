/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/covariates/log_entries/notification_ad_event.h"

namespace ads {

NotificationAdEvent::NotificationAdEvent() = default;

NotificationAdEvent::~NotificationAdEvent() = default;

void NotificationAdEvent::SetEventType(
    const mojom::NotificationAdEventType event_type) {
  event_type_ = event_type;
}

brave_federated::mojom::DataType NotificationAdEvent::GetDataType() const {
  return brave_federated::mojom::DataType::kBool;
}

brave_federated::mojom::CovariateType NotificationAdEvent::GetType() const {
  return brave_federated::mojom::CovariateType::kNotificationAdEvent;
}

std::string NotificationAdEvent::GetValue() const {
  switch (event_type_) {
    case mojom::NotificationAdEventType::kClicked:
      return "clicked";
    case mojom::NotificationAdEventType::kDismissed:
      return "dismissed";
    case mojom::NotificationAdEventType::kTimedOut:
      return "timedOut";
    default:
      return "unknown";
  }
}

}  // namespace ads
