/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_PANEL_SERVICE_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_PANEL_SERVICE_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_rewards/common/brave_rewards_panel.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

class Browser;
class Profile;

namespace brave_rewards {

// Provides a communication channel for arbitrary browser components that need
// to open the Rewards panel and application views that control the state of the
// Rewards panel.
class RewardsPanelService : public KeyedService {
 public:
  explicit RewardsPanelService(Profile* profile);
  ~RewardsPanelService() override;

  static bool IsRewardsPanelURLForTesting(const GURL& url);

  // Opens the Rewards panel with the default view.
  bool OpenRewardsPanel();

  // Displays the Rewards onboarding tour in the Rewards panel.
  bool ShowRewardsTour();

  // Displays a grant captcha for the specified grant in the Rewards panel.
  bool ShowGrantCaptcha(const std::string& grant_id);

  // Opens the Rewards panel in order to display the currently scheduled
  // adaptive captcha for the user.
  bool ShowAdaptiveCaptcha();

  // Opens the Rewards panel in order to display the Brave Talk Rewards opt-in.
  bool ShowBraveTalkOptIn();

  class Observer : public base::CheckedObserver {
   public:
    // Called when an application component requests that the Rewards panel be
    // opened. The arguments provided by |OpenRewardsPanel| must be retrieved
    // using |TakePanelArguments|.
    virtual void OnRewardsPanelRequested(Browser* browser,
                                         const mojom::RewardsPanelArgs& args) {}

    // Called when the Rewards panel has been closed.
    virtual void OnRewardsPanelClosed(Browser* browser) {}
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  using Observation = base::ScopedObservation<RewardsPanelService, Observer>;

  // Notifies observers that the Rewards panel has been closed. This should
  // only be called by UI objects.
  void NotifyPanelClosed(Browser* browser);

  // Retrieves the `mojom::RewardsPanelArgs` associated with the most recent
  // Rewards panel request.
  const mojom::RewardsPanelArgs& panel_args() { return panel_args_; }

 private:
  // Opens the Rewards panel using the specified arguments.
  bool OpenPanelWithArgs(mojom::RewardsPanelArgs&& args);

  raw_ptr<Profile> profile_ = nullptr;
  base::ObserverList<Observer> observers_;
  mojom::RewardsPanelArgs panel_args_;
  std::unique_ptr<Observer> extension_handler_;
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_PANEL_SERVICE_H_
