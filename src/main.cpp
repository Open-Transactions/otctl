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

    const auto& ot = opentxs::InitContext();
    std::unique_ptr<opentxs::otctl::CLI> otctl;
    otctl.reset(new opentxs::otctl::CLI(ot, variables));
    otctl->Run();
    opentxs::LogNormal("Shutting down...").Flush();
    otctl.reset();
    opentxs::Cleanup();
    opentxs::Join();

    return 0;
}
