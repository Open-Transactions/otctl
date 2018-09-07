// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <opentxs/opentxs.hpp>

#include "CLI.hpp"

int main(int, char**)
{
    const auto& ot = opentxs::OT::Start({});
    opentxs::otctl::CLI cli{ot};
    cli.Run();
    opentxs::OT::Cleanup();

    return 0;
}
