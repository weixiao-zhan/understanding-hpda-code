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
          net_module_started(false), conn_setup(false), done_transfer(false)
    {   
        init_net_module();
    }

    ~to_net() {
        net_module_thrd->join();
    }

    void start_net_module() {
        if(!net_module_started){
           net_module_thrd = new std::thread(std::bind(&to_net::run_net_module, this));
           net_module_started = true;
        } else {
            std::cerr << "net_module already started" << std::endl;
        }
    }

    void end_net_module() { // to be manually called after engine.run complete.
        send_end_flag(server);
    }

    bool process() override 
    {
        if (!net_module_started) {
            start_net_module();
        }
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
        pkghub.tcp_to_recv_pkg<NETIO_no_more_data_flag>(std::bind(&to_net::on_recv_end_flag, this,
                                    std::placeholders::_1,
                                    std::placeholders::_2));
        netn = new ff::net::net_nervure(ff::net::net_mode::real_net);
        netn->add_pkg_hub(pkghub);
        netn->add_tcp_client(ip, port);
        netn->get_event_handler()->listen<ff::net::event::tcp_get_connection>(
            std::bind(&to_net::on_conn_succ, this, std::placeholders::_1));
        netn->get_event_handler()->listen<ff::net::event::tcp_lost_connection>(
            std::bind(&to_net::on_conn_lost, this, std::placeholders::_1));
    }
    
    void run_net_module() { 
        std::cout << "to_net.client: starting" << std::endl;
        netn->run();
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

    void on_recv_end_flag(std::shared_ptr<NETIO_no_more_data_flag> msg,
                    ff::net::tcp_connection_base *server)
    {
        std::cout << "got ack end flag" << std::endl;
        done_transfer.store(true);
    }

    void on_conn_succ(ff::net::tcp_connection_base *server)
    {
        ff::net::mout << "connect success" << std::endl;
        this->server = server;
        conn_setup.store(true);
    }

    void on_conn_lost(ff::net::tcp_connection_base *server)
    {
        ff::net::mout << "connection closed!" << std::endl;
    }

protected:
    std::string ip;
    int port;
    ff::net::typed_pkg_hub pkghub;
    ff::net::net_nervure *netn;
    ff::net::tcp_connection_base *server;
    std::thread* net_module_thrd;

    bool net_module_started = true;
    std::atomic<bool> conn_setup;
    std::atomic<bool> done_transfer;
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
      net_module_started(false), conn_setup(false), done_transfer(false)
    {
        init_net_module();
    }

    ~from_net() {
        net_module_thrd->join();
    }

    void start_net_module() {
        if(!net_module_started){
           net_module_thrd = new std::thread(std::bind(&from_net::run_net_module, this));
           net_module_started = true;
        } else {
            std::cerr << "net_module already started" << std::endl;
        }
    }

    bool process() override {
        if(!net_module_started){
            start_net_module();
        }
        while (conn_setup.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        while(!done_transfer && queue.empty()){
            ;
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

    void run_net_module() { 
        std::thread monitor_thrd(std::bind(&from_net::done_transfer_and_close_conn, this));
        std::cout << "from_net.server: starting" << std::endl;
        netn->run();
        monitor_thrd.join();
    }

    void on_recv_data(std::shared_ptr<NTP_data> nt_data,
                    ff::net::tcp_connection_base *client)
    {
        std::cout << "receviced one" << std::endl;
        OutputObjType data = nt_data-> template get<payload>();
        queue.enqueue(data);
    }

    void on_recv_end_flag(std::shared_ptr<NETIO_no_more_data_flag> msg,
                    ff::net::tcp_connection_base *client)
    {
        std::cout << "got end flag" << std::endl;
        done_transfer.store(true);
        std::shared_ptr<NETIO_no_more_data_flag> pkg(new NETIO_no_more_data_flag());
        client->send(pkg);
        std::cout<< "sent ack end flag" << std::endl;
    }

    void on_conn_succ(ff::net::tcp_connection_base *server)
    {
        ff::net::mout << "connect success, waiting for data ..." << std::endl;
    }

    void on_conn_lost(ff::net::tcp_connection_base *pConn,
                    ff::net::net_nervure *netn)
    {
        netn->stop();
        ff::net::mout << "Client lost, netn closed" << std::endl;
    }

    void done_transfer_and_close_conn() {
        while(!done_transfer.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        // done transfer;
        netn->stop();
        std::cout << "monitor_thrd: on transfer done, stopped net_module" << std::endl;
    }

protected:
    std::string ip;
    int port;
    ff::net::typed_pkg_hub pkghub;
    ff::net::net_nervure *netn;
    std::thread* net_module_thrd;

    bool net_module_started = false;
    std::atomic<bool> conn_setup;
    std::atomic<bool> done_transfer;

    SafeQueue<OutputObjType> queue;
    OutputObjType theObj;
};