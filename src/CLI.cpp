// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "CLI.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>
#include <json/json.h>

extern "C" {
#include <pwd.h>
#include <unistd.h>
}

namespace fs = boost::filesystem;

#define RPC_COMMAND_VERSION 1
#define CREATE_NYM_VERSION 1

namespace opentxs::otctl
{
const std::map<std::string, proto::RPCCommandType> CLI::commands_{
    {"addclient", proto::RPCCOMMAND_ADDCLIENTSESSION},
    {"createnym", proto::RPCCOMMAND_CREATENYM},
};
const std::map<proto::RPCPushType, CLI::PushHandler> CLI::push_handlers_{
    {proto::RPCPUSH_TASK, &CLI::task_complete_push},
};
const std::map<proto::RPCCommandType, CLI::ResponseHandler>
    CLI::response_handlers_{
        {proto::RPCCOMMAND_ADDCLIENTSESSION, &CLI::add_client_session_response},
        {proto::RPCCOMMAND_CREATENYM, &CLI::create_nym_response},
    };
const std::map<proto::RPCCommandType, CLI::Processor> CLI::processors_{
    {proto::RPCCOMMAND_ADDCLIENTSESSION, &CLI::add_client_session},
    {proto::RPCCOMMAND_CREATENYM, &CLI::create_nym},
};

const std::map<proto::RPCCommandType, std::string> CLI::command_names_{
    {proto::RPCCOMMAND_ADDCLIENTSESSION, "ADDCLIENTSESSION"},
    {proto::RPCCOMMAND_ADDSERVERSESSION, "ADDSERVERSESSION"},
    {proto::RPCCOMMAND_LISTCLIENTSSESSIONS, "LISTCLIENTSSESSIONS"},
    {proto::RPCCOMMAND_LISTSERVERSSESSIONS, "LISTSERVERSSESSIONS"},
    {proto::RPCCOMMAND_IMPORTHDSEED, "IMPORTHDSEED"},
    {proto::RPCCOMMAND_LISTHDSEEDS, "LISTHDSEEDS"},
    {proto::RPCCOMMAND_GETHDSEED, "GETHDSEED"},
    {proto::RPCCOMMAND_CREATENYM, "CREATENYM"},
    {proto::RPCCOMMAND_LISTNYMS, "LISTNYMS"},
    {proto::RPCCOMMAND_GETNYM, "GETNYM"},
    {proto::RPCCOMMAND_ADDCLAIM, "ADDCLAIM"},
    {proto::RPCCOMMAND_DELETECLAIM, "DELETECLAIM"},
    {proto::RPCCOMMAND_IMPORTSERVERCONTRACT, "IMPORTSERVERCONTRACT"},
    {proto::RPCCOMMAND_LISTSERVERCONTRACTS, "LISTSERVERCONTRACTS"},
    {proto::RPCCOMMAND_REGISTERNYM, "REGISTERNYM"},
    {proto::RPCCOMMAND_CREATEUNITDEFINITION, "CREATEUNITDEFINITION"},
    {proto::RPCCOMMAND_LISTUNITDEFINITIONS, "LISTUNITDEFINITIONS"},
    {proto::RPCCOMMAND_ISSUEUNITDEFINITION, "ISSUEUNITDEFINITION"},
    {proto::RPCCOMMAND_CREATEACCOUNT, "CREATEACCOUNT"},
    {proto::RPCCOMMAND_LISTACCOUNTS, "LISTACCOUNTS"},
    {proto::RPCCOMMAND_GETACCOUNTBALANCE, "GETACCOUNTBALANCE"},
    {proto::RPCCOMMAND_GETACCOUNTACTIVITY, "GETACCOUNTACTIVITY"},
    {proto::RPCCOMMAND_SENDPAYMENT, "SENDPAYMENT"},
    {proto::RPCCOMMAND_MOVEFUNDS, "MOVEFUNDS"},
    {proto::RPCCOMMAND_ADDCONTACT, "ADDCONTACT"},
    {proto::RPCCOMMAND_LISTCONTACTS, "LISTCONTACTS"},
    {proto::RPCCOMMAND_GETCONTACT, "GETCONTACT"},
    {proto::RPCCOMMAND_ADDCONTACTCLAIM, "ADDCONTACTCLAIM"},
    {proto::RPCCOMMAND_DELETECONTACTCLAIM, "DELETECONTACTCLAIM"},
    {proto::RPCCOMMAND_VERIFYCLAIM, "VERIFYCLAIM"},
    {proto::RPCCOMMAND_ACCEPTVERIFICATION, "ACCEPTVERIFICATION"},
    {proto::RPCCOMMAND_SENDCONTACTMESSAGE, "SENDCONTACTMESSAGE"},
    {proto::RPCCOMMAND_GETCONTACTACTIVITY, "GETCONTACTACTIVITY"},
};

const std::map<proto::RPCResponseCode, std::string> CLI::status_names_{
    {proto::RPCRESPONSE_INVALID, "INVALID"},
    {proto::RPCRESPONSE_SUCCESS, "SUCCESS"},
    {proto::RPCRESPONSE_BAD_SESSION, "BAD_SESSION"},
    {proto::RPCRESPONSE_NONE, "NONE"},
    {proto::RPCRESPONSE_PARTIAL, "PARTIAL"},
    {proto::RPCRESPONSE_QUEUED, "QUEUED"},
    {proto::RPCRESPONSE_UNNECESSARY, "UNNECESSARY"},
    {proto::RPCRESPONSE_ERROR, "ERROR"},
};

CLI::CLI(const api::Native& ot)
    : ot_(ot)
    , endpoint_(get_socket_path())
    , callback_(network::zeromq::ListenCallback::Factory(
          std::bind(&CLI::callback, this, std::placeholders::_1)))
    , socket_(ot.ZMQ().DealerSocket(callback_, true))
{
    OT_ASSERT(false == endpoint_.empty());

    set_keys(socket_);
    const auto connected = socket_->Start(endpoint_);

    OT_ASSERT(connected);
}

void CLI::add_client_session(
    const std::string& in,
    const network::zeromq::DealerSocket& socket)
{
    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_ADDCLIENTSESSION);
    out.set_session(-1);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = socket.Send(proto::ProtoAsData(out));

