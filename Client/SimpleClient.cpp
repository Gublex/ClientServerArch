#include <boost/asio.hpp>
#include <iostream>
#include <sstream>
#include <string>

using boost::asio::ip::tcp;

const int max_length = 1024;

class IMAPClient {
public:
    IMAPClient(const std::string& server_ip, int port)
        : socket_(io_service_), server_ip_(server_ip), port_(port) {}

    bool connect() {
        try {
            tcp::resolver resolver(io_service_);
            tcp::resolver::query query(server_ip_, std::to_string(port_));
            tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

            boost::asio::connect(socket_, endpoint_iterator);
            std::cout << "Connected to server successfully.\n";
            return true;
        }
        catch (const boost::system::system_error& e) {
            std::cerr << "Connection error: " << e.what() << "\n";
            return false;
        }
        catch (const std::exception& e) {
            std::cerr << "Unexpected error during connection: " << e.what() << "\n";
            return false;
        }
    }

    bool authenticate(const std::string& email, const std::string& password) {
        if (email.empty() || password.empty()) {
            std::cerr << "Email or password is empty. Authentication aborted.\n";
            return false;
        }

        try {
            std::ostringstream oss;
            oss << "AUTH " << email << " " << password;
            std::string auth_message = oss.str();

            send_request(auth_message);
            std::string response = receive_response();
            if (response == "AUTH_SUCCESS") {
                std::cout << "Authentication successful!\n";
                return true;
            }
            else {
                std::cout << "Authentication failed.\n";
                return false;
            }
        }
        catch (const boost::system::system_error& e) {
            std::cerr << "Authentication error: " << e.what() << "\n";
            return false;
        }
    }

    void send_message(const std::string& recipient_email, const std::string& message) {
        if (recipient_email.empty() || message.empty()) {
            std::cerr << "Recipient email or message is empty. Sending aborted.\n";
            return;
        }

        try {
            std::ostringstream oss;
            oss << "MESSAGE " << recipient_email << " " << message;
            std::string msg = oss.str();

            send_request(msg);
            std::string response = receive_response();
            if (response == "MESSAGE_SENT") {
                std::cout << "Message sent successfully!\n";
            }
            else {
                std::cout << "Failed to send the message.\n";
            }
        }
        catch (const boost::system::system_error& e) {
            std::cerr << "Message sending error: " << e.what() << "\n";
        }
    }

private:
    void send_request(const std::string& request) {
        try {
            boost::asio::write(socket_, boost::asio::buffer(request.data(), request.size()));
        }
        catch (const boost::system::system_error& e) {
            std::cerr << "Error while sending request: " << e.what() << "\n";
            throw; // Пробрасываем исключение для обработки выше
        }
    }

    std::string receive_response() {
        try {
            boost::asio::streambuf buffer;
            boost::asio::read_until(socket_, buffer, "\n"); // Читаем до разделителя (новая строка)
            std::istream is(&buffer);
            std::string response;
            std::getline(is, response);
            return response;
        }
        catch (const boost::system::system_error& e) {
            std::cerr << "Error while receiving response: " << e.what() << "\n";
            throw; // Пробрасываем исключение для обработки выше
        }
    }

    boost::asio::io_service io_service_;
    tcp::socket socket_;
    std::string server_ip_;
    int port_;
};

int main() {
    try {
        IMAPClient client("127.0.0.1", 993);

        if (!client.connect()) {
            return 1;
        }

        std::string email, password, recipient_email, message;

        std::cout << "Enter your email: ";
        std::getline(std::cin, email);

        std::cout << "Enter your password: ";
        std::getline(std::cin, password);

        if (client.authenticate(email, password)) {
            std::cout << "Enter recipient email: ";
            std::getline(std::cin, recipient_email);

            std::cout << "Enter your message: ";
            std::getline(std::cin, message);

            client.send_message(recipient_email, message);
        }
    }
    catch (const boost::system::system_error& e) {
        std::cerr << "Boost system error: " << e.what() << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << "\n";
    }
    catch (...) {
        std::cerr << "An unknown error occurred.\n";
    }

    return 0;
}
