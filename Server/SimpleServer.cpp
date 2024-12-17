#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <thread>
#include <memory>
#include <vector>

using boost::asio::ip::tcp;
using namespace std::chrono_literals;

const int max_length = 1024;
std::string server_ip = "127.0.0.1";
int port = 60000;
const int timeout_duration = 30 * 60; // Таймаут в секундах (30 минут)

// Класс для обработки одного клиента
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, boost::asio::io_service& io_service)
        : socket_(std::move(socket)), timer_(io_service) {}

    void start() {
        std::cout << "Client connected: "
            << socket_.remote_endpoint().address().to_string()
            << ":" << socket_.remote_endpoint().port() << "\n";
        start_timer();
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
                    // Сбрасываем таймер при получении данных
                    reset_timer();

                    // Выводим сообщение и порт клиента
                    std::cout << "Received message from "
                        << socket_.remote_endpoint().port() << ": "
                        << std::string(data_, length) << "\n";

                    do_write(length);
                }
                else {
                    handle_disconnect(error);
                }
            });
    }

    void do_write(std::size_t length) {
        auto self(shared_from_this());
        boost::asio::async_write(
            socket_, boost::asio::buffer(data_, length),
            [this, self](boost::system::error_code error, std::size_t /*length*/) {
                if (!error) {
                    do_read();
                }
                else {
                    handle_disconnect(error);
                }
            });
    }

    void start_timer() {
        timer_.expires_from_now(std::chrono::seconds(timeout_duration));
        timer_.async_wait([self = shared_from_this(), this](const boost::system::error_code& error) {
            if (!error) {
                std::cerr << "Timeout: Client inactive for 30 minutes. Disconnecting: "
                    << socket_.remote_endpoint().address().to_string() << ":"
                    << socket_.remote_endpoint().port() << "\n";
                socket_.close();
            }
            });
    }

    void reset_timer() {
        timer_.cancel();
        start_timer();
    }

    void handle_disconnect(const boost::system::error_code& error) {
        std::cerr << "Client disconnected: "
            << socket_.remote_endpoint().address().to_string() << ":"
            << socket_.remote_endpoint().port()
            << " (" << error.message() << ")\n";
        socket_.close();
    }

    tcp::socket socket_;
    boost::asio::steady_timer timer_; // Таймер для отслеживания таймаута
    char data_[max_length];
};

// Класс сервера для обработки новых подключений
class Server {
public:
    Server(boost::asio::io_service& io_service, short port)
        : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)), io_service_(io_service) {
        start_accept();
    }

private:
    void start_accept() {
        auto new_session = std::make_shared<Session>(tcp::socket(acceptor_.get_executor()), io_service_);

        acceptor_.async_accept(new_session->socket(),
            [this, new_session](boost::system::error_code error) {
                if (!error) {
                    new_session->start();
                }
                start_accept();
            });
    }

    tcp::acceptor acceptor_;
    boost::asio::io_service& io_service_;
};

int main() {
    setlocale(0, "");
    try {
        boost::asio::io_service io_service;

        // Создаем сервер на указанном порту
        Server server(io_service, port);

        std::cout << "Server is running on port " << port
            << ". Waiting for clients...\n";

        // Запускаем сервис в несколько потоков
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

    //f
    //fjjfjf
    return 0;
}
