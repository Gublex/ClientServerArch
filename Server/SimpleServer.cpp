#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <thread>
#include <memory>
#include <vector>

using boost::asio::ip::tcp;

const int max_length = 1024;
std::string server_ip = "127.0.0.1";
int port = 60000;
//

// Класс для обработки одного клиента
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() {
        std::cout << "Client connected: "
            << socket_.remote_endpoint().address().to_string()
            << ":" << socket_.remote_endpoint().port() << "\n";
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
                    std::cout << "Received message: "
                        << std::string(data_, length) << "\n";

                    do_write(length); // Отправляем ответ клиенту
                }
                else {
                    std::cerr << "Client disconnected: "
                        << socket_.remote_endpoint().address().to_string()
                        << ":" << socket_.remote_endpoint().port() << "\n";
                    socket_.close();
                }
            });
    }

    void do_write(std::size_t length) {
        auto self(shared_from_this());
        boost::asio::async_write(
            socket_, boost::asio::buffer(data_, length),
            [this, self](boost::system::error_code error, std::size_t /*length*/) {
                if (!error) {
                    do_read(); // Снова ждем сообщения от клиента
                }
                else {
                    std::cerr << "Write error: " << error.message() << "\n";
                    socket_.close();
                }
            });
    }

    tcp::socket socket_;
    char data_[max_length]; // Буфер для чтения и записи
};

// Класс сервера для обработки новых подключений
class Server {
public:
    Server(boost::asio::io_service& io_service, short port)
        : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
        start_accept();
    }

private:
    void start_accept() {
        auto new_session = std::make_shared<Session>(tcp::socket(acceptor_.get_executor()));

        acceptor_.async_accept(new_session->socket(),
            [this, new_session](boost::system::error_code error) {
                if (!error) {
                    new_session->start(); // Начинаем обслуживание клиента
                }
                start_accept(); // Продолжаем принимать новые подключения
            });
    }

    tcp::acceptor acceptor_;
};

int main() {
    try {
        boost::asio::io_service io_service;

        // Создаем сервер на указанном порту
        Server server(io_service, port);

        std::cout << "Server is running on port " << port
            << ". Waiting for clients...\n";

        // Запускаем сервис в отдельном потоке
        std::vector<std::thread> threads;
        for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
            threads.emplace_back([&io_service]() { io_service.run(); });
        }

        for (auto& t : threads) {
            t.join();
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
