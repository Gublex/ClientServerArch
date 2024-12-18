#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <memory>

using boost::asio::ip::tcp;

const int max_length = 1024;

// Класс для обработки сессий клиента
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() {
        std::cout << "Client connected.\n";
        do_read();
    }

    tcp::socket& socket() {
        return socket_;
    }

private:
    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(data_, max_length),
            [this, self](boost::system::error_code error, std::size_t length) {
                if (!error) {
                    std::string received_data(data_, length);
                    std::cout << "Received: " << received_data << "\n";

                    // Обрабатываем команды клиента
                    process_client_request(received_data);
                }
                else {
                    std::cerr << "Error reading data: " << error.message() << "\n";
                }
            });
    }

    void process_client_request(const std::string& request) {
        std::istringstream iss(request);
        std::string command;
        iss >> command;

        std::string sender_email; // Добавляем переменную для хранения email отправителя

        if (command == "AUTH") {
            std::string email, password;
            iss >> email >> password;
            sender_email = email; // Сохраняем email отправителя при аутентификации
            if (authenticate_user(email, password)) {
                send_response("AUTH_SUCCESS\n");  // Добавляем символ новой строки
            }
            else {
                send_response("AUTH_FAILED\n");  // Добавляем символ новой строки
            }
        }
        else if (command == "MESSAGE") {
            std::string recipient_email, message;
            iss >> recipient_email;
            std::getline(iss, message);
            save_message(sender_email, recipient_email, message);  // Передаем email отправителя
            send_response("MESSAGE_SENT\n");  // Добавляем символ новой строки
        }
        else {
            send_response("INVALID_COMMAND\n");  // Добавляем символ новой строки
        }
        do_read();  // Ждем следующее сообщение от клиента
    }

    bool authenticate_user(const std::string& email, const std::string& password) {
        std::ifstream users_file("users.txt");
        std::string line;
        while (std::getline(users_file, line)) {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                std::string stored_email = line.substr(0, pos);
                std::string stored_password = line.substr(pos + 1);
                if (stored_email == email && stored_password == password) {
                    return true;
                }
            }
        }
        return false;
    }

    void save_message(const std::string& sender_email, const std::string& recipient_email, const std::string& message) {
        std::ofstream out_file(recipient_email + ".txt", std::ios::app);
        if (out_file.is_open()) {
            out_file << "From: " << sender_email << "\n";  // Сохраняем email отправителя
            out_file << "Message: " << message << "\n";
            out_file << "-----------------------------------\n";
            std::cout << "Message saved for recipient: " << recipient_email << "\n";
        }
    }

    void send_response(const std::string& response) {
        auto self(shared_from_this());
        boost::asio::async_write(
            socket_, boost::asio::buffer(response),
            [this, self](boost::system::error_code error, std::size_t /*length*/) {
                if (error) {
                    std::cerr << "Error sending response: " << error.message() << "\n";
                }
            });
    }

    tcp::socket socket_;
    char data_[max_length];
};

// Класс сервера
class Server {
public:
    Server(boost::asio::io_service& io_service, short port)
        : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
        start_accept();
    }

private:
    void start_accept() {
        auto new_session = std::make_shared<Session>(tcp::socket(acceptor_.get_executor()));
        acceptor_.async_accept(
            new_session->socket(),
            [this, new_session](boost::system::error_code error) {
                if (!error) {
                    new_session->start();
                }
                else {
                    std::cerr << "Error accepting connection: " << error.message() << "\n";
                }
                start_accept();
            });
    }

    tcp::acceptor acceptor_;
};

int main() {
    setlocale(0, "");
    try {
        boost::asio::io_service io_service;
        Server server(io_service, 993);
        std::cout << "Server is running on port 993...\n";
        io_service.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
