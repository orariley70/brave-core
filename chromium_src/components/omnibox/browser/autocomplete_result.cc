/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "src/components/omnibox/browser/autocomplete_result.cc"

// Move match at |from| to |to|.
void AutocompleteResult::MoveMatch(size_t from, size_t to) {
  DCHECK_LE(from, matches_.size());
  DCHECK_LE(to, matches_.size());

  if (from == to)
    return;

  auto promotion = matches_[from];
  RemoveMatch(matches_.begin() + from);

  if (from < to)
    to--;

  matches_.insert(matches_.begin() + to, promotion);
}

void AutocompleteResult::RemoveMatch(const ACMatches::iterator& it) {
  matches_.erase(it);
}
