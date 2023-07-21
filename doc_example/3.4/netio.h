#pragma once
#include "hpda/hpda.h"
#include "ff/network.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

template<typename T>
class SafeQueue {
private:
    std::queue<T> q;
    mutable std::mutex mtx;
    std::condition_variable cond;
public:
    SafeQueue() {}

    void enqueue(T t) {
        std::lock_guard<std::mutex> lock(mtx);
        q.push(t);
        cond.notify_one();
    }

    T dequeue() {
        std::unique_lock<std::mutex> lock(mtx);
        cond.wait(lock, [this](){ return !q.empty(); });
        T val = q.front();
        q.pop();
        return val;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return q.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return q.size();
    }
};


typedef ff::net::ntpackage<8568956> NETIO_no_more_data_flag; // using a random-generated magic packageID

template <typename InputObjType>
class to_net 
    : public hpda::internal::processor_with_input<InputObjType>
{
define_nt(payload, InputObjType);
typedef ff::net::ntpackage<0, payload> NTP_data;

public:
    to_net(hpda::internal::processor_with_output<InputObjType> *upper_stream, 
            std::string ip="127.0.0.1", int port = 8000) 
        : hpda::internal::processor_with_input<InputObjType>(upper_stream),
          ip(ip), port(port),
          net_module_running(false), conn_setup(false)
    {   
        init_net_module();
        start_net_module();
    }

    ~to_net() {
        stop_net_module();
    }

    bool process() override 
    {
        while(!conn_setup.load()){
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        // @here conn is setup
        if (!hpda::internal::processor_with_input<InputObjType>::has_input_value()){
            return false;
        }
        std::cout << "sending data" << std::endl;
        send_data(server, hpda::internal::processor_with_input<InputObjType>::input_value());
        hpda::internal::processor_with_input<InputObjType>::consume_input_value();
        return true;
    }

private:
    virtual void init_net_module()
    {
        netn = new ff::net::net_nervure(ff::net::net_mode::real_net);
        netn->add_pkg_hub(pkghub);
        netn->add_tcp_client(ip, port);
        netn->get_event_handler()->listen<ff::net::event::tcp_get_connection>(
            std::bind(&to_net::on_conn_succ, this, std::placeholders::_1));
        netn->get_event_handler()->listen<ff::net::event::tcp_lost_connection>(
            std::bind(&to_net::on_conn_lost, this, std::placeholders::_1));
    }

    void start_net_module() {
        if(!net_module_running){
            net_module_thrd = new std::thread([&](){
                std::cout << "to_net.net_module: client starting" << std::endl;
                netn->run();
            });
           net_module_running = true;
        } else {
            std::cerr << "to_net.net_module: client already started" << std::endl;
        }
    }

    void stop_net_module() {
        send_end_flag(server);
        net_module_thrd->join(); // wait for server close connection after receiving the end flag
        delete net_module_thrd;
        delete netn;
        std::cout << "to_net.net_module: client stopping" << std::endl;
    }

    void send_data(ff::net::tcp_connection_base *server, InputObjType data) {
        std::shared_ptr<NTP_data> pkg = std::make_shared<NTP_data>();
        pkg->template set<payload>(data);
        server->send(pkg);
        std::cout << "sent one" << std::endl;
    }

    void send_end_flag(ff::net::tcp_connection_base* server) {
        std::shared_ptr<NETIO_no_more_data_flag> pkg(new NETIO_no_more_data_flag());
        server->send(pkg);
        std::cout<< "sent end flag" << std::endl;
    }

    void on_conn_succ(ff::net::tcp_connection_base *server)
    {
        ff::net::mout << "to_net.net_module: connect success" << std::endl;
        this->server = server;
        conn_setup.store(true);
    }

    void on_conn_lost(ff::net::tcp_connection_base *server)
    {
        ff::net::mout << "to_net.net_module: connection closed" << std::endl;
    }

protected:
    std::string ip;
    int port;
    ff::net::typed_pkg_hub pkghub;
    ff::net::net_nervure *netn;
    ff::net::tcp_connection_base *server;
    std::thread* net_module_thrd;

    bool net_module_running = true;
    std::atomic<bool> conn_setup;
};

template <typename OutputObjType>
class from_net 
    : public hpda::internal::processor_with_output<OutputObjType>
{
define_nt(payload, OutputObjType);
typedef ff::net::ntpackage<0, payload> NTP_data;

public:
    from_net(std::string ip="127.0.0.1", int port = 8000)
    : hpda::internal::processor_with_output<OutputObjType>(),
      ip(ip), port(port),
      net_module_running(false), conn_setup(false), got_end_flag(false)
    {
        init_net_module();
        start_net_module();
    }

    ~from_net() {
        stop_net_module();
    }

    bool process() override {
        while (conn_setup.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        while(!got_end_flag.load() && queue.empty()){
            ;
        }
        if(got_end_flag.load() && net_module_running){
            stop_net_module();
        }
        if (queue.empty()) {
            return false;
        }
        theObj = queue.dequeue();
        return true;
    }

    OutputObjType output_value() override
    {
        return theObj;
    }
protected:
    virtual void init_net_module()
    {
        pkghub.tcp_to_recv_pkg<NTP_data>(std::bind(&from_net::on_recv_data, this,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2));
        pkghub.tcp_to_recv_pkg<NETIO_no_more_data_flag>(std::bind(&from_net::on_recv_end_flag, this,
                                            std::placeholders::_1,
                                            std::placeholders::_2));

        netn = new ff::net::net_nervure(ff::net::net_mode::real_net);
        netn->add_pkg_hub(pkghub);
        netn->add_tcp_server(ip, port);
        netn->get_event_handler()->listen<ff::net::event::tcp_get_connection>(
            std::bind(&from_net::on_conn_succ, this, std::placeholders::_1));
        netn->get_event_handler()->listen<ff::net::event::tcp_lost_connection>(
            std::bind(&from_net::on_conn_lost, this, std::placeholders::_1, netn));
    }

    void start_net_module() {
        if(!net_module_running){
            net_module_thrd = new std::thread([&](){
                std::cout << "from_net.net_module: server starting" << std::endl;
                netn->run();
            });

            net_module_running = true;
        }
    }

    void stop_net_module() {
        if(net_module_running){
            netn->stop();
            std::cout << "from_net.net_module: server stopping" << std::endl;
            net_module_thrd->join();
            delete net_module_thrd;
            delete netn;
            net_module_running = false;
        }
    }

    void on_recv_data(std::shared_ptr<NTP_data> nt_data,
                    ff::net::tcp_connection_base *client)
    {
        std::cout << "received one" << std::endl;
        OutputObjType data = nt_data-> template get<payload>();
        queue.enqueue(data);
    }

    void on_recv_end_flag(std::shared_ptr<NETIO_no_more_data_flag> msg,
                    ff::net::tcp_connection_base *client)
    {
        std::cout << "got end flag" << std::endl;
        got_end_flag.store(true);
    }

    void on_conn_succ(ff::net::tcp_connection_base *client)
    {
        ff::net::mout << "from_net.net_module: connect success, waiting for data ..." << std::endl;
        this->client = client;
    }

    void on_conn_lost(ff::net::tcp_connection_base *pConn,
                    ff::net::net_nervure *netn)
    {
        ff::net::mout << "from_net.net_module: connection closed" << std::endl;
    }

protected:
    std::string ip;
    int port;
    ff::net::typed_pkg_hub pkghub;
    ff::net::net_nervure *netn;
    ff::net::tcp_connection_base *client;
    std::thread* net_module_thrd;

    bool net_module_running = false;
    std::atomic<bool> conn_setup;
    std::atomic<bool> got_end_flag;

    SafeQueue<OutputObjType> queue;
    OutputObjType theObj;
};