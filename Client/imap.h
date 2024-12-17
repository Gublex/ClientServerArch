#pragma once
#ifndef IMAP_H
#define IMAP_H

#include <string>
#include <boost/asio.hpp>

class IMAPClient {
public:
    IMAPClient(const std::string& server_ip, int port);
    ~IMAPClient();

    bool connect();
    bool login(const std::string& email, const std::string& password);
    bool select_mailbox(const std::string& mailbox);
    void logout();
    bool send_command(const std::string& command, const std::string& argument = "");
    std::string receive_response();

private:
    boost::asio::io_service io_service_;
    boost::asio::ip::tcp::socket socket_;
    std::string server_ip_;
    int port_;
};

#endif // IMAP_H