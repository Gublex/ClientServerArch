#pragma once
#ifndef IMAP_H
#define IMAP_H

#include <iostream>
#include <fstream>
#include <string>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

const int max_length = 1024;

class IMAPClient {
public:
    // Конструктор
    IMAPClient();

    // Метод для аутентификации пользователя
    bool authenticate(const std::string& email, const std::string& password);

    // Метод для отправки сообщения на сервер
    void send_message_to_server(const std::string& message, const std::string& recipient_email);

    // Метод для сохранения сообщения в файл
    void save_message_to_file(const std::string& recipient_email, const std::string& sender_email, const std::string& message);

private:
    boost::asio::io_service io_service_; // Boost IO service для отправки данных
};

#endif // IMAP_H
