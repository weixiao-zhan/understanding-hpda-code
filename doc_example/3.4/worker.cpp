#include "common.h"
#include <atomic>

class extract_server 
    : public hpda::extractor::internal::raw_data_impl<NTO_data_entry>,
      public ff::net::routine 
{
public:
    extract_server(int port)
    : ff::net::routine("worker.extract_server"),
      hpda::extractor::internal::raw_data_impl<NTO_data_entry>(),
      done_transfer(false),
      port(port)
    {    }

    virtual void initialize(ff::net::net_mode nm,
                            const std::vector<std::string> &args)
    {
        pkghub.tcp_to_recv_pkg<NTP_data_entry>(std::bind(&extract_server::on_recv_data, this,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2));
        pkghub.tcp_to_recv_pkg<NTP_no_more_data_flag>(std::bind(&extract_server::on_recv_no_data, this,
                                            std::placeholders::_1,
                                            std::placeholders::_2));

        netn = new ff::net::net_nervure(nm);
        netn->add_pkg_hub(pkghub);
        netn->add_tcp_server("127.0.0.1", port);
        netn->get_event_handler()->listen<ff::net::event::tcp_get_connection>(
            std::bind(&extract_server::on_conn_succ, this, std::placeholders::_1));
        netn->get_event_handler()->listen<ff::net::event::tcp_lost_connection>(
            std::bind(&extract_server::on_conn_lost, this, std::placeholders::_1, netn));
    }

    virtual void run() { 
        std::thread monitor_thrd(std::bind(&extract_server::done_transfer_and_close_conn, this));
        netn->run();
        monitor_thrd.join();
    }

    bool process() override {
        return done_transfer && hpda::extractor::internal::raw_data_impl<NTO_data_entry>::process();
    }

protected:
    void on_recv_data(std::shared_ptr<NTP_data_entry> nt_data,
                    ff::net::tcp_connection_base *client)
    {
        NTO_data_entry data;
        data = nt_data-> template get<data_entry>();
        add_data(data);
        std::cout << "got data: " << data.get<phone_number>() << std::endl;
    }

    void on_recv_no_data(std::shared_ptr<NTP_no_more_data_flag> msg,
                    ff::net::tcp_connection_base *client)
    {
        done_transfer.store(true);
        std::cout << "got end flag" << std::endl;
        std::shared_ptr<NTP_no_more_data_flag> pkg(new NTP_no_more_data_flag());
        client->send(pkg);
        std::cout<< "sent end flag" << std::endl;
    }

    void on_conn_succ(ff::net::tcp_connection_base *server)
    {
        ff::net::mout << "connect success, waiting for data ..." << std::endl;
    }

    void on_conn_lost(ff::net::tcp_connection_base *pConn,
                    ff::net::net_nervure *netn)
    {
        ff::net::mout << "Client lost!" << std::endl;
        netn->stop();
    }

    void done_transfer_and_close_conn() {
        while(!done_transfer.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        // done transfer;
        netn->stop();
    } 

protected:
    int port;
    std::atomic<bool> done_transfer;
    ff::net::typed_pkg_hub pkghub;
    ff::net::net_nervure *netn;
};

int main(int argc, char *argv[])
{
    hpda::engine engine;


    ff::net::application app("worker");
    char *arguments[] = {
        "../bin/worker",
        "--routine",
        "worker.extract_server",
        nullptr
    };
    int my_argc = sizeof(arguments) / sizeof(arguments[0]) - 1;
    app.initialize(my_argc, arguments);
    extract_server c(8002);
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