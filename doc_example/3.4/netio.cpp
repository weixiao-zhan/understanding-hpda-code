#include "hpda/hpda.h"
#include "ff/network.h"
#include "SafeQueue.cpp"

typedef ff::net::ntpackage<3> NTP_no_more_data_flag; // using a magic UID

//class to_net:
template <typename InputObjType>
class to_net 
    : public hpda::internal::processor_with_input<InputObjType>, 
      public ff::net::routine
{
define_nt(payload, InputObjType);
typedef ff::net::ntpackage<0, payload> NTP_data;

public:
    to_net(hpda::internal::processor_with_output<InputObjType> *upper_stream, 
            std::string ip="127.0.0.1", int port = 8000, char* routine_name="to_net") 
        : hpda::internal::processor_with_input<InputObjType>(upper_stream),
          ff::net::routine(routine_name), ip(ip), port(port), routine_name(routine_name),
          net_app_started(false), conn_setup(false), done_transfer(false)
    {   }

    ~to_net()
    {

        netapp_thrd->join();
        delete app;
    }

    virtual void initialize(ff::net::net_mode nm, const std::vector<std::string> &args)
    {
        pkghub.tcp_to_recv_pkg<NTP_no_more_data_flag>(std::bind(&to_net::on_recv_end_flag, this,
                                    std::placeholders::_1,
                                    std::placeholders::_2));
        netn = new ff::net::net_nervure(nm);
        netn->add_pkg_hub(pkghub);
        netn->add_tcp_client(ip, port);
        netn->get_event_handler()->listen<ff::net::event::tcp_get_connection>(
            std::bind(&to_net::on_conn_succ, this, std::placeholders::_1));
        netn->get_event_handler()->listen<ff::net::event::tcp_lost_connection>(
            std::bind(&to_net::on_conn_lost, this, std::placeholders::_1));
    }

    virtual void run()
    {
        std::cout << "to_net.client: starting" << std::endl;
        netn->run();
    }

    bool process() override 
    {
        if (!net_app_started) {
            netapp_thrd = new std::thread(std::bind(&to_net::do_net_app_initialization, this));
        }
        while(!conn_setup.load()){
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (!hpda::internal::processor_with_input<InputObjType>::has_input_value()){
            return false;
        }
        std::cout << "sending data" << std::endl;
        // @here conn is setup
        send_data(server, hpda::internal::processor_with_input<InputObjType>::input_value());
        hpda::internal::processor_with_input<InputObjType>::consume_input_value();
        return true;
    }

    void hpda_engine_complete() { // to be manually called after engine.run complete.
        send_end_flag(server);
    }

private:
    void do_net_app_initialization() {
        app = new ff::net::application("to_net");
        char *arguments[] = {
            "binary place holder",
            "--routine",
            routine_name,
            nullptr
        };
        int my_argc = sizeof(arguments) / sizeof(arguments[0]) - 1;
        app->initialize(my_argc, arguments);
        app->register_routine(this);
        net_app_started = true;
        app->run();
    }

    void send_data(ff::net::tcp_connection_base *server, InputObjType data) {
        std::shared_ptr<NTP_data> pkg = std::make_shared<NTP_data>();
        pkg->template set<payload>(data);
        server->send(pkg);
        std::cout << "sent one" << std::endl;
    }

    void send_end_flag(ff::net::tcp_connection_base* server) {
        std::shared_ptr<NTP_no_more_data_flag> pkg(new NTP_no_more_data_flag());
        server->send(pkg);
        std::cout<< "sent end flag" << std::endl;
    }

    void on_recv_end_flag(std::shared_ptr<NTP_no_more_data_flag> msg,
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
    char* routine_name;
    ff::net::application* app;
    ff::net::typed_pkg_hub pkghub;
    ff::net::net_nervure *netn;
    ff::net::tcp_connection_base *server;
    std::thread* netapp_thrd;

    bool net_app_started = true;
    std::atomic<bool> conn_setup;
    std::atomic<bool> done_transfer;
};

template <typename OutputObjType>
class from_net 
    : public hpda::internal::processor_with_output<OutputObjType>,
      public ff::net::routine 
{
define_nt(payload, OutputObjType);
typedef ff::net::ntpackage<0, payload> NTP_data;

public:
    from_net(std::string ip="127.0.0.1", int port = 8000, char* routine_name="from_net")
    : hpda::internal::processor_with_output<OutputObjType>(),
      ff::net::routine(routine_name), ip(ip), port(port), routine_name(routine_name),
      net_app_started(false), conn_setup(false), done_transfer(false)
    {    }

    ~from_net()
    {
        netapp_thrd->join();
        delete app;
    }

    virtual void initialize(ff::net::net_mode nm,
                            const std::vector<std::string> &args)
    {
        pkghub.tcp_to_recv_pkg<NTP_data>(std::bind(&from_net::on_recv_data, this,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2));
        pkghub.tcp_to_recv_pkg<NTP_no_more_data_flag>(std::bind(&from_net::on_recv_end_flag, this,
                                            std::placeholders::_1,
                                            std::placeholders::_2));

        netn = new ff::net::net_nervure(nm);
        netn->add_pkg_hub(pkghub);
        netn->add_tcp_server(ip, port);
        netn->get_event_handler()->listen<ff::net::event::tcp_get_connection>(
            std::bind(&from_net::on_conn_succ, this, std::placeholders::_1));
        netn->get_event_handler()->listen<ff::net::event::tcp_lost_connection>(
            std::bind(&from_net::on_conn_lost, this, std::placeholders::_1, netn));
    }

    virtual void run() { 
        std::thread monitor_thrd(std::bind(&from_net::done_transfer_and_close_conn, this));
        std::cout << "from_net.server: starting" << std::endl;
        netn->run();
        monitor_thrd.join();
    }

    bool process() override {
        if(!net_app_started){
            start_net_app();
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

    void start_net_app() {
        if(!net_app_started){
           netapp_thrd = new std::thread(std::bind(&from_net::do_net_app_initialization, this));
        } else {
            std::cerr << "net_app already started" << std::endl;
        }
    }

protected:
    void do_net_app_initialization() {
        app = new ff::net::application("from_net");
        char *arguments[] = {
            "binary place holder",
            "--routine",
            routine_name,
            nullptr
        };
        int my_argc = sizeof(arguments) / sizeof(arguments[0]) - 1;
        app->initialize(my_argc, arguments);
        app->register_routine(this);
        net_app_started = true;
        app->run();
    }

    void on_recv_data(std::shared_ptr<NTP_data> nt_data,
                    ff::net::tcp_connection_base *client)
    {
        std::cout << "receviced one" << std::endl;
        OutputObjType data = nt_data-> template get<payload>();
        queue.enqueue(data);
    }

    void on_recv_end_flag(std::shared_ptr<NTP_no_more_data_flag> msg,
                    ff::net::tcp_connection_base *client)
    {
        std::cout << "got end flag" << std::endl;
        done_transfer.store(true);
        std::shared_ptr<NTP_no_more_data_flag> pkg(new NTP_no_more_data_flag());
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
        std::cout << "monitor_thrd: on transfer done, stopped net_app" << std::endl;
    } 

protected:
    std::string ip;
    int port;
    char* routine_name;
    ff::net::application* app;
    ff::net::typed_pkg_hub pkghub;
    ff::net::net_nervure *netn;
    std::thread* netapp_thrd;

    bool net_app_started = true;
    std::atomic<bool> conn_setup;
    std::atomic<bool> done_transfer;

    SafeQueue<OutputObjType> queue;
    OutputObjType theObj;
};