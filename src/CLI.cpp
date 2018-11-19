// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "CLI.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>

#if __has_include("json/json.h")
#include <json/json.h>
#elif __has_include("jsoncpp/json/json.h")
#include <jsoncpp/json/json.h>
#endif

extern "C" {
#include <pwd.h>
#include <unistd.h>
}

namespace fs = boost::filesystem;
namespace zmq = opentxs::network::zeromq;

#define ACCEPTPENDINGPAYMENT_VERSION 1
#define ADD_CONTACT_VERSION 1
#define API_ARG_VERSION 1
#define CREATE_NYM_VERSION 1
#define CREATE_UNITDEFINITION_VERSION 1
#define GETWORKFLOW_VERSION 1
#define HDSEED_VERSION 1
#define MOVEFUNDS_VERSION 1
#define RPC_COMMAND_VERSION 2
#define SENDPAYMENT_VERSION 1

namespace opentxs::otctl
{
const std::map<std::string, proto::RPCCommandType> CLI::commands_{
    {"acceptpendingpayment", proto::RPCCOMMAND_ACCEPTPENDINGPAYMENTS},
    {"addclient", proto::RPCCOMMAND_ADDCLIENTSESSION},
    {"addcontact", proto::RPCCOMMAND_ADDCONTACT},
    {"addserver", proto::RPCCOMMAND_ADDSERVERSESSION},
    {"createaccount", proto::RPCCOMMAND_CREATEACCOUNT},
    {"createcompatibleaccount", proto::RPCCOMMAND_CREATECOMPATIBLEACCOUNT},
    {"createnym", proto::RPCCOMMAND_CREATENYM},
    {"createunitdefinition", proto::RPCCOMMAND_CREATEUNITDEFINITION},
    {"getaccountactivity", proto::RPCCOMMAND_GETACCOUNTACTIVITY},
    {"getaccountbalance", proto::RPCCOMMAND_GETACCOUNTBALANCE},
    {"getcompatibleaccounts", proto::RPCCOMMAND_GETCOMPATIBLEACCOUNTS},
    {"getnym", proto::RPCCOMMAND_GETNYM},
    {"getpendingpayments", proto::RPCCOMMAND_GETPENDINGPAYMENTS},
    {"getseed", proto::RPCCOMMAND_GETHDSEED},
    {"getserver", proto::RPCCOMMAND_GETSERVERCONTRACT},
    {"getworkflow", proto::RPCCOMMAND_GETWORKFLOW},
    {"importseed", proto::RPCCOMMAND_IMPORTHDSEED},
    {"importserver", proto::RPCCOMMAND_IMPORTSERVERCONTRACT},
    {"issueunitdefinition", proto::RPCCOMMAND_ISSUEUNITDEFINITION},
    {"listaccounts", proto::RPCCOMMAND_LISTACCOUNTS},
    {"listclientsessions", proto::RPCCOMMAND_LISTCLIENTSESSIONS},
    {"listcontacts", proto::RPCCOMMAND_LISTCONTACTS},
    {"listnyms", proto::RPCCOMMAND_LISTNYMS},
    {"listseeds", proto::RPCCOMMAND_LISTHDSEEDS},
    {"listservers", proto::RPCCOMMAND_LISTSERVERCONTRACTS},
    {"listserversessions", proto::RPCCOMMAND_LISTSERVERSESSIONS},
    {"listunitdefinitions", proto::RPCCOMMAND_LISTUNITDEFINITIONS},
    {"movefunds", proto::RPCCOMMAND_MOVEFUNDS},
    {"registernym", proto::RPCCOMMAND_REGISTERNYM},
    {"sendcheque", proto::RPCCOMMAND_SENDPAYMENT},
    {"transfer", proto::RPCCOMMAND_SENDPAYMENT},
    {"gettransactiondata", proto::RPCCOMMAND_GETTRANSACTIONDATA},
};
const std::map<proto::RPCPushType, CLI::PushHandler> CLI::push_handlers_{
    {proto::RPCPUSH_ACCOUNT, &CLI::account_event_push},
    {proto::RPCPUSH_TASK, &CLI::task_complete_push},
};
const std::map<proto::RPCCommandType, CLI::ResponseHandler>
    CLI::response_handlers_{
        {proto::RPCCOMMAND_ACCEPTPENDINGPAYMENTS,
         &CLI::accept_pending_payment_response},
        {proto::RPCCOMMAND_ADDCLIENTSESSION, &CLI::add_session_response},
        {proto::RPCCOMMAND_ADDCONTACT, &CLI::add_contact_response},
        {proto::RPCCOMMAND_ADDSERVERSESSION, &CLI::add_session_response},
        {proto::RPCCOMMAND_CREATEACCOUNT, &CLI::create_account_response},
        {proto::RPCCOMMAND_CREATECOMPATIBLEACCOUNT,
         &CLI::create_account_response},
        {proto::RPCCOMMAND_CREATENYM, &CLI::create_nym_response},
        {proto::RPCCOMMAND_CREATEUNITDEFINITION,
         &CLI::create_unit_definition_response},
        {proto::RPCCOMMAND_GETACCOUNTACTIVITY,
         &CLI::get_account_activity_response},
        {proto::RPCCOMMAND_GETACCOUNTBALANCE,
         &CLI::get_account_balance_response},
        {proto::RPCCOMMAND_GETCOMPATIBLEACCOUNTS,
         &CLI::get_compatible_accounts_response},
        {proto::RPCCOMMAND_GETNYM, &CLI::get_nym_response},
        {proto::RPCCOMMAND_GETPENDINGPAYMENTS,
         &CLI::get_pending_payments_response},
        {proto::RPCCOMMAND_GETHDSEED, &CLI::get_seed_response},
        {proto::RPCCOMMAND_GETSERVERCONTRACT,
         &CLI::get_server_contract_response},
        {proto::RPCCOMMAND_GETWORKFLOW, &CLI::get_workflow_response},
        {proto::RPCCOMMAND_IMPORTHDSEED, &CLI::import_seed_response},
        {proto::RPCCOMMAND_IMPORTSERVERCONTRACT,
         &CLI::import_server_contract_response},
        {proto::RPCCOMMAND_ISSUEUNITDEFINITION,
         &CLI::issue_unit_definition_response},
        {proto::RPCCOMMAND_LISTACCOUNTS, &CLI::list_accounts_response},
        {proto::RPCCOMMAND_LISTCLIENTSESSIONS, &CLI::list_session_response},
        {proto::RPCCOMMAND_LISTCONTACTS, &CLI::list_contacts_response},
        {proto::RPCCOMMAND_LISTNYMS, &CLI::list_nyms_response},
        {proto::RPCCOMMAND_LISTHDSEEDS, &CLI::list_seeds_response},
        {proto::RPCCOMMAND_LISTSERVERCONTRACTS, &CLI::list_servers_response},
        {proto::RPCCOMMAND_LISTSERVERSESSIONS, &CLI::list_session_response},
        {proto::RPCCOMMAND_LISTUNITDEFINITIONS,
         &CLI::list_unit_definitions_response},
        {proto::RPCCOMMAND_MOVEFUNDS, &CLI::move_funds_response},
        {proto::RPCCOMMAND_REGISTERNYM, &CLI::register_nym_response},
        {proto::RPCCOMMAND_SENDPAYMENT, &CLI::send_payment_response},
        {proto::RPCCOMMAND_GETTRANSACTIONDATA,
         &CLI::get_transaction_data_response},
    };
const std::map<proto::RPCCommandType, CLI::Processor> CLI::processors_{
    {proto::RPCCOMMAND_ACCEPTPENDINGPAYMENTS, &CLI::accept_pending_payment},
    {proto::RPCCOMMAND_ADDCLIENTSESSION, &CLI::add_client_session},
    {proto::RPCCOMMAND_ADDCONTACT, &CLI::add_contact},
    {proto::RPCCOMMAND_ADDSERVERSESSION, &CLI::add_server_session},
    {proto::RPCCOMMAND_CREATEACCOUNT, &CLI::create_account},
    {proto::RPCCOMMAND_CREATECOMPATIBLEACCOUNT,
     &CLI::create_compatible_account},
    {proto::RPCCOMMAND_CREATENYM, &CLI::create_nym},
    {proto::RPCCOMMAND_CREATEUNITDEFINITION, &CLI::create_unit_definition},
    {proto::RPCCOMMAND_GETACCOUNTACTIVITY, &CLI::get_account_activity},
    {proto::RPCCOMMAND_GETACCOUNTBALANCE, &CLI::get_account_balance},
    {proto::RPCCOMMAND_GETCOMPATIBLEACCOUNTS, &CLI::get_compatible_accounts},
    {proto::RPCCOMMAND_GETNYM, &CLI::get_nym},
    {proto::RPCCOMMAND_GETPENDINGPAYMENTS, &CLI::get_pending_payments},
    {proto::RPCCOMMAND_GETHDSEED, &CLI::get_seed},
    {proto::RPCCOMMAND_GETSERVERCONTRACT, &CLI::get_server_contract},
    {proto::RPCCOMMAND_IMPORTSERVERCONTRACT, &CLI::import_server_contract},
    {proto::RPCCOMMAND_IMPORTHDSEED, &CLI::import_seed},
    {proto::RPCCOMMAND_ISSUEUNITDEFINITION, &CLI::issue_unit_definition},
    {proto::RPCCOMMAND_LISTACCOUNTS, &CLI::list_accounts},
    {proto::RPCCOMMAND_LISTCLIENTSESSIONS, &CLI::list_client_sessions},
    {proto::RPCCOMMAND_LISTCONTACTS, &CLI::list_contacts},
    {proto::RPCCOMMAND_LISTNYMS, &CLI::list_nyms},
    {proto::RPCCOMMAND_LISTHDSEEDS, &CLI::list_seeds},
    {proto::RPCCOMMAND_LISTSERVERCONTRACTS, &CLI::list_server_contracts},
    {proto::RPCCOMMAND_LISTSERVERSESSIONS, &CLI::list_server_sessions},
    {proto::RPCCOMMAND_LISTUNITDEFINITIONS, &CLI::list_unit_definitions},
    {proto::RPCCOMMAND_MOVEFUNDS, &CLI::move_funds},
    {proto::RPCCOMMAND_REGISTERNYM, &CLI::register_nym},
    {proto::RPCCOMMAND_SENDPAYMENT, &CLI::send_payment},
    {proto::RPCCOMMAND_GETWORKFLOW, &CLI::get_workflow},
    {proto::RPCCOMMAND_GETTRANSACTIONDATA, &CLI::get_transaction_data},
};

const std::map<proto::RPCCommandType, std::string> CLI::command_names_{
    {proto::RPCCOMMAND_ADDCLIENTSESSION, "ADDCLIENTSESSION"},
    {proto::RPCCOMMAND_ADDSERVERSESSION, "ADDSERVERSESSION"},
    {proto::RPCCOMMAND_LISTCLIENTSESSIONS, "LISTCLIENTSSESSIONS"},
    {proto::RPCCOMMAND_LISTSERVERSESSIONS, "LISTSERVERSSESSIONS"},
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
    {proto::RPCCOMMAND_GETSERVERCONTRACT, "GETSERVERCONTRACT"},
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
    {proto::RPCCOMMAND_GETPENDINGPAYMENTS, "GETPENDINGPAYMENTS"},
    {proto::RPCCOMMAND_ACCEPTPENDINGPAYMENTS, "ACCEPTPENDINGPAYMENTS"},
    {proto::RPCCOMMAND_CREATECOMPATIBLEACCOUNT, "CREATECOMPATIBLEACCOUNT"},
    {proto::RPCCOMMAND_GETCOMPATIBLEACCOUNTS, "GETCOMPATIBLEACCOUNTS"},
    {proto::RPCCOMMAND_GETWORKFLOW, "GETWORKFLOW"},
    {proto::RPCCOMMAND_GETUNITDEFINITION, "GETUNITDEFINITION"},
    {proto::RPCCOMMAND_GETTRANSACTIONDATA, "GETTRANSACTIONDATA"},
};

const std::map<proto::RPCResponseCode, std::string> CLI::status_names_{
    {proto::RPCRESPONSE_INVALID, "INVALID"},
    {proto::RPCRESPONSE_SUCCESS, "SUCCESS"},
    {proto::RPCRESPONSE_BAD_SESSION, "BAD_SESSION"},
    {proto::RPCRESPONSE_NONE, "NONE"},
    {proto::RPCRESPONSE_QUEUED, "QUEUED"},
    {proto::RPCRESPONSE_UNNECESSARY, "UNNECESSARY"},
    {proto::RPCRESPONSE_RETRY, "RETRY"},
    {proto::RPCRESPONSE_NO_PATH_TO_RECIPIENT, "NO_PATH_TO_RECIPIENT"},
    {proto::RPCRESPONSE_ERROR, "ERROR"},
    {proto::RPCRESPONSE_UNIMPLEMENTED, "UNIMPLEMENTED"},
};

const std::map<proto::AccountEventType, std::string> CLI::account_push_names_{
    {proto::ACCOUNTEVENT_INCOMINGCHEQUE, "INCOMING CHEQUE"},
    {proto::ACCOUNTEVENT_OUTGOINGCHEQUE, "OUTGOING CHEQUE"},
    {proto::ACCOUNTEVENT_INCOMINGTRANSFER, "INCOMING TRANSFER"},
    {proto::ACCOUNTEVENT_OUTGOINGTRANSFER, "OUTGOING TRANSFER"},
};

CLI::CLI(const api::Native& ot, const po::variables_map& options)
    : options_(options)
    , endpoint_(get_socket_path(options_))
    , callback_(zmq::ListenCallback::Factory(
          std::bind(&CLI::callback, this, std::placeholders::_1)))
    , socket_(ot.ZMQ().DealerSocket(callback_, zmq::Socket::Direction::Connect))
    , log_callback_(zmq::ListenCallback::Factory(
          std::bind(&CLI::remote_log, this, std::placeholders::_1)))
    , log_subscriber_(ot.ZMQ().SubscribeSocket(log_callback_))
{
    OT_ASSERT(false == endpoint_.empty());

    set_keys(options_, socket_);
    auto connected = socket_->Start(endpoint_);

    OT_ASSERT(connected);

    if (options_.count("logendpoint") != 0) {
        connected =
            log_subscriber_->Start(options_["logendpoint"].as<std::string>());

        OT_ASSERT(connected);
    }
}

void CLI::account_event_push(const proto::RPCPush& in)
{
    print_basic_info(in);
    const auto& event = in.accountevent();
    std::stringstream time{};
    time << std::time_t{event.timestamp()};

    LogOutput("   Type: ACCOUNT").Flush();
    LogOutput("   Account ID: ")(event.id()).Flush();
    LogOutput("   Event type: ")(get_account_push_name(event.type())).Flush();
    LogOutput("   Contact: ")(event.contact()).Flush();
    LogOutput("   Workflow ID: ")(event.workflow()).Flush();
    LogOutput("   Finalized amount: ")(event.amount()).Flush();
    LogOutput("   Pending amount: ")(event.pendingamount()).Flush();
    LogOutput("   Timestamp: ")(time.str()).Flush();
    LogOutput("   Memo: ")(event.memo()).Flush();
}

void CLI::accept_pending_payment(
    const std::string& in,
    const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string destinationAccount{""};
    std::string workflow{""};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()(
        "destinationaccount",
        po::value<std::string>(&destinationAccount),
        "<string>");
    options.add_options()(
        "workflow", po::value<std::string>(&workflow), "<string>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (destinationAccount.empty()) {
        LogOutput(__FUNCTION__)(": Missing destination account option").Flush();

        return;
    }

    if (workflow.empty()) {
        LogOutput(__FUNCTION__)(": Missing workflow option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_ACCEPTPENDINGPAYMENTS);
    out.set_session(instance);

    auto& acceptpendingpayment = *out.add_acceptpendingpayment();
    acceptpendingpayment.set_version(ACCEPTPENDINGPAYMENT_VERSION);
    acceptpendingpayment.set_destinationaccount(destinationAccount);
    acceptpendingpayment.set_workflow(workflow);

    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::accept_pending_payment_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& taskid : in.identifier()) {

        LogOutput("   Accept Payment task id: ")(taskid).Flush();
    }
}

void CLI::add_client_session(
    const std::string& in,
    const zmq::DealerSocket& socket)
{
    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_ADDCLIENTSESSION);
    out.set_session(-1);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::add_contact(const std::string& in, const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string label{""};
    std::string nymid{""};
    std::string paymentcode{""};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()("label", po::value<std::string>(&label), "<string>");
    options.add_options()("nymid", po::value<std::string>(&nymid), "<string>");
    options.add_options()(
        "paymentcode", po::value<std::string>(&paymentcode), "<string>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (label.empty()) {
        LogOutput(__FUNCTION__)(": Missing label option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_ADDCONTACT);
    out.set_session(instance);
    auto& addcontact = *out.add_addcontact();
    addcontact.set_version(ADD_CONTACT_VERSION);
    addcontact.set_label(label);
    addcontact.set_paymentcode(paymentcode);
    addcontact.set_nymid(nymid);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::add_contact_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& id : in.identifier()) {
        LogOutput("   Contact ID: ")(id).Flush();
    }
}

void CLI::add_server_session(
    const std::string& in,
    const zmq::DealerSocket& socket)
{
    std::string ip{};
    std::string onion{};
    int port{-1};

    po::options_description options("Options");
    options.add_options()("ip", po::value<std::string>(&ip), "<string>");
    options.add_options()("port", po::value<int>(&port), "<number>");
    options.add_options()("onion", po::value<std::string>(&onion), "<string>");
    parse_command(in, options);

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_ADDSERVERSESSION);
    out.set_session(-1);

    if (0 < port) {
        auto& arg1 = *out.add_arg();
        arg1.set_version(API_ARG_VERSION);
        arg1.set_key("commandport");
        arg1.add_value(std::to_string(port));
        auto& arg2 = *out.add_arg();
        arg2.set_version(API_ARG_VERSION);
        arg2.set_key("listencommand");
        arg2.add_value(std::to_string(port));
    }

    if (false == ip.empty()) {
        auto& arg = *out.add_arg();
        arg.set_version(API_ARG_VERSION);
        arg.set_key("externalip");
        arg.add_value(ip);
    }

    if (false == onion.empty()) {
        auto& arg = *out.add_arg();
        arg.set_version(API_ARG_VERSION);
        arg.set_key("onion");
        arg.add_value(onion);
    }

    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::add_session_response(const proto::RPCResponse& in)
{
    print_basic_info(in);
    LogOutput("   Session: ")(in.session()).Flush();
}

void CLI::callback(zmq::Message& in)
{
    const auto size = in.Body().size();

    if (1 > size) {
        LogOutput(__FUNCTION__)(": Missing reply.").Flush();

        return;
    }

    if (1 == size) {
        process_reply(in);
    } else if (2 == size) {
        process_push(in);
    } else {
        LogOutput(__FUNCTION__)(": Invalid reply.").Flush();
    }
}

void CLI::create_account(const std::string& in, const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string owner{""};
    std::string server{""};
    std::string unitDefinition{""};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()("owner", po::value<std::string>(&owner), "<string>");
    options.add_options()(
        "server", po::value<std::string>(&server), "<string>");
    options.add_options()(
        "unitdefinition", po::value<std::string>(&unitDefinition), "<string>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (owner.empty()) {
        LogOutput(__FUNCTION__)(": Missing owner option").Flush();

        return;
    }

    if (server.empty()) {
        LogOutput(__FUNCTION__)(": Missing server option").Flush();

        return;
    }

    if (unitDefinition.empty()) {
        LogOutput(__FUNCTION__)(": Missing unitdefinition option").Flush();

        return;
    }

    proto::RPCCommand command{};
    command.set_version(RPC_COMMAND_VERSION);
    command.set_cookie(Identifier::Random()->str());
    command.set_type(proto::RPCCOMMAND_CREATEACCOUNT);
    command.set_session(instance);
    command.set_owner(owner);
    command.set_notary(server);
    command.set_unit(unitDefinition);

    const auto valid = proto::Validate(command, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, command);

    OT_ASSERT(sent);
}

void CLI::create_account_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& id : in.identifier()) {
        LogOutput("   Account ID: ")(id).Flush();
    }
}

void CLI::create_compatible_account(
    const std::string& in,
    const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string nymID{""};
    std::string workflowID{""};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()("owner", po::value<std::string>(&nymID), "<string>");
    options.add_options()(
        "workflow", po::value<std::string>(&workflowID), "<string>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (nymID.empty()) {
        LogOutput(__FUNCTION__)(": Missing owner id option").Flush();

        return;
    }

    if (workflowID.empty()) {
        LogOutput(__FUNCTION__)(": Missing workflow id option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_CREATECOMPATIBLEACCOUNT);
    out.set_session(instance);
    out.set_owner(nymID);
    out.add_identifier(workflowID);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::create_nym(const std::string& in, const zmq::DealerSocket& socket)
{
    int instance{-1};
    int type{proto::CITEMTYPE_INDIVIDUAL};
    std::string name{""};
    std::string seed{""};
    int index{-1};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()("type", po::value<int>(&type), "<number>");
    options.add_options()("name", po::value<std::string>(&name), "<string>");
    options.add_options()("seed", po::value<std::string>(&seed), "<string>");
    options.add_options()("index", po::value<int>(&index), "<number>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (name.empty()) {
        LogOutput(__FUNCTION__)(": Missing name option").Flush();

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

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::create_nym_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& id : in.identifier()) {
        LogOutput("   Nym ID: ")(id).Flush();
    }
}

void CLI::create_unit_definition(
    const std::string& in,
    const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string nymID{""};
    std::string name{""};
    std::string symbol{""};
    std::string primaryUnitName{""};
    std::string fractionalUnitName{""};
    std::string tickerSymbol{""};
    int power{-1};
    std::string terms{""};
    int unitOfAccount{proto::CITEMTYPE_UNKNOWN};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()("owner", po::value<std::string>(&nymID), "<string>");
    options.add_options()("name", po::value<std::string>(&name), "<string>");
    options.add_options()(
        "symbol", po::value<std::string>(&symbol), "<string>");
    options.add_options()(
        "primaryunitname",
        po::value<std::string>(&primaryUnitName),
        "<string>");
    options.add_options()(
        "fractionalunitname",
        po::value<std::string>(&fractionalUnitName),
        "<string>");
    options.add_options()(
        "tickersymbol", po::value<std::string>(&tickerSymbol), "<string>");
    options.add_options()("power", po::value<int>(&power), "<number>");
    options.add_options()("terms", po::value<std::string>(&terms), "<string>");
    options.add_options()(
        "unitofaccount", po::value<int>(&unitOfAccount), "<number>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (nymID.empty()) {
        LogOutput(__FUNCTION__)(": Missing owner option").Flush();

        return;
    }

    if (name.empty()) {
        LogOutput(__FUNCTION__)(": Missing name option").Flush();

        return;
    }

    if (symbol.empty()) {
        LogOutput(__FUNCTION__)(": Missing symbol option").Flush();

        return;
    }

    if (primaryUnitName.empty()) {
        LogOutput(__FUNCTION__)(": Missing primary unit name option").Flush();

        return;
    }

    if (fractionalUnitName.empty()) {
        LogOutput(__FUNCTION__)(": Missing fractional unit name option")
            .Flush();

        return;
    }

    if (tickerSymbol.empty()) {
        LogOutput(__FUNCTION__)(": Missing ticker symbol option").Flush();

        return;
    }

    if (-1 == power) {
        LogOutput(__FUNCTION__)(": Missing power option").Flush();

        return;
    }

    if (terms.empty()) {
        LogOutput(__FUNCTION__)(": Missing terms option").Flush();

        return;
    }

    if (proto::CITEMTYPE_UNKNOWN == unitOfAccount) {
        LogOutput(__FUNCTION__)(": Missing unit of account option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_CREATEUNITDEFINITION);
    out.set_session(instance);
    out.set_owner(nymID);
    auto& create = *out.mutable_createunit();
    create.set_version(CREATE_UNITDEFINITION_VERSION);
    create.set_name(name);
    create.set_symbol(symbol);
    create.set_primaryunitname(primaryUnitName);
    create.set_fractionalunitname(fractionalUnitName);
    create.set_tla(tickerSymbol);
    create.set_power(power);
    create.set_terms(terms);
    create.set_unitofaccount(
        static_cast<proto::ContactItemType>(unitOfAccount));
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::create_unit_definition_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& id : in.identifier()) {
        LogOutput("   Unit Definition ID: ")(id).Flush();
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
        LogOutput(__FUNCTION__)(": Unable to determine the home directory.")
            .Flush();
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

void CLI::get_compatible_accounts(
    const std::string& in,
    const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string nymID{""};
    std::string workflowID{""};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()("owner", po::value<std::string>(&nymID), "<string>");
    options.add_options()(
        "workflow", po::value<std::string>(&workflowID), "<string>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (nymID.empty()) {
        LogOutput(__FUNCTION__)(": Missing owner id option").Flush();

        return;
    }

    if (workflowID.empty()) {
        LogOutput(__FUNCTION__)(": Missing workflow id option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_GETCOMPATIBLEACCOUNTS);
    out.set_session(instance);
    out.set_owner(nymID);
    out.add_identifier(workflowID);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::get_compatible_accounts_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& id : in.identifier()) {

        LogOutput("   Account ID: ")(id).Flush();
    }
}

std::string CLI::get_json(const po::variables_map& cli)
{
    std::string filename{};
    const auto& cliValue = cli["keyfile"];

    if (cliValue.empty()) {
        filename = find_home() + "/otagent.key";
    } else {
        filename = cli["keyfile"].as<std::string>();
    }

    boost::system::error_code ec{};

    if (false == boost::filesystem::exists(filename, ec)) { return {}; }

    std::ifstream file(
        filename, std::ios::in | std::ios::ate | std::ios::binary);

    if (file.good()) {
        std::ifstream::pos_type pos = file.tellg();

        if ((0 >= pos) || (0xFFFFFFFF <= pos)) { return {}; }

        std::uint64_t size(pos);
        file.seekg(0, std::ios::beg);
        std::vector<char> bytes(size);
        file.read(&bytes[0], size);

        return std::string(&bytes[0], size);
    }

    return {};
}

void CLI::get_account_activity(
    const std::string& in,
    const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string accountID{""};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()(
        "account", po::value<std::string>(&accountID), "<string>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (accountID.empty()) {
        LogOutput(__FUNCTION__)(": Missing account id option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_GETACCOUNTACTIVITY);
    out.set_session(instance);
    out.add_identifier(accountID);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::get_account_activity_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& accountevent : in.accountevent()) {
        LogOutput("   Account ID: ")(accountevent.id()).Flush();
        LogOutput("   Workflow ID: ")(accountevent.workflow()).Flush();
        LogOutput("   Amount: ")(accountevent.amount()).Flush();
        LogOutput("   Pending Amount: ")(accountevent.pendingamount()).Flush();
        LogOutput("   Memo: ")(accountevent.memo()).Flush();
        LogOutput("   UUID: ")(accountevent.uuid()).Flush();
    }
}

void CLI::get_account_balance(
    const std::string& in,
    const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string accountID{""};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()(
        "account", po::value<std::string>(&accountID), "<string>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (accountID.empty()) {
        LogOutput(__FUNCTION__)(": Missing acount id option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_GETACCOUNTBALANCE);
    out.set_session(instance);
    out.add_identifier(accountID);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::get_account_balance_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& accountdata : in.balance()) {

        LogOutput("   Account ID: ")(accountdata.id()).Flush();
        LogOutput("   Balance: ")(accountdata.balance()).Flush();
        LogOutput("   Pending Balance: ")(accountdata.pendingbalance()).Flush();
    }
}

std::string CLI::get_account_push_name(const proto::AccountEventType type)
{
    try {
        return account_push_names_.at(type);
    } catch (...) {
        return std::to_string(type);
    }
}

void CLI::get_nym(const std::string& in, const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string ownerID{""};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()(
        "owner", po::value<std::string>(&ownerID), "<string>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (ownerID.empty()) {
        LogOutput(__FUNCTION__)(": Missing owner id option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_GETNYM);
    out.set_session(instance);
    out.add_identifier(ownerID);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::get_nym_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& credentialindex : in.nym()) {
        LogOutput("   Nym ID: ")(credentialindex.nymid()).Flush();
        LogOutput("   Revision: ")(credentialindex.revision()).Flush();
        LogOutput("   Active Credential Count: ")(
            credentialindex.activecredentials_size())
            .Flush();
        LogOutput("   Revoked Credential Count: ")(
            credentialindex.revokedcredentials_size())
            .Flush();
    }
}

void CLI::get_pending_payments(
    const std::string& in,
    const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string ownerID{""};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()(
        "owner", po::value<std::string>(&ownerID), "<string>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (ownerID.empty()) {
        LogOutput(__FUNCTION__)(": Missing owner id option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_GETPENDINGPAYMENTS);
    out.set_session(instance);
    out.set_owner(ownerID);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::get_pending_payments_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& accountevent : in.accountevent()) {
        std::string eventType = "Incoming cheque";
        if (proto::ACCOUNTEVENT_INCOMINGINVOICE == accountevent.type()) {
            eventType = "Incoming invoice";
        }
        LogOutput("   Account Event: ")(eventType).Flush();
        LogOutput("   Contact ID: ")(accountevent.contact()).Flush();
        LogOutput("   Workflow ID: ")(accountevent.workflow()).Flush();
        LogOutput("   Pending Amount: ")(accountevent.pendingamount()).Flush();
    }
}

void CLI::get_seed(const std::string& in, const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string seedID{""};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()("seed", po::value<std::string>(&seedID), "<string>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (seedID.empty()) {
        LogOutput(__FUNCTION__)(": Missing seed id option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_GETHDSEED);
    out.set_session(instance);
    out.add_identifier(seedID);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::get_seed_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& seed : in.seed()) {
        LogOutput("   Seed ID: ")(seed.id()).Flush();
        LogOutput("   Seed Words: ")(seed.words()).Flush();
        LogOutput("   Seed Passphrase: ")(seed.passphrase()).Flush();
    }
}

void CLI::get_server_contract(
    const std::string& in,
    const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string serverID{""};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()(
        "server", po::value<std::string>(&serverID), "<string>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (serverID.empty()) {
        LogOutput(__FUNCTION__)(": Missing server id option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_GETSERVERCONTRACT);
    out.set_session(instance);
    out.add_identifier(serverID);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::get_server_contract_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& id : in.notary()) {
        auto output =
            proto::ProtoAsArmored(id, String::Factory("SERVER CONTRACT"));

        OT_ASSERT(!output->empty());

        LogOutput("   Server Contract:\n")(output).Flush();
    }
}

std::string CLI::get_socket_path(const po::variables_map& cli)
{
    std::string output{};
    const auto& cliValue = cli["endpoint"];

    if (cliValue.empty()) {
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

        output = socket_path;
    } else {
        output = cliValue.as<std::string>();
    }

    LogOutput("Connecting to ")(output).Flush();

    return output;
}

std::string CLI::get_status_name(const proto::RPCResponseCode code)
{
    try {
        return status_names_.at(code);
    } catch (...) {
        return std::to_string(code);
    }
}

void CLI::get_transaction_data(
    const std::string& in,
    const network::zeromq::DealerSocket& socket)
{
    int instance{-1};
    std::string uuid{""};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()("uuid", po::value<std::string>(&uuid), "<string>");
    auto hasOptions = parse_command(in, options);

    if (false == hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (uuid.empty()) {
        LogOutput(__FUNCTION__)(": Missing uuid option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_GETTRANSACTIONDATA);
    out.set_session(instance);
    out.add_identifier(uuid);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::get_transaction_data_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& data : in.transactiondata()) {
        LogOutput("   UUID: ")(data.uuid()).Flush();
        LogOutput("   Type: ")(data.type()).Flush();

        for (const auto& account : data.sourceaccounts()) {
            LogOutput("   Source account: ")(account).Flush();
        }

        for (const auto& account : data.destinationaccounts()) {
            LogOutput("   Destination account: ")(account).Flush();
        }

        LogOutput("   Amount: ")(data.amount()).Flush();
        LogOutput("   State: ")(data.state()).Flush();
    }
}

void CLI::get_workflow(const std::string& in, const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string nymID{""};
    std::string workflowID{""};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()("nym", po::value<std::string>(&nymID), "<string>");
    options.add_options()(
        "workflow", po::value<std::string>(&workflowID), "<string>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (nymID.empty()) {
        LogOutput(__FUNCTION__)(": Missing nym id option").Flush();

        return;
    }

    if (workflowID.empty()) {
        LogOutput(__FUNCTION__)(": Missing workflow id option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_GETWORKFLOW);
    out.set_session(instance);
    auto& getworkflow = *out.add_getworkflow();
    getworkflow.set_version(GETWORKFLOW_VERSION);
    getworkflow.set_nymid(nymID);
    getworkflow.set_workflowid(workflowID);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::get_workflow_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& workflow : in.workflow()) {
        LogOutput(__FUNCTION__)(": Version ")(workflow.version())(" workflow")
            .Flush();
        LogOutput(__FUNCTION__)(": * ID: ")(workflow.id()).Flush();
        LogOutput(__FUNCTION__)(": * Type: ")(workflow.type()).Flush();
        LogOutput(__FUNCTION__)(": * State: ")(workflow.state()).Flush();

        for (const auto& source : workflow.source()) {
            LogOutput(__FUNCTION__)(": * Source version: ")(source.version())
                .Flush();
            LogOutput(__FUNCTION__)(":   * id: ")(source.id()).Flush();
            LogOutput(__FUNCTION__)(":   * revision: ")(source.revision())
                .Flush();
            LogOutput(__FUNCTION__)(":   * item: ").Flush();
            LogOutput(source.item()).Flush();
        }

        LogOutput(__FUNCTION__)(": * Notary: ")(workflow.notary()).Flush();

        for (const auto& party : workflow.party()) {
            LogOutput(__FUNCTION__)(": * Party nym id: ")(party).Flush();
        }

        for (const auto& unit : workflow.unit()) {
            LogOutput(__FUNCTION__)(": * Unit definition id: ")(unit).Flush();
        }

        for (const auto& account : workflow.account()) {
            LogOutput(__FUNCTION__)(": * Account id: ")(account).Flush();
        }

        for (const auto& event : workflow.event()) {
            LogOutput(__FUNCTION__)(": * Event version: ")(event.version())
                .Flush();
            LogOutput(__FUNCTION__)(":   * type: ")(event.type()).Flush();

            for (const auto& item : event.item()) {
                LogOutput(__FUNCTION__)(":   * item: ").Flush();
                LogOutput(item).Flush();
            }

            LogOutput(__FUNCTION__)(":   * timestamp: ")(event.time()).Flush();
            LogOutput(__FUNCTION__)(":   * method: ")(event.method()).Flush();
            LogOutput(__FUNCTION__)(":   * transport: ")(event.transport())
                .Flush();
            LogOutput(__FUNCTION__)(":   * nym: ")(event.nym()).Flush();
            LogOutput(__FUNCTION__)(":   * success: ")(
                (event.success() ? "true" : "false"))
                .Flush();
            LogOutput(__FUNCTION__)(":   * memo: ")(event.memo()).Flush();
        }

        LogOutput(__FUNCTION__)(": * Archived: ")(
            (workflow.archived() ? "true" : "false"))
            .Flush();
    }
}

void CLI::import_seed(const std::string& in, const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string words{""};
    std::string passphrase{""};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()("words", po::value<std::string>(&words), "<string>");
    options.add_options()(
        "passphrase", po::value<std::string>(&passphrase), "<string>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (words.empty()) {
        LogOutput(__FUNCTION__)(": Missing words option").Flush();

        return;
    }

    proto::RPCCommand command{};
    command.set_version(RPC_COMMAND_VERSION);
    command.set_cookie(Identifier::Random()->str());
    command.set_type(proto::RPCCOMMAND_IMPORTHDSEED);
    command.set_session(instance);
    auto& seed = *command.mutable_hdseed();
    seed.set_version(HDSEED_VERSION);
    seed.set_words(words);
    seed.set_passphrase(passphrase);

    const auto valid = proto::Validate(command, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, command);

    OT_ASSERT(sent);
}

void CLI::import_seed_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& id : in.identifier()) {
        LogOutput("   Seed ID: ")(id).Flush();
    }
}

void CLI::import_server_contract(
    const std::string& in,
    const zmq::DealerSocket& socket)
{
    int instance{-1};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    LogOutput("Please paste a server contract,\n")(
        "followed by an EOF or a ~ on a line by itself:\n")
        .Flush();

    std::string input = OT_CLI_ReadUntilEOF();
    if ("" == input) {
        LogOutput(__FUNCTION__)("Error: you did not paste a server contract.\n")
            .Flush();
        return;
    }

    proto::RPCCommand command{};
    command.set_version(RPC_COMMAND_VERSION);
    command.set_cookie(Identifier::Random()->str());
    command.set_type(proto::RPCCOMMAND_IMPORTSERVERCONTRACT);
    command.set_session(instance);
    auto& server = *command.add_server();
    server = proto::StringToProto<proto::ServerContract>(
        String::Factory(input.c_str()));

    const auto valid = proto::Validate(command, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, command);

    OT_ASSERT(sent);
}

void CLI::import_server_contract_response(const proto::RPCResponse& in)
{
    print_basic_info(in);
}

void CLI::issue_unit_definition(
    const std::string& in,
    const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string owner{""};
    std::string server{""};
    std::string unitDefinition{""};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()("owner", po::value<std::string>(&owner), "<string>");
    options.add_options()(
        "server", po::value<std::string>(&server), "<string>");
    options.add_options()(
        "unitdefinition", po::value<std::string>(&unitDefinition), "<string>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (owner.empty()) {
        LogOutput(__FUNCTION__)(": Missing owner option").Flush();

        return;
    }

    if (server.empty()) {
        LogOutput(__FUNCTION__)(": Missing server option").Flush();

        return;
    }

    if (unitDefinition.empty()) {
        LogOutput(__FUNCTION__)(": Missing unitdefinition option").Flush();

        return;
    }

    proto::RPCCommand command{};
    command.set_version(RPC_COMMAND_VERSION);
    command.set_cookie(Identifier::Random()->str());
    command.set_type(proto::RPCCOMMAND_ISSUEUNITDEFINITION);
    command.set_session(instance);
    command.set_owner(owner);
    command.set_notary(server);
    command.set_unit(unitDefinition);

    const auto valid = proto::Validate(command, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, command);

    OT_ASSERT(sent);
}

void CLI::issue_unit_definition_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& id : in.identifier()) {
        LogOutput("   Issuer account ID: ")(id).Flush();
    }
}

void CLI::list_accounts(const std::string& in, const zmq::DealerSocket& socket)
{
    int instance{-1};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_LISTACCOUNTS);
    out.set_session(instance);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::list_accounts_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& id : in.identifier()) {
        LogOutput("   Account ID: ")(id).Flush();
    }
}

void CLI::list_client_sessions(
    const std::string& in,
    const zmq::DealerSocket& socket)
{
    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_LISTCLIENTSESSIONS);
    out.set_session(-1);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::list_contacts(const std::string& in, const zmq::DealerSocket& socket)
{
    int instance{-1};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_LISTCONTACTS);
    out.set_session(instance);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::list_contacts_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& id : in.identifier()) {
        LogOutput("   Contact ID: ")(id).Flush();
    }
}

void CLI::list_nyms(const std::string& in, const zmq::DealerSocket& socket)
{
    int instance{-1};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_LISTNYMS);
    out.set_session(instance);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::list_nyms_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& id : in.identifier()) {
        LogOutput("   Nym ID: ")(id).Flush();
    }
}

void CLI::list_seeds(const std::string& in, const zmq::DealerSocket& socket)
{
    int instance{-1};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_LISTHDSEEDS);
    out.set_session(instance);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::list_seeds_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& id : in.identifier()) {
        LogOutput("   Seed ID: ")(id).Flush();
    }
}

void CLI::list_server_contracts(
    const std::string& in,
    const zmq::DealerSocket& socket)
{
    int instance{-1};
    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_LISTSERVERCONTRACTS);
    out.set_session(instance);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::list_server_sessions(
    const std::string& in,
    const zmq::DealerSocket& socket)
{
    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_LISTSERVERSESSIONS);
    out.set_session(-1);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::list_servers_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& id : in.identifier()) {
        LogOutput("   Notary: ")(id).Flush();
    }
}

void CLI::list_session_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& session : in.sessions()) {
        LogOutput("   Instance: ")(session.instance()).Flush();
    }
}

void CLI::list_unit_definitions(
    const std::string& in,
    const zmq::DealerSocket& socket)
{
    int instance{-1};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_LISTUNITDEFINITIONS);
    out.set_session(instance);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::list_unit_definitions_response(const proto::RPCResponse& in)
{
    print_basic_info(in);

    for (const auto& id : in.identifier()) {
        LogOutput("   Unit definition: ")(id).Flush();
    }
}

void CLI::move_funds(const std::string& in, const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string sourceAccountID{""};
    std::string destinationAccountID{""};
    std::string memo{""};
    int amount{-1};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()(
        "sourceaccount", po::value<std::string>(&sourceAccountID), "<string>");
    options.add_options()(
        "destinationaccount",
        po::value<std::string>(&destinationAccountID),
        "<string>");
    options.add_options()("memo", po::value<std::string>(&memo), "<string>");
    options.add_options()("amount", po::value<int>(&amount), "<number>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (sourceAccountID.empty()) {
        LogOutput(__FUNCTION__)(": Missing source account id option").Flush();

        return;
    }

    if (destinationAccountID.empty()) {
        LogOutput(__FUNCTION__)(": Missing destination account id option")
            .Flush();

        return;
    }

    if (0 >= amount) {
        LogOutput(__FUNCTION__)(": Missing amount option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_MOVEFUNDS);
    out.set_session(instance);

    auto& movefunds = *out.mutable_movefunds();
    movefunds.set_version(MOVEFUNDS_VERSION);
    movefunds.set_type(proto::RPCPAYMENTTYPE_TRANSFER);
    movefunds.set_sourceaccount(sourceAccountID);
    movefunds.set_destinationaccount(destinationAccountID);
    if (!memo.empty()) { movefunds.set_memo(memo); }
    movefunds.set_amount(amount);

    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::move_funds_response(const proto::RPCResponse& in)
{
    print_basic_info(in);
}

bool CLI::parse_command(
    const std::string& input,
    po::options_description& options)
{
    po::variables_map variables;
    po::store(
        po::command_line_parser(po::split_unix(input)).options(options).run(),
        variables);
    po::notify(variables);

    if (variables.empty()) { return false; }

    return true;
}

void CLI::print_basic_info(const proto::RPCPush& in)
{
    LogOutput(" * Received RPC push notification for ")(in.id()).Flush();
}

void CLI::print_basic_info(const proto::RPCResponse& in)
{
    LogOutput(" * Received RPC reply type: ")(get_command_name(in.type()))
        .Flush();

    for (auto status : in.status()) {
        LogOutput("   Status: ")(get_status_name(status.code())).Flush();

        if (proto::RPCRESPONSE_QUEUED == status.code() &&
            static_cast<int>(status.index()) < in.task_size()) {
            LogOutput("   Task ID: ")(in.task(status.index()).id()).Flush();
        }
    }
}

void CLI::print_options_description(po::options_description& options)
{
    std::stringstream str;
    for (auto option : options.options()) {
        str << option->format_name() << " " << option->description() << " ";
    }
    LogOutput(str.str()).Flush();
}

void CLI::process_push(zmq::Message& in)
{
    const auto& frame = in.Body_at(1);
    const auto response =
        proto::RawToProto<proto::RPCPush>(frame.data(), frame.size());

    if (false == proto::Validate(response, VERBOSE)) {
        LogOutput(__FUNCTION__)(": Invalid RPCPush.").Flush();

        return;
    }

    try {
        auto& handler = *push_handlers_.at(response.type());
        handler(response);
    } catch (...) {
        LogOutput(__FUNCTION__)(": Unhandled response type: ")(response.type())
            .Flush();
    }
}

void CLI::process_reply(zmq::Message& in)
{
    const auto& frame = in.Body_at(0);
    const auto response =
        proto::RawToProto<proto::RPCResponse>(frame.data(), frame.size());

    if (false == proto::Validate(response, VERBOSE)) {
        LogOutput(__FUNCTION__)(": Invalid RPCResponse.").Flush();

        return;
    }

    try {
        auto& handler = *response_handlers_.at(response.type());
        handler(response);
    } catch (...) {
        LogOutput(__FUNCTION__)(": Unhandled response type: ")(response.type())
            .Flush();
    }
}

void CLI::register_nym(const std::string& in, const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string nymID{""};
    std::string serverID{""};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()("owner", po::value<std::string>(&nymID), "<string>");
    options.add_options()(
        "server", po::value<std::string>(&serverID), "<string>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (-1 == instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (nymID.empty()) {
        LogOutput(__FUNCTION__)(": Missing nym id option").Flush();

        return;
    }

    if (serverID.empty()) {
        LogOutput(__FUNCTION__)(": Missing server id option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_REGISTERNYM);
    out.set_session(instance);
    out.add_associatenym(nymID);
    out.set_owner(nymID);
    out.set_notary(serverID);
    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::register_nym_response(const proto::RPCResponse& in)
{
    print_basic_info(in);
}

void CLI::remote_log(network::zeromq::Message& in)
{
    if (3 > in.Body().size()) { return; }

    int level{-1};
    const auto& levelFrame = in.Body_at(0);
    const auto& messageFrame = in.Body_at(1);
    const auto& id = in.Body_at(2);
    OTPassword::safe_memcpy(
        &level, sizeof(level), levelFrame.data(), levelFrame.size());
    std::cout << "Remote log received:\n"
              << "Level: " << level << "\n"
              << "Thread ID: " << std::string(id) << "\n"
              << "Message:\n"
              << std::string(messageFrame) << std::endl;
}

int CLI::Run()
{
    std::string input{};
    LogOutput("otctl shell mode activated").Flush();

    while (true) {
        std::getline(std::cin, input);

        if (input.empty()) { continue; }

        const auto first = input.substr(0, input.find(" "));

        if ("quit" == first) { break; }

        try {
            const auto command = commands_.at(first);
            const auto& processor = *processors_.at(command);
            processor(input, socket_);
        } catch (po::error& err) {
            LogOutput("Error processing command: ")(err.what()).Flush();
        } catch (...) {
            LogOutput("Unknown command").Flush();
        }
    }

    return 0;
}

bool CLI::send_message(
    const zmq::DealerSocket& socket,
    const proto::RPCCommand command)
{
    auto message = zmq::Message::Factory();
    message->AddFrame();
    message->AddFrame(proto::ProtoAsData(command));

    OT_ASSERT(0 == message->Header().size());
    OT_ASSERT(1 == message->Body().size());

    return socket.Send(message);
}

// Invokes RPCCOMMAND_SENDPAYMENT for transaction type RPCPAYMENTTYPE_CHEQUE
// only.
void CLI::send_cheque(const std::string& in, const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string contactID{""};
    std::string sourceAccountID{""};
    std::string memo{""};
    int amount{-1};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()(
        "contact", po::value<std::string>(&contactID), "<string>");
    options.add_options()(
        "sourceaccount", po::value<std::string>(&sourceAccountID), "<string>");
    options.add_options()("memo", po::value<std::string>(&memo), "<string>");
    options.add_options()("amount", po::value<int>(&amount), "<number>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (0 > instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (contactID.empty()) {
        LogOutput(__FUNCTION__)(": Missing contactid option").Flush();

        return;
    }

    if (sourceAccountID.empty()) {
        LogOutput(__FUNCTION__)(": Missing sourceaccountid option").Flush();

        return;
    }

    if (0 >= amount) {
        LogOutput(__FUNCTION__)(": Missing amount option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_SENDPAYMENT);
    out.set_session(instance);

    auto& sendpayment = *out.mutable_sendpayment();
    sendpayment.set_version(SENDPAYMENT_VERSION);
    sendpayment.set_type(proto::RPCPAYMENTTYPE_CHEQUE);
    sendpayment.set_contact(contactID);
    sendpayment.set_sourceaccount(sourceAccountID);
    if (!memo.empty()) { sendpayment.set_memo(memo); }
    sendpayment.set_amount(amount);

    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

void CLI::send_payment(const std::string& in, const zmq::DealerSocket& socket)
{
    const auto first = in.substr(0, in.find(" "));
    if ("sendcheque" == first) {
        send_cheque(in, socket);
    } else if ("transfer" == first) {
        transfer(in, socket);
    }
}

void CLI::send_payment_response(const proto::RPCResponse& in)
{
    print_basic_info(in);
}

void CLI::set_keys(const po::variables_map& cli, zmq::DealerSocket& socket)
{
    std::stringstream json(get_json(cli));
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
    LogOutput("   Type: TASK").Flush();
    LogOutput("   ID: ")(task.id()).Flush();
    LogOutput("   Result: ")(((task.result()) ? "success" : "failure")).Flush();
}

// Invokes RPCCOMMAND_SENDPAYMENT for transaction type RPCPAYMENTTYPE_TRANSFER
// only.
void CLI::transfer(const std::string& in, const zmq::DealerSocket& socket)
{
    int instance{-1};
    std::string contactID{""};
    std::string sourceAccountID{""};
    std::string destinationAccountID{""};
    std::string memo{""};
    int amount{-1};

    po::options_description options("Options");
    options.add_options()("instance", po::value<int>(&instance), "<number>");
    options.add_options()(
        "contact", po::value<std::string>(&contactID), "<string>");
    options.add_options()(
        "sourceaccount", po::value<std::string>(&sourceAccountID), "<string>");
    options.add_options()(
        "destinationaccount",
        po::value<std::string>(&destinationAccountID),
        "<string>");
    options.add_options()("memo", po::value<std::string>(&memo), "<string>");
    options.add_options()("amount", po::value<int>(&amount), "<number>");
    auto hasOptions = parse_command(in, options);

    if (!hasOptions) {
        print_options_description(options);
        return;
    }

    if (0 > instance) {
        LogOutput(__FUNCTION__)(": Missing instance option").Flush();

        return;
    }

    if (contactID.empty()) {
        LogOutput(__FUNCTION__)(": Missing contactid option").Flush();

        return;
    }

    if (sourceAccountID.empty()) {
        LogOutput(__FUNCTION__)(": Missing sourceaccountid option").Flush();

        return;
    }

    if (destinationAccountID.empty()) {
        LogOutput(__FUNCTION__)(": Missing destinationaccountid option")
            .Flush();

        return;
    }

    if (0 >= amount) {
        LogOutput(__FUNCTION__)(": Missing amount option").Flush();

        return;
    }

    proto::RPCCommand out{};
    out.set_version(RPC_COMMAND_VERSION);
    out.set_cookie(Identifier::Random()->str());
    out.set_type(proto::RPCCOMMAND_SENDPAYMENT);
    out.set_session(instance);

    auto& sendpayment = *out.mutable_sendpayment();
    sendpayment.set_version(SENDPAYMENT_VERSION);
    sendpayment.set_type(proto::RPCPAYMENTTYPE_TRANSFER);
    sendpayment.set_contact(contactID);
    sendpayment.set_sourceaccount(sourceAccountID);
    sendpayment.set_destinationaccount(destinationAccountID);
    if (!memo.empty()) { sendpayment.set_memo(memo); }
    sendpayment.set_amount(amount);

    const auto valid = proto::Validate(out, VERBOSE);

    OT_ASSERT(valid);

    const auto sent = send_message(socket, out);

    OT_ASSERT(sent);
}

}  // namespace opentxs::otctl
