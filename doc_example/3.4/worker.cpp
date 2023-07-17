#include "common.h"
#include <atomic>

class extract_client 
    : public hpda::extractor::internal::raw_data_impl<NTO_data_entry>,
      public ff::net::routine 
{
public:
    extract_client(int port)
    : ff::net::routine("worker.extract_client"),
      hpda::extractor::internal::raw_data_impl<NTO_data_entry>(),
      done_transfer(false),
      port(port)
    {    }

    virtual void initialize(ff::net::net_mode nm,
                            const std::vector<std::string> &args)
    {
        pkghub.tcp_to_recv_pkg<NTP_data_entry>(std::bind(&extract_client::on_recv_data, this,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2));
        pkghub.tcp_to_recv_pkg<NTP_no_data_entry>(std::bind(&extract_client::on_recv_no_data, this,
                                            std::placeholders::_1,
                                            std::placeholders::_2));

        pnn = new ff::net::net_nervure(nm);
        pnn->add_pkg_hub(pkghub);
        pnn->add_tcp_client("127.0.0.1", port);
        pnn->get_event_handler()->listen<ff::net::event::tcp_get_connection>(
            std::bind(&extract_client::on_conn_succ, this, std::placeholders::_1));
        pnn->get_event_handler()->listen<ff::net::event::tcp_lost_connection>(
            std::bind(&extract_client::on_conn_lost, this, std::placeholders::_1, pnn));
    }

    virtual void run() { 
        std::thread monitor_thrd(std::bind(&extract_client::done_transfer_and_close_conn, this));
        pnn->run();
        monitor_thrd.join();
    }

    bool process() override {
        return done_transfer && hpda::extractor::internal::raw_data_impl<NTO_data_entry>::process();
    }

protected:
    void send_request(ff::net::tcp_connection_base *server)
    {
        std::shared_ptr<client_request_msg> req_msg(new client_request_msg());
        req_msg->template set<idx>(m_data.size());
        std::cout << "requesting on index: " << m_data.size() << std::endl;
        server->send(req_msg);
    }

    void on_recv_data(std::shared_ptr<NTP_data_entry> nt_data,
                    ff::net::tcp_connection_base *server)
    {
        NTO_data_entry data;
        data = nt_data-> template get<data_entry>();
        add_data(data);
        std::cout << "got data: " << data.get<phone_number>() << std::endl;
        send_request(server);
    }

    void on_recv_no_data(std::shared_ptr<NTP_no_data_entry> msg,
                    ff::net::tcp_connection_base *server)
    {
        done_transfer.store(true);
        std::cout<< "cp" << done_transfer << std::endl;
    }

    void on_conn_succ(ff::net::tcp_connection_base *server)
    {
        ff::net::mout << "connect success, requesting data ..." << std::endl;
        send_request(server);
    }

    void on_conn_lost(ff::net::tcp_connection_base *pConn,
                    ff::net::net_nervure *pbn)
    {
        ff::net::mout << "Server lost!" << std::endl;
        pbn->stop();
    }

    void done_transfer_and_close_conn() {
        while(!done_transfer.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        // done transfer;
        pnn->stop();
    } 

protected:
    int port;
    std::atomic<bool> done_transfer;
    ff::net::typed_pkg_hub pkghub;
    ff::net::net_nervure *pnn;
};

int main(int argc, char *argv[])
{
    hpda::engine engine;


    ff::net::application app("worker");
    char *arguments[] = {
        "../bin/worker",
        "--routine",
        "worker.extract_client",
        nullptr
    };
    int my_argc = sizeof(arguments) / sizeof(arguments[0]) - 1;
    app.initialize(my_argc, arguments);
    extract_client c(8002);
    app.register_routine(&c);
    c.set_engine(&engine);

    hpda::output::internal::memory_output_impl<NTO_data_entry> checker( &c );
    
    app.run();
    
    engine.run();

    std::cout << checker.values().size() << std::endl;
    for (auto v : checker.values()) {
        std::cout << v.get<phone_number>() << "|";
        std::cout << v.get<loc_info>().get<longitude>() << "," \
            << v.get<loc_info>().get<latitude>() << "," \
            << v.get<loc_info>().get<timestamp>() << "," \
          << std::endl;
    }
}