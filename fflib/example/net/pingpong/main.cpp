/***********************************************
  The MIT License (MIT)

  Copyright (c) 2012 Athrun Arthur <athrunarthur@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
 *************************************************/
#include "ff/network.h"
#include <chrono>
#include <iostream>

define_nt(msg, std::string);
define_nt(uid, uint64_t, "uid");

typedef ff::net::ntpackage<110, msg, uid> ping_msg;
typedef ff::net::ntpackage<111, msg, uid> pong_msg;

class client : public ff::net::routine {
public:
  client() : ff::net::routine("client") {}

  virtual void initialize(ff::net::net_mode nm,
                          const std::vector<std::string> &args) {
    pkghub.tcp_to_recv_pkg<pong_msg>(std::bind(&client::onRecvPong, this,
                                               std::placeholders::_1,
                                               std::placeholders::_2));

    pnn = new ff::net::net_nervure(nm);
    pnn->add_pkg_hub(pkghub);
    pnn->add_tcp_client("127.0.0.1", 6891);
    pnn->get_event_handler()->listen<ff::net::event::tcp_get_connection>(
        std::bind(&client::onConnSucc, this, std::placeholders::_1));
    pnn->get_event_handler()->listen<ff::net::event::tcp_lost_connection>(
        std::bind(&client::onLostConn, this, std::placeholders::_1, pnn));
  }

  virtual void run() { pnn->run(); }

protected:
  void sendPingMsg(ff::net::tcp_connection_base *pConn) {
    char *pContent = new char[16];
    const char *str = "ping world!";
    std::memcpy(pContent, str, std::strlen(str) + 1);
    std::shared_ptr<ping_msg> pMsg(new ping_msg());
    pMsg->template set<msg, uid>(std::string("ping world"), 1);

    pConn->send(pMsg);
    ff::net::mout << "service running..." << std::endl;
  }

  void onRecvPong(std::shared_ptr<pong_msg> pPong,
                  ff::net::tcp_connection_base *from) {
    // pong_msg &msg = *pPong.get();
    ff::net::mout << "got pong!" << pPong->template get<msg>() << ", "
                  << pPong->template get<uid>() << std::endl;
    sendPingMsg(from);
  }

  void onConnSucc(ff::net::tcp_connection_base *pConn) {
    ff::net::mout << "connect success" << std::endl;
    sendPingMsg(pConn);
  }
  void onLostConn(ff::net::tcp_connection_base *pConn,
                  ff::net::net_nervure *pbn) {
    ff::net::mout << "Server lost!" << std::endl;
    pbn->stop();
  }

protected:
  ff::net::typed_pkg_hub pkghub;
  ff::net::net_nervure *pnn;
};

class server : public ff::net::routine {
public:
  server() : ff::net::routine("server") {}

  virtual void initialize(ff::net::net_mode nm,
                          const std::vector<std::string> &args) {
    pkghub.tcp_to_recv_pkg<ping_msg>(std::bind(&server::onRecvPing, this,
                                               std::placeholders::_1,
                                               std::placeholders::_2));

    pnn = new ff::net::net_nervure(nm);
    pnn->add_pkg_hub(pkghub);
    pnn->add_tcp_server("127.0.0.1", 6891);
    pnn->get_event_handler()->listen<ff::net::event::tcp_lost_connection>(
        std::bind(&server::onLostTCPConnection, this, std::placeholders::_1));
  }

  virtual void run() {
    std::thread monitor_thrd(std::bind(&server::press_and_stop, this));
    pnn->run();

    monitor_thrd.join();
  }

protected:
  void onRecvPing(std::shared_ptr<ping_msg> pPing,
                  ff::net::tcp_connection_base *from) {

    // pPing->print();
    auto ping_uid = pPing->template get<uid>();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::shared_ptr<pong_msg> pkg(new pong_msg());
    pkg->template set<uid>(ping_uid + 1);
    pkg->template set<msg>(pPing->template get<msg>());
    from->send(pkg);
  }

  void onLostTCPConnection(ff::net::tcp_connection_base *pConn) {
    ff::net::mout << "lost connection!" << std::endl;
  }

  void press_and_stop() {
    ff::net::mout << "Press any key to quit..." << std::endl;
    std::cin.get();
    pnn->stop();
    ff::net::mout << "Stopping, please wait..." << std::endl;
  }

protected:
  ff::net::typed_pkg_hub pkghub;
  ff::net::net_nervure *pnn;
};

int main(int argc, char *argv[]) {
  ff::net::application app("pingpong");
  app.initialize(argc, argv);

  client c;
  server s;
  app.register_routine(&c);
  app.register_routine(&s);
  app.run();
  return 0;
}

