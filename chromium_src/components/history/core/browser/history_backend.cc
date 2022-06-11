/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "brave/components/constants/brave_switches.h"

namespace {
bool KeepOldHistory()
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kKeepOldHistory);
}

}  // namespace
#include "src/components/history/core/browser/history_backend.cc"
