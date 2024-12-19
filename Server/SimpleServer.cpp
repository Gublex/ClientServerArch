#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <algorithm>
#include <map>

using boost::asio::ip::tcp;

const int max_length = 1024;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, std::map<std::string, unsigned short>& clientPorts)
        : socket_(std::move(socket)), sender_email_(""), clientPorts_(clientPorts) {}


    void start() {
        std::cout << "Client connected.\n";
        do_read();
    }

    tcp::socket& socket() {
        return socket_;
    }

    unsigned short getPort() const {
        return socket_.remote_endpoint().port();
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

        if (command == "AUTH") {
            std::string email, password;
            iss >> email >> password;
            if (authenticate_user(email, password)) {
                sender_email_ = email;
                clientPorts_[sender_email_] = getPort(); // Сохраняем порт после успешной аутентификации
                send_response("AUTH_SUCCESS\n");
            }
            else {
                send_response("AUTH_FAILED\n");
            }
        }
        else if (command == "MESSAGE") {
            if (sender_email_.empty()) {
                std::cout << "Sender email is empty, authentication required!\n";
                send_response("AUTH_REQUIRED\n");
                return;
            }

            std::string recipient_email, message;
            iss >> recipient_email;
            std::getline(iss, message);

            std::cout << "Sender email: " << sender_email_ << " (" << getPort() << ")\n";
            std::cout << "Recipient email: " << recipient_email << " (порт неизвестен до аутентификации)\n"; // Показать порт получателя если он есть.

            // Проверить, есть ли порт получателя в карте.
            auto it = clientPorts_.find(recipient_email);
            if (it != clientPorts_.end()) {
                std::cout << "Recipient port: " << it->second << std::endl;
            }

            save_message(sender_email_, recipient_email, message);
            send_response("MESSAGE_SENT\n");
        }
        else {
            send_response("INVALID_COMMAND\n");
        }
        do_read();
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
        if (sender_email.empty()) {
            std::cerr << "Error: sender_email is empty!\n";
            return;
        }

        std::ofstream out_file(recipient_email + ".txt", std::ios::app);
        if (out_file.is_open()) {
            out_file << "From: " << sender_email << "\n";  // Сохраняем email отправителя
            out_file << "Message: " << message << "\n";
            out_file << "-----------------------------------\n";
            std::cout << "Message saved for recipient: " << recipient_email << "\n";
        }
        else {
            std::cerr << "Error opening file for writing.\n";
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
    std::string sender_email_; // Сохраняем email отправителя
    std::map<std::string, unsigned short>& clientPorts_;
};

// Класс сервера
class Server {
public:
    Server(boost::asio::io_service& io_service, short port)
        : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)), clientPorts_() {
        start_accept();
    }
private:
    void start_accept() {
        auto new_session = std::make_shared<Session>(tcp::socket(acceptor_.get_executor()), clientPorts_);
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
    std::map<std::string, unsigned short> clientPorts_;
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
