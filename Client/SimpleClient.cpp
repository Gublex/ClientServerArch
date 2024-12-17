#include <iostream>
#include <fstream>
#include <string>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

const int max_length = 1024;

bool authenticate(const std::string& email, const std::string& password) {
    std::ifstream data_file("data.txt");
    std::string line;
    while (std::getline(data_file, line)) {
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

void send_message_to_server(boost::asio::io_service& io_service, const std::string& message, const std::string& recipient_email) {
    tcp::resolver resolver(io_service);
    tcp::resolver::query query("127.0.0.1", "60000");
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::socket socket(io_service);

    boost::asio::connect(socket, endpoint_iterator);

    // Отправка сообщения на сервер
    boost::asio::write(socket, boost::asio::buffer(message));

    // Ожидание ответа от сервера
    char reply[max_length];
    size_t reply_length = socket.read_some(boost::asio::buffer(reply));
    std::cout << "Server response: ";
    std::cout.write(reply, reply_length);
    std::cout << "\n";
}

void save_message_to_file(const std::string& recipient_email, const std::string& sender_email, const std::string& message) {
    std::ofstream output_file(recipient_email + ".txt", std::ios::app);
    if (output_file.is_open()) {
        output_file << "From: " << sender_email << "\n";
        output_file << "Message: " << message << "\n";
        output_file << "-----------------------------------\n";
    }
}

int main() {
    setlocale(0, "");
    std::string email, password, recipient_email, message;

    std::cout << "Enter your email: ";
    std::getline(std::cin, email);

    std::cout << "Enter your password: ";
    std::getline(std::cin, password);

    if (authenticate(email, password)) {
        std::cout << "Authentication successful!\n";

        std::cout << "Enter the recipient's email: ";
        std::getline(std::cin, recipient_email);

        std::cout << "Enter your message: ";
        std::getline(std::cin, message);

        // Отправляем сообщение на сервер
        boost::asio::io_service io_service;
        send_message_to_server(io_service, message, recipient_email);

        // Записываем сообщение в файл, названный по адресу получателя
        save_message_to_file(recipient_email, email, message);
    }
    else {
        std::cout << "Authentication failed! Invalid email or password.\n";
    }

    return 0;
}
