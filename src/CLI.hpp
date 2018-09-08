// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <opentxs/opentxs.hpp>

#include <functional>
#include <map>

namespace opentxs::otctl
{
class CLI
{
public:
    CLI(const api::Native& ot);

    int Run();

    ~CLI() = default;

private:
    using Handler = void (*)(const proto::RPCResponse&);
    using Processor = void (*)(
        const std::string&,
        const network::zeromq::DealerSocket&);

    static const std::map<std::string, proto::RPCCommandType> commands_;
    static const std::map<proto::RPCCommandType, Handler> response_handlers_;
    static const std::map<proto::RPCCommandType, Processor> processors_;
    static const std::map<proto::RPCCommandType, std::string> command_names_;
    static const std::map<proto::RPCResponseCode, std::string> status_names_;

    const api::Native& ot_;
    const std::string endpoint_;
    OTZMQListenCallback callback_;
    OTZMQDealerSocket socket_;

    static void add_client_session_response(const proto::RPCResponse& in);
    static std::string get_command_name(const proto::RPCCommandType type);
    static std::string get_socket_path();
    static std::string get_status_name(const proto::RPCResponseCode code);
    static void print_basic_info(const proto::RPCResponse& in);

    static void add_client_session(
        const std::string& in,
        const network::zeromq::DealerSocket& socket);

    void callback(network::zeromq::Message& in);

    CLI() = delete;
    CLI(const CLI&) = delete;
    CLI(CLI&&) = delete;
    CLI& operator=(const CLI&) = delete;
    CLI& operator=(CLI&&) = delete;
};
}  // namespace opentxs::agent
