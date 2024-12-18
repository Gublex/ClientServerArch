#include "imap.h"

IMAPClient::IMAPClient() : io_service_() {}

// Реализация функции аутентификации
bool IMAPClient::authenticate(const std::string& email, const std::string& password) {
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

// Реализация функции отправки сообщения на сервер
void IMAPClient::send_message_to_server(const std::string& message, const std::string& recipient_email) {
    tcp::resolver resolver(io_service_);
    tcp::resolver::query query("127.0.0.1", "993");
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::socket socket(io_service_);

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

// Реализация функции сохранения сообщения в файл
void IMAPClient::save_message_to_file(const std::string& recipient_email, const std::string& sender_email, const std::string& message) {
    std::ofstream output_file(recipient_email + ".txt", std::ios::app);
    if (output_file.is_open()) {
        output_file << "From: " << sender_email << "\n";
        output_file << "Message: " << message << "\n";
        output_file << "-----------------------------------\n";
    }
}