    OT_ASSERT(sent);
}

void CLI::add_client_session_response(const proto::RPCResponse& in)
{
    print_basic_info(in);
    otErr << "   Session: " << in.session() << std::endl;
}

void CLI::callback(network::zeromq::Message& in)
{
    const auto size = in.Body().size();

    if (1 > size) {
        otErr << __FUNCTION__ << ": Missing reply." << std::endl;

        return;
    }

    if (1 == size) {
        process_reply(in);
    } else if (2 == size) {
        process_push(in);
    } else {
        otErr << __FUNCTION__ << ": Invalid reply." << std::endl;
    }
}

void CLI::create_nym(
    const std::string& in,
    const network::zeromq::DealerSocket& socket)
{
    int instance{-1};
    int type{proto::CITEMTYPE_INDIVIDUAL};
    std::string name{""};
    std::string seed{""};
    int index{-1};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance));
    options.add_options()("type", po::value<int>(&type));
    options.add_options()("name", po::value<std::string>(&name));
    options.add_options()("seed", po::value<std::string>(&seed));
    options.add_options()("index", po::value<int>(&index));
    parse_command(in, options);

    if (-1 == instance) {
        otErr << __FUNCTION__ << ": Missing instance option" << std::endl;

        return;
    }

    if (name.empty()) {
        otErr << __FUNCTION__ << ": Missing name option" << std::endl;

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_CREATENYM);
    out.set_session(instance);
    auto& create = *out.mutable_createnym();
    create.set_version(CREATE_NYM_VERSION);
    create.set_type(static_cast<proto::ContactItemType>(type));
    create.set_name(name);
    create.set_seedid(seed);
    create.set_index(index);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = socket.Send(proto::ProtoAsData(out));

    OT_ASSERT(sent);
}

void CLI::create_nym_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& id : in.identifier()) {
        otErr << "   Nym ID: " << id << std::endl;
    }
}

std::string CLI::find_home()
{
    std::string output;
#ifdef __APPLE__
    output = OTPaths::AppDataFolder().Get();
#else
    std::string environment;
    const char* env = getenv("HOME");

    if (nullptr != env) { environment.assign(env); }

    if (!environment.empty()) {
        output = environment;
    } else {
        passwd* entry = getpwuid(getuid());
        const char* password = entry->pw_dir;
        output.assign(password);
    }

    if (output.empty()) {
        opentxs::otErr << __FUNCTION__
                       << ": Unable to determine the home directory."
                       << std::endl;
    }
#endif

    return output;
}

std::string CLI::get_command_name(const proto::RPCCommandType type)
{
    try {
        return command_names_.at(type);
    } catch (...) {
        return std::to_string(type);
    }
}

