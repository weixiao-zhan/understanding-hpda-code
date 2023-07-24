#pragma once
#include "hpda/hpda.h"
#include "ff/network.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

template <typename T>
class SafeQueue
{
private:
    std::queue<T> q;
    mutable std::mutex mtx;
    std::condition_variable cv;

public:
    SafeQueue() {}

    void enqueue(T t)
    {
        std::lock_guard<std::mutex> lock(mtx);
        q.push(t);
        if (q.size() == 1)
        {
            cv.notify_one();
        }
    }

    T dequeue()
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]()
                { return !q.empty(); });
        T val = q.front();
        q.pop();
        return val;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return q.empty();
    }

    size_t size() const
    {
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
           std::string ip = "127.0.0.1", int port = 8000)
        : hpda::internal::processor_with_input<InputObjType>(upper_stream),
          ip(ip), port(port),
          net_module_running(false), connected(false)
    {
        init_net_module();
        start_net_module();
    }

    ~to_net()
    {
        stop_net_module();
    }

    bool process() override
    {
        { // wait for connection set up
            std::unique_lock<std::mutex> lock(connected_mtx);
            connected_cv.wait(lock, [this]() { return connected == true; });
        } // release the connected_mtx
        // @here conn is setup
        if (!hpda::internal::processor_with_input<InputObjType>::has_input_value())
        {
            return false;
        }
#ifdef VERBOSE_OUTPUT
        ff::net::mout << "sending data" << std::endl;
#endif
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

    void start_net_module()
    {
        if (!net_module_running)
        {
            net_module_thrd = new std::thread([&]() {
                ff::net::mout << "to_net.net_module: client starting" << std::endl;
                netn->run(); });
            net_module_running = true;
        }
    }

    void stop_net_module()
    {
        if (net_module_running)
        {
            send_end_flag(server);
            net_module_thrd->join();
            delete net_module_thrd;
            delete netn;
            net_module_running = false;
            ff::net::mout << "to_net.net_module: client stopping" << std::endl;
        }
    }

    void send_data(ff::net::tcp_connection_base *server, InputObjType data)
    {
        std::shared_ptr<NTP_data> pkg = std::make_shared<NTP_data>();
        pkg->template set<payload>(data);
        server->send(pkg);
#ifdef VERBOSE_OUTPUT
        ff::net::mout << "sent one" << std::endl;
#endif
    }

    void send_end_flag(ff::net::tcp_connection_base *server)
    {
        std::shared_ptr<NETIO_no_more_data_flag> pkg(new NETIO_no_more_data_flag());
        server->send(pkg);
        ff::net::mout << "sent end flag" << std::endl;
    }

    void on_conn_succ(ff::net::tcp_connection_base *server)
    {
        ff::net::mout << "to_net.net_module: connect success" << std::endl;
        this->server = server;

        std::lock_guard<std::mutex> lock(connected_mtx);
        connected = true;
        connected_cv.notify_one();
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
    std::thread *net_module_thrd;

    bool net_module_running;
    bool connected;
    std::mutex connected_mtx;
    std::condition_variable connected_cv;
};

template <typename OutputObjType>
class from_net
    : public hpda::internal::processor_with_output<OutputObjType>
{
    define_nt(payload, OutputObjType);
    typedef ff::net::ntpackage<0, payload> NTP_data;

public:
    from_net(std::string ip = "127.0.0.1", int port = 8000)
        : hpda::internal::processor_with_output<OutputObjType>(),
          ip(ip), port(port),
          net_module_running(false), connected(false), got_end_flag(false)
    {
        init_net_module();
        start_net_module();
    }

    ~from_net()
    {
        stop_net_module();
    }

    bool process() override
    {
        { // wait for connection set up
            std::unique_lock<std::mutex> lock(connected_mtx);
            connected_cv.wait(lock, [this]() { return connected == true; });
        }// release the connected_mtx
        // @here conn is setup
        std::unique_lock<std::mutex> lock(got_end_flag_mtx);
        got_end_flag_cv.wait(lock, [this]() { return !queue.empty() || got_end_flag; });

        if (got_end_flag && net_module_running)
        {
            stop_net_module();
        }
        if (queue.empty() && got_end_flag)
        {
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

    void start_net_module()
    {
        if (!net_module_running)
        {
            net_module_thrd = new std::thread([&]() {
                ff::net::mout << "from_net.net_module: server starting" << std::endl;
                netn->run(); });
            net_module_running = true;
        }
    }

    void stop_net_module()
    {
        if (net_module_running)
        {
            netn->stop();
            net_module_thrd->join();
            delete net_module_thrd;
            delete netn;
            net_module_running = false;
            ff::net::mout << "from_net.net_module: server stopping" << std::endl;
        }
    }

    void on_recv_data(std::shared_ptr<NTP_data> nt_data,
                      ff::net::tcp_connection_base *client)
    {
#ifdef VERBOSE_OUTPUT
        ff::net::mout << "received one" << std::endl;
#endif
        OutputObjType data = nt_data->template get<payload>();
        queue.enqueue(data);
        got_end_flag_cv.notify_one();
    }

    void on_recv_end_flag(std::shared_ptr<NETIO_no_more_data_flag> msg,
                          ff::net::tcp_connection_base *client)
    {
        ff::net::mout << "got end flag" << std::endl;
        std::lock_guard<std::mutex> lock(got_end_flag_mtx);
        got_end_flag = true;
        got_end_flag_cv.notify_one();
    }

    void on_conn_succ(ff::net::tcp_connection_base *client)
    {
        ff::net::mout << "from_net.net_module: connect success, waiting for data ..." << std::endl;
        this->client = client;

        std::lock_guard<std::mutex> lock(connected_mtx);
        connected = true;
        connected_cv.notify_one();
    }

    void on_conn_lost(ff::net::tcp_connection_base *client, ff::net::net_nervure *netn)
    {
        ff::net::mout << "from_net.net_module: connection closed" << std::endl;
    }

protected:
    std::string ip;
    int port;
    ff::net::typed_pkg_hub pkghub;
    ff::net::net_nervure *netn;
    ff::net::tcp_connection_base *client;
    std::thread *net_module_thrd;

    bool net_module_running = false;
    bool connected;
    std::mutex connected_mtx;
    std::condition_variable connected_cv;
    bool got_end_flag;
    std::mutex got_end_flag_mtx;
    std::condition_variable got_end_flag_cv;

    SafeQueue<OutputObjType> queue;
    OutputObjType theObj;
};