#include <boost/asio.h>

#include <string>

class Session : public std::enable_shared_from_this<Session> {
 public:
  Session(boost::asio::socket&& socket) : out_socket(socket.get_executor()),resolver(socket.get_executor()),in_socket(std::move(socket)),in_buf(1024),out_buf(1024) {}
  ~Session();
  void run();
  void authHandle(boost::system::error_code ec, std::size_t length);

 private:
  boost::asio::ip::tcp::socket in_socket;
  boost::asio::ip::tcp::socket out_socket;
  boost::asio::ip::tcp::resolver resolver;
  std::vector<char> in_buf;
  std::vector<char> out_buf;
};
class Listener : public std::enable_shared_from_this<Listener> {
 public:
  Listener(boost::asio::io_context ioc, boost::asio::tcp::endpoint endpoint)
      : ioc(ioc), listen_acceptor(ioc, endpoint){};
  ~Listener();
  tcp::acceptor listen_acceptor;
  net::io_context& ioc;
  void run();
  void do_accept();
  void on_accept(boost::system::error_code ec, boost::asio::tcp::socket socket);
};
class Server {
 public:
  Server() {}
  ~Server();
  void run();
};
