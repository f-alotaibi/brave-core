// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/feature_list.h"

namespace ai_rewriter::features {

BASE_FEATURE(kAIRewriter, "AIRewriter", base::FEATURE_DISABLED_BY_DEFAULT);

bool IsAIRewriterEnabled() {
	return false;
}

}  // namespace ai_rewriter::features
