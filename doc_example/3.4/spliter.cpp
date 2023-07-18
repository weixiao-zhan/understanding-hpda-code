#include "common.h"
#include "hpda/processor/transform/split.h"

// starting with a csv file reader
class cvs_extractor : public hpda::extractor::internal::raw_data_impl<NTO_data_entry>
{
public:
    cvs_extractor(const std::string &filename)
        :hpda::extractor::internal::raw_data_impl<NTO_data_entry>()
    {
        file.open(filename);
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open file");
        }
    }
    ~cvs_extractor()
    {
        file.close();
    }

    bool process() override {
        std::string line;
        if (std::getline(file, line))
        {
            std::istringstream s(line);

            std::vector<std::string> fields;
            std::string field;
            while (getline(s, field, ','))
            {
                fields.push_back(field);
            }

            NTO_loc_info one_loc_info;
            one_loc_info.set<longitude, latitude, timestamp>(
                std::stof(fields[1]), std::stof(fields[2]), std::stof(fields[3])); 

            NTO_data_entry one_entry;
            one_entry.set<phone_number>(std::stoll(fields[0]));
            one_entry.set<loc_info>(one_loc_info);
            hpda::extractor::internal::raw_data_impl<NTO_data_entry>::add_data(one_entry);
        }
        return hpda::extractor::internal::raw_data_impl<NTO_data_entry>::process();
    }

private:
    std::ifstream file;
};


class hash_spliter : public hpda::processor::internal::split_impl<NTO_data_entry>
{
public:
    hash_spliter(hpda::internal::processor_with_output<NTO_data_entry> *upper_stream)
        : hpda::processor::internal::split_impl<NTO_data_entry>(upper_stream)
    {    }

    bool process() override
    {
        if (m_streams.empty() || !base::has_input_value() )
        {
            return false;
        }
        NTO_data_entry t = base::input_value();
        size_t hashValue = hashFunc(t.get<phone_number>());
        m_streams[hashValue%m_streams.size()]->add_data(t.make_copy());
        consume_input_value();
        return true;
    }

protected:
    std::hash<int> hashFunc;
};

//class output_client: 
class output_client 
    : public hpda::output::internal::memory_output_impl<NTO_data_entry>, 
      public ff::net::routine
{
public:
    output_client(hpda::internal::processor_with_output<NTO_data_entry> *upper_stream, int port) 
        : hpda::output::internal::memory_output_impl<NTO_data_entry>(upper_stream),
          ff::net::routine("spliter.output_client"),
          port(port)
    {   }

    virtual void initialize(ff::net::net_mode nm, const std::vector<std::string> &args)
    {
        pkghub.tcp_to_recv_pkg<NTP_no_more_data_flag>(std::bind(&output_client::on_recv_no_data, this,
                                    std::placeholders::_1,
                                    std::placeholders::_2));
        netn = new ff::net::net_nervure(nm);
        netn->add_pkg_hub(pkghub);
        netn->add_tcp_client("127.0.0.1", port);
        netn->get_event_handler()->listen<ff::net::event::tcp_get_connection>(
            std::bind(&output_client::on_conn_succ, this, std::placeholders::_1));
        netn->get_event_handler()->listen<ff::net::event::tcp_lost_connection>(
            std::bind(&output_client::on_conn_lost, this, std::placeholders::_1));
    }

    virtual void run()
    {
        netn->run();
    }
private:
    void send_data(ff::net::tcp_connection_base *server) {
        for (NTO_data_entry data : m_data) {
            std::shared_ptr<NTP_data_entry> pkg = std::make_shared<NTP_data_entry>();
            (*pkg).set<data_entry>(data);
            server->send(pkg);
            std::cout<< "sent data [" << data.get<phone_number>() << "]" << std::endl;
        }
        
        std::shared_ptr<NTP_no_more_data_flag> pkg(new NTP_no_more_data_flag());
        server->send(pkg);
        std::cout<< "sent end flag" << std::endl;
    }

    void on_recv_no_data(std::shared_ptr<NTP_no_more_data_flag> msg,
                    ff::net::tcp_connection_base *server)
    {
        done_transfer.store(true);
    }

    void on_conn_succ(ff::net::tcp_connection_base *server)
    {
        ff::net::mout << "connect success, sending data ..." << std::endl;
        send_data(server);
    }

    void on_conn_lost(ff::net::tcp_connection_base *server)
    {
        ff::net::mout << "lost connection!" << std::endl;
    }

    void done_transfer_and_close_conn() {
        while(!done_transfer.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        // done transfer;
        netn->stop();
    } 


protected:
    std::atomic<bool> done_transfer;

    int port;
    ff::net::typed_pkg_hub pkghub;
    ff::net::net_nervure *netn;
};


int main(int argc, char *argv[])
{
    hpda::engine engine;

    cvs_extractor ce("../doc_example/3.4/data/phone_geo_time.csv");
    ce.set_engine(&engine);
    /*
    hpda::output::internal::memory_output_impl<NTO_data_entry> checker( &ce );
    engine.run();
    std::cout << checker.values().size() << std::endl;
    for (auto v : checker.values()) {
        std::cout << v.get<phone_number>() << std::endl;
    }
    */


    hash_spliter hs(&ce);
    hs.set_engine(&engine);

    /*
    hpda::output::internal::memory_output_impl<NTO_data_entry> checker1( hs.new_split_stream() );
    hpda::output::internal::memory_output_impl<NTO_data_entry> checker2( hs.new_split_stream() );
    engine.run();
    std::cout << "size" << checker1.values().size() << std::endl;
    for (auto v : checker1.values()) {
        std::cout << v.get<phone_number>() << "|";
        std::cout << v.get<loc_info>().get<timestamp>()  << std::endl;
    }
    std::cout << "size" << checker2.values().size() << std::endl;
    for (auto v : checker2.values()) {
        std::cout << v.get<phone_number>() << "|";
        std::cout << v.get<loc_info>().get<timestamp>()  << std::endl ;
    }
    */

    ff::net::application app("spliter");
    char *arguments[] = {
        "../bin/spliter",
        "--routine",
        "spliter.output_client",
        nullptr
    };
    int my_argc = sizeof(arguments) / sizeof(arguments[0]) - 1;
    app.initialize(my_argc, arguments);
    output_client s(hs.new_split_stream(), 8002);
    app.register_routine(&s);
    s.set_engine(&engine);

    engine.run();
    app.run();
    return 0; 
}