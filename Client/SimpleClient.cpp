#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>
#include <string>

using boost::asio::ip::tcp;

enum { max_length = 1024 };

int main()
{
    setlocale(0, "");
    std::string server_ip = "127.0.0.1";
    std::string port = "60000";

    try
    {
        boost::asio::io_service io_service;

        // Создание resolver для получения конечной точки
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(server_ip, port);
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        // Создание и подключение сокета
        tcp::socket socket(io_service);
        boost::asio::connect(socket, endpoint_iterator);

        std::cout << "Connected to server " << server_ip << ":" << port << "\n";
        std::cout << "Enter 'exit' to quit.\n";

        while (true) // Бесконечный цикл для общения с сервером
        {
            std::cout << "\nEnter message: ";
            char request[max_length];
            std::cin.getline(request, max_length);

            if (std::strcmp(request, "exit") == 0) {
                std::cout << "Closing connection...\n";
                break; // Завершаем цикл и программу
            }

            size_t request_length = std::strlen(request);

            // Отправка данных через сокет
            boost::asio::write(socket, boost::asio::buffer(request, request_length));

            // Чтение ответа от сервера
            char reply[max_length];
            boost::system::error_code error;
            size_t reply_length = socket.read_some(boost::asio::buffer(reply), error);

            if (!error) {
                std::cout << "Reply from server: ";
                std::cout.write(reply, reply_length);
                std::cout << "\n";
            }
            else {
                std::cerr << "Error receiving response: " << error.message() << "\n";
                break; // Прерываем цикл при ошибке
            }
        }

        socket.close();
        std::cout << "Connection closed.\n";
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