std::string CLI::get_json()
{
    const auto filename = find_home() + "/otagent.key";
    boost::system::error_code ec{};

    if (false == boost::filesystem::exists(filename, ec)) { return {}; }

    std::ifstream file(
        filename, std::ios::in | std::ios::ate | std::ios::binary);

    if (file.good()) {
        std::ifstream::pos_type pos = file.tellg();

        if ((0 >= pos) || (0xFFFFFFFF <= pos)) { return {}; }

        std::uint32_t size(pos);
        file.seekg(0, std::ios::beg);
        std::vector<char> bytes(size);
        file.read(&bytes[0], size);

        return std::string(&bytes[0], size);
    }

    return {};
}

std::string CLI::get_socket_path()
{
    std::string socket_path{"ipc://"};
    const std::string uid = std::to_string(getuid());
    std::string dir = "/run/user/" + uid;
    fs::path path(dir);
    fs::file_status status = fs::status(path);

    if (0 != (status.permissions() & fs::owner_write)) {
        socket_path += dir + "/otagent.sock";
    } else {
        dir = "/tmp/user/" + uid;
        path = dir;
        status = fs::status(path);

        if (0 != (status.permissions() & fs::owner_write)) {
            socket_path += dir + "/otagent.sock";
        }
    }

    return socket_path;
}

std::string CLI::get_status_name(const proto::RPCResponseCode code)
{
    try {
        return status_names_.at(code);
    } catch (...) {
        return std::to_string(code);
    }
}

void CLI::parse_command(
    const std::string& input,
    po::options_description& options)
{
    po::variables_map variables;
    po::store(
        po::command_line_parser(po::split_unix(input)).options(options).run(),
        variables);
    po::notify(variables);
}

void CLI::print_basic_info(const proto::RPCPush& in)
{
    otErr << " * Received RPC push notification for " << in.id() << "\n"
          << std::endl;
}

void CLI::print_basic_info(const proto::RPCResponse& in)
{
    otErr << " * Received RPC reply type: " << get_command_name(in.type())
          << "\n   Status: " << get_status_name(in.success()) << std::endl;
}

void CLI::process_push(network::zeromq::Message& in)
{
    const auto& frame = in.Body_at(1);
    const auto response =
        proto::RawToProto<proto::RPCPush>(frame.data(), frame.size());

    if (false == proto::Validate(response, SILENT)) {
        otErr << __FUNCTION__ << ": Invalid RPCPush." << std::endl;

        return;
    }

    try {
        auto& handler = *push_handlers_.at(response.type());
        handler(response);
    } catch (...) {
        otErr << __FUNCTION__ << ": Unhandled response type: "
              << std::to_string(response.type()) << std::endl;
    }
}

void CLI::process_reply(network::zeromq::Message& in)
{
    const auto& frame = in.Body_at(0);
    const auto response =
        proto::RawToProto<proto::RPCResponse>(frame.data(), frame.size());

    if (false == proto::Validate(response, SILENT)) {
        otErr << __FUNCTION__ << ": Invalid RPCResponse." << std::endl;

        return;
    }

    try {
        auto& handler = *response_handlers_.at(response.type());
        handler(response);
    } catch (...) {
        otErr << __FUNCTION__ << ": Unhandled response type: "
              << std::to_string(response.type()) << std::endl;
    }
}

int CLI::Run()
{
    std::string input{};
    otErr << "otctl shell mode activated" << std::endl;

    while (true) {
        std::getline(std::cin, input);

        if (input.empty()) { continue; }

        const auto first = input.substr(0, input.find(" "));

        if ("quit" == first) { break; }

        try {
            const auto command = commands_.at(first);
            const auto& processor = *processors_.at(command);
            processor(input, socket_);
        } catch (...) {
            otErr << "Unknown command" << std::endl;
        }
    }

    return 0;
}

void CLI::set_keys(network::zeromq::DealerSocket& socket)
{
    std::stringstream json(get_json());
    Json::Value root;
    json >> root;
    const auto main = root["otagent"];
    const auto serverKey = main["server_pubkey"].asString();
    const auto clientPrivateKey = main["client_privkey"].asString();
    const auto clientPublicKey = main["client_pubkey"].asString();
    socket.SetKeysZ85(serverKey, clientPrivateKey, clientPublicKey);
}

void CLI::task_complete_push(const proto::RPCPush& in)
{
    print_basic_info(in);
    const auto& task = in.taskcomplete();
    otErr << "   Type: TASK" << std::endl;
    otErr << "   ID: " << task.id() << std::endl;
    otErr << "   Result: " << ((task.result()) ? "success" : "failure")
          << std::endl;
}
}  // namespace opentxs::otctl
