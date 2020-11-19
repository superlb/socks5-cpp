#include "server.h"

void Server::run(unsigned short port) {
  constexpr int threads = 10;
  boost::asio::io_context ioc{threads};
  std::make_shared<Listener>(
      ioc, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), port})
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
      std::bind(&Listener::on_accept, shared_from_this(), std::placeholders::_1,
                std::placeholders::_2));
}

void Listener::on_accept(boost::system::error_code ec,
                         boost::asio::ip::tcp::socket socket) {
  if (ec) {
  } else {
    std::make_shared<Session>(ioc, std::move(socket))->run();
  }
  do_accept();
}

void Session::run() {
  in_socket.async_receive(
      boost::asio::buffer(in_buf),
      std::bind(&Session::readAuthHandle, shared_from_this(),
                std::placeholders::_1, std::placeholders::_2));
}
void Session::readAuthHandle(boost::system::error_code ec, std::size_t length) {
  if (ec) {
  } else {
    if (length < 3 || in_buf[0] != 0x05) {
      return;
    }
    uint8_t methodsNum = in_buf[1];
    in_buf[1] = 0xFF;
    for (uint8_t i = 0; i < methodsNum; ++i)
      if (in_buf[2 + i] == 0x00) {
        in_buf[1] = 0x00;
        break;
      }
    in_socket.async_send(
        boost::asio::buffer(in_buf, 2),
        std::bind(&Session::writeAuthHandle, shared_from_this(),
                  std::placeholders::_1, std::placeholders::_2));
  }
}
void Session::writeAuthHandle(boost::system::error_code ec,
                              std::size_t length) {
  if (ec) {
  } else {
    if (in_buf[1] == 0xFF) return;
    in_socket.async_receive(
        boost::asio::buffer(in_buf),
        std::bind(&Session::readEstablishmentHandle, shared_from_this(),
                  std::placeholders::_1, std::placeholders::_2));
  }
}
void Session::readEstablishmentHandle(boost::system::error_code ec,
                                      std::size_t length) {
  if (ec) {
  } else {
    if (length < 5 || in_buf[0] != 0x05 || in_buf[1] != 0x01) {
      return;
    }
    uint8_t addtype = in_buf[3], addlen;
    if (addtype == 0x01) {
      addlen = 4;
      if (length != 6 + addlen) {
        return;
      }
      out_host = boost::asio::ip::address_v4(ntohl(*((uint32_t*)&in_buf[4])))
                     .to_string();
      out_port = std::to_string(ntohs(*((uint16_t*)&in_buf[8])));
    } else if (addtype == 0x03) {
      addlen = in_buf[4];
      if (length != 7 + addlen) {
        return;
      }
      out_port = std::to_string(ntohs(*((uint16_t*)&in_buf[5 + addlen])));
      auto tmp = std::string(&in_buf[5], addlen);
      boost::asio::ip::tcp::resolver tmpresolver(in_socket.get_executor());
      auto queryRes = *tmpresolver.resolve(tmp, out_port);
      out_host = queryRes.endpoint().address().to_string();
    } else if (addtype == 0x04) {
      // TODO
    }
    out_socket.connect(boost::asio::ip::tcp::endpoint{
        boost::asio::ip::make_address(out_host),
        static_cast<unsigned short>(std::stoi(out_port))});
    for (int i = 0; i < 10; ++i) in_buf[i] = 0;
    in_buf[0] = 0x05;
    in_buf[3] = 0x01;
    in_socket.async_send(
        boost::asio::buffer(in_buf, 10),
        std::bind(&Session::writeEstablishmentHandle, shared_from_this(),
                  std::placeholders::_1, std::placeholders::_2));
  }
}
void Session::writeEstablishmentHandle(boost::system::error_code ec,
                                       std::size_t length) {
  if (ec) {
  } else {
    in_socket.async_receive(
        boost::asio::buffer(in_buf),
        std::bind(&Session::readForwardingHandle, shared_from_this(), 1,
                  std::placeholders::_1, std::placeholders::_2));
    out_socket.async_receive(
        boost::asio::buffer(out_buf),
        std::bind(&Session::readForwardingHandle, shared_from_this(), 2,
                  std::placeholders::_1, std::placeholders::_2));
  }
}
void Session::readForwardingHandle(int direction, boost::system::error_code ec,
                                   std::size_t length) {
  if (ec) {
    in_socket.close();
    out_socket.close();
  } else {
    if (direction == 1) {
      // std::cout<<(in_host + ":" + in_port + " -> " + out_host + ":" +
      // out_port)<<std::endl;
      out_socket.async_send(
          boost::asio::buffer(in_buf, length),
          std::bind(&Session::writeForwardingHandle, shared_from_this(), 1,
                    std::placeholders::_1, std::placeholders::_2));
    } else if (direction == 2) {
      // std::cout<<(out_host + ":" + out_port + " -> " + in_host + ":" +
      // in_port)<<std::endl;
      in_socket.async_send(
          boost::asio::buffer(out_buf, length),
          std::bind(&Session::writeForwardingHandle, shared_from_this(), 2,
                    std::placeholders::_1, std::placeholders::_2));
    }
  }
}
void Session::writeForwardingHandle(int direction, boost::system::error_code ec,
                                    std::size_t length) {
  if (ec) {
    in_socket.close();
    out_socket.close();
  } else {
    if (direction == 1) {
      in_socket.async_receive(
          boost::asio::buffer(in_buf),
          std::bind(&Session::readForwardingHandle, shared_from_this(), 1,
                    std::placeholders::_1, std::placeholders::_2));
    } else if (direction == 2) {
      out_socket.async_receive(
          boost::asio::buffer(out_buf),
          std::bind(&Session::readForwardingHandle, shared_from_this(), 2,
                    std::placeholders::_1, std::placeholders::_2));
    }
  }
}