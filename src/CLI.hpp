// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <opentxs/opentxs.hpp>

#include <boost/program_options.hpp>

#include <functional>
#include <map>

namespace po = boost::program_options;

namespace opentxs::otctl
{
class CLI
{
public:
    CLI(const api::Native& ot, const po::variables_map& options);

    int Run();

    ~CLI() = default;

private:
    using PushHandler = void (*)(const proto::RPCPush&);
    using ResponseHandler = void (*)(const proto::RPCResponse&);
    using Processor =
        void (*)(const std::string&, const network::zeromq::DealerSocket&);

    static const std::map<std::string, proto::RPCCommandType> commands_;
    static const std::map<proto::RPCPushType, PushHandler> push_handlers_;
    static const std::map<proto::RPCCommandType, ResponseHandler>
        response_handlers_;
    static const std::map<proto::RPCCommandType, Processor> processors_;
    static const std::map<proto::RPCCommandType, std::string> command_names_;
    static const std::map<proto::RPCResponseCode, std::string> status_names_;

    const api::Native& ot_;
    const po::variables_map& options_;
    const std::string endpoint_;
    OTZMQListenCallback callback_;
    OTZMQDealerSocket socket_;

    static void add_client_session(
        const std::string& in,
        const network::zeromq::DealerSocket& socket);
    static void add_server_session(
        const std::string& in,
        const network::zeromq::DealerSocket& socket);
    static void create_nym(
        const std::string& in,
        const network::zeromq::DealerSocket& socket);
    static void list_client_sessions(
        const std::string& in,
        const network::zeromq::DealerSocket& socket);
    static void list_server_contracts(
        const std::string& in,
        const network::zeromq::DealerSocket& socket);
    static void list_server_sessions(
        const std::string& in,
        const network::zeromq::DealerSocket& socket);
    static void register_nym(
        const std::string& in,
        const network::zeromq::DealerSocket& socket);

    static void add_session_response(const proto::RPCResponse& in);
    static void create_nym_response(const proto::RPCResponse& in);
    static void list_servers_response(const proto::RPCResponse& in);
    static void list_session_response(const proto::RPCResponse& in);
    static void register_nym_response(const proto::RPCResponse& in);

    static std::string find_home();
    static std::string get_command_name(const proto::RPCCommandType type);
    static std::string get_json(const po::variables_map& cli);
    static std::string get_socket_path(const po::variables_map& cli);
    static std::string get_status_name(const proto::RPCResponseCode code);
    static void parse_command(
        const std::string& input,
        po::options_description& options);
    static void print_basic_info(const proto::RPCPush& in);
    static void print_basic_info(const proto::RPCResponse& in);
    static void process_push(network::zeromq::Message& in);
    static void process_reply(network::zeromq::Message& in);
    static bool send_message(
        const network::zeromq::DealerSocket& socket,
        const proto::RPCCommand command);
    static void set_keys(
        const po::variables_map& cli,
        network::zeromq::DealerSocket& socket);
    static void task_complete_push(const proto::RPCPush& in);

    void callback(network::zeromq::Message& in);

    CLI() = delete;
    CLI(const CLI&) = delete;
    CLI(CLI&&) = delete;
    CLI& operator=(const CLI&) = delete;
    CLI& operator=(CLI&&) = delete;
};
}  // namespace opentxs::otctl
