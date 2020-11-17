#include "server.h"

void Server::run(unsigned short port) {
  constexpr int threads = 10;
  boost::asio::io_context ioc{threads};
  std::make_shared<listener>(
      ioc, boost::asio::tcp::endpoint{boost::asio::tcp::v4(), port})
      ->run();
  boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
  signals.async_wait(
      [&ioc](const boost::system::error_code&, int) { ioc.stop(); });
  std::vector<std::thread> v;
  v.reserve(threads - 1);
  for (auto i = threads - 1; i > 0; --i) v.emplace_back([&ioc] { ioc.run(); });
  ioc.run();
  for (auto& t : v) t.join();
}

void Listener::run() {
  boost::asio::dispatch(listen_acceptor.get_executor(),
                        std::bind(&Listener::do_accept, shared_from_this()));
}

void Listener::do_accept() {
  listen_acceptor.async_accept(
      boost::asio::make_strand(ioc),
      std::bind(&Listener::on_accept, shared_from_this()));
}

void Listener::on_accept(boost::system::error_code ec,
                         boost::asio::tcp::socket socket) {
  if (ec) {
  } else {
    std::make_shared<Session>(std::move(socket))->run();
  }
  do_accept();
}

void Session::run() {
  in_socket.async_receive(
      boost::asio::buffer(in_buf),
      std::bind(&Session::readAuthHandle, shared_from_this()));
}
void Session::readAuthHandle(boost::system::error_code ec, std::size_t length) {
  if (ec) {
  } else {
    in_socket.async_send(
        boost::asio::buffer(in_buf, 2),
        std::bind(&Session::writeAuthHandle, shared_from_this()));
  }
}
void Session::writeAuthHandle(boost::system::error_code ec,
                              std::size_t length) {
  if (ec) {
  } else {
    in_socket.async_receive(
        boost::asio::buffer(in_buf),
        std::bind(&Session::readEstablishmentHandle, shared_from_this()));
  }
}
void Session::readEstablishmentHandle(boost::system::error_code ec,
                                      std::size_t length) {
  if (ec) {
  } else {
    out_socket.async_connect();
    in_socket.async_send();
  }
}
void Session::writeEstablishmentHandle(boost::system::error_code ec,
                                       std::size_t length) {
  if (ec) {
  } else {
    in_socket.async_receive(
        boost::asio::buffer(in_buf),
        std::bind(&Session::readForwardingHandle, shared_from_this(), 1));
    out_socket.async_receive(
        boost::asio::buffer(out_buf),
        std::bind(&Session::readForwardingHandle, shared_from_this(), 2));
  }
}
void Session::readForwardingHandle(int direction, boost::system::error_code ec,
                                   std::size_t length) {
  if (ec) {
  } else {
    if (direction == 1) {
      out_socket.async_send(
          boost::asio::buffer(in_buf, length),
          std::bind(&Session::writeForwardingHandle, shared_from_this(), 1));
    } else if (direction == 2) {
        in_socket.async_send(
          boost::asio::buffer(out_buf, length),
          std::bind(&Session::writeForwardingHandle, shared_from_this(), 2));
    }
  }
}
void Session::writeForwardingHandle(int direction, boost::system::error_code ec,
                                    std::size_t length) {
  if (ec) {
  } else {
      if(direction==1)
      {
          in_socket.async_receive(
        boost::asio::buffer(in_buf),
        std::bind(&Session::readForwardingHandle, shared_from_this(), 1));
      }
      else if(direction==2)
      {
          out_socket.async_receive(
        boost::asio::buffer(out_buf),
        std::bind(&Session::readForwardingHandle, shared_from_this(), 2));
      }
  }
}