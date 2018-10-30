// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <opentxs/opentxs.hpp>

#include <boost/program_options.hpp>

#include "CLI.hpp"

namespace po = boost::program_options;

int main(int argc, char** argv)
{
    auto options = po::options_description{"otctl"};
    options.add_options()(
        "keyfile",
        po::value<std::string>(),
        "Path to file containing endpoint keys.")(
        "endpoint", po::value<std::string>(), "Remote zmq endpoint")(
        "logendpoint", po::value<std::string>(), "Source of otagent logs");
    auto variables = po::variables_map{};

    try {
        po::store(po::parse_command_line(argc, argv, options), variables);
        po::notify(variables);
    } catch (const po::error& e) {
        std::cerr << "ERROR: " << e.what() << "\n\n" << options << std::endl;

        return 1;
    }

    const auto& ot = opentxs::OT::Start({});
    opentxs::otctl::CLI cli{ot, variables};
    cli.Run();
    opentxs::OT::Cleanup();

    return 0;
}
