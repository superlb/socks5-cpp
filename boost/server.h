#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

class Session : public std::enable_shared_from_this<Session> {
 public:
  Session(boost::asio::io_context& ioc, boost::asio::ip::tcp::socket&& socket)
      : out_socket(ioc),
        in_socket(std::move(socket)),
        in_buf(4096),
        out_buf(4096) {
    in_host = in_socket.remote_endpoint().address().to_string();
    in_port = std::to_string(in_socket.remote_endpoint().port());
  }
  ~Session() {}
  void run();

 private:
  void readAuthHandle(boost::system::error_code ec, std::size_t length);
  void writeAuthHandle(boost::system::error_code ec, std::size_t length);
  void readEstablishmentHandle(boost::system::error_code ec,
                               std::size_t length);
  void writeEstablishmentHandle(boost::system::error_code ec,
                                std::size_t length);
  void readForwardingHandle(int direction, boost::system::error_code ec,
                            std::size_t length);
  void writeForwardingHandle(int direction, boost::system::error_code ec,
                             std::size_t length);
  boost::asio::ip::tcp::socket in_socket;
  boost::asio::ip::tcp::socket out_socket;
  std::vector<char> in_buf;
  std::vector<char> out_buf;
  std::string in_host;
  std::string in_port;
  std::string out_host;
  std::string out_port;
};
class Listener : public std::enable_shared_from_this<Listener> {
 public:
  Listener(boost::asio::io_context& ioc,
           boost::asio::ip::tcp::endpoint endpoint)
      : ioc(ioc), listen_acceptor(ioc, endpoint){};
  ~Listener() {}
  void run();

 private:
  void do_accept();
  void on_accept(boost::system::error_code ec,
                 boost::asio::ip::tcp::socket socket);
  boost::asio::ip::tcp::acceptor listen_acceptor;
  boost::asio::io_context& ioc;
};
class Server {
 public:
  Server() {}
  ~Server() {}
  void run(unsigned short port);
};
