#include "common.h"
#include "hpda/processor/transform/split.h"

// starting with a csv file reader
class cvs_extractor : public hpda::extractor::raw_data<phone_number, longitude, latitude, timestamp>
{
public:
    cvs_extractor(const std::string &filename)
        :raw_data_impl()
    {
        file.open(filename);
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open file");
        }
        read_file();
    }
    ~cvs_extractor()
    {
        file.close();
    }

private:
    std::ifstream file;

    void read_file()
    {
        std::string line;
        
        while (std::getline(file, line))
        {
            std::istringstream s(line);

            std::vector<std::string> fields;
            std::string field;
            while (getline(s, field, ','))
            {
                fields.push_back(field);
            }


            typedef ff::util::ntobject<phone_number, longitude, latitude, timestamp> data_entry; // local use
            data_entry one_entry;
            one_entry.set<phone_number>(std::stoll(fields[0]));
            one_entry.set<longitude>(std::stof(fields[1]));
            one_entry.set<latitude>(std::stof(fields[2]));
            one_entry.set<timestamp>(std::stoll(fields[3]));

            m_data.push_back(one_entry);
        }
    }
};

class hash_spliter : public hpda::processor::split<phone_number, longitude, latitude, timestamp>
{
public:
    hash_spliter(hpda::processor_with_output<phone_number, longitude, latitude, timestamp> *upper_stream)
        : hpda::processor::split<phone_number, longitude, latitude, timestamp>(upper_stream)
    {    }

    bool process() override
    {
        if (m_streams.empty() || !base::has_input_value() )
        {
            return false;
        }
        auto t = base::input_value();
        size_t hashValue = hashFunc(t.get<phone_number>());

        m_streams[hashValue%m_streams.size()]->add_data(t.make_copy());
        consume_input_value();
        return true;
    }

protected:
    std::hash<int> hashFunc;
};


//class output_server: 
class output_server 
    : public hpda::output::memory_output<phone_number, longitude, latitude, timestamp>, 
      public ff::net::routine
{
public:
    output_server(hpda::processor_with_output<phone_number, longitude, latitude, timestamp> *upper_stream, int port) 
        : hpda::output::memory_output<phone_number, longitude, latitude, timestamp>(upper_stream),
          ff::net::routine("spliter.output_server"),
          port(port)
    {   }


    virtual void initialize(ff::net::net_mode nm, const std::vector<std::string> &args)
    {
        pkghub.tcp_to_recv_pkg<client_request_msg>(std::bind(&output_server::on_recv_client_request, this,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2));

        pnn = new ff::net::net_nervure(nm);
        pnn->add_pkg_hub(pkghub);
        pnn->add_tcp_server("127.0.0.1", port);
        pnn->get_event_handler()->listen<ff::net::event::tcp_lost_connection>(
            std::bind(&output_server::on_lost_TCP_connection, this, std::placeholders::_1));
    }

    virtual void run()
    {
        std::thread monitor_thrd(std::bind(&output_server::press_and_stop, this));
        pnn->run();
        monitor_thrd.join();
    }
private:
    void on_recv_client_request(std::shared_ptr<client_request_msg> msg,
                    ff::net::tcp_connection_base *from) {

        auto i = msg->template get<idx>();
        if (i < m_data.size()) {
            std::shared_ptr<nt_data_entry> pkg=std::make_shared<nt_data_entry>();
            (*pkg) = m_data[i];
            std::cout<< "sent data[" << i << "]" << std::endl;
        } else {
            std::shared_ptr<nt_no_data_entry> pkg(new nt_no_data_entry());
            from->send(pkg);
            std::cout<< "sent no data" << i << "]" << std::endl;
        }
    }

    void on_lost_TCP_connection(ff::net::tcp_connection_base *pConn)
    {
        ff::net::mout << "lost connection!" << std::endl;
    }

    void press_and_stop()
    {
        ff::net::mout << "Press any key to quit..." << std::endl;
        std::cin.get();
        pnn->stop();
        ff::net::mout << "Stopping, please wait..." << std::endl;
    }
protected:
    int port;
    ff::net::typed_pkg_hub pkghub;
    ff::net::net_nervure *pnn;
};


int main(int argc, char *argv[])
{
    hpda::engine engine;

    cvs_extractor ce("../doc_example/3.4/data/phone_geo_time.csv");
    ce.set_engine(&engine);
/*
    hpda::output::internal::memory_output_impl<data_entry> checker( &ce );
    engine.run();
    std::cout << checker.values().size() << std::endl;
    for (auto v : checker.values()) {
        std::cout << v.get<phone_number>() << "|";
        std::cout << v.get<timestamp>()  << std::endl;
    }
*/

    hash_spliter hs(&ce);
    hs.set_engine(&engine);

    engine.run();

/*
    hpda::output::memory_output<phone_number, longitude, latitude, timestamp> checker1( hs.new_split_stream() );
    hpda::output::memory_output<phone_number, longitude, latitude, timestamp> checker2( hs.new_split_stream() );
    std::cout << "size" << checker1.values().size() << std::endl;
    for (auto v : checker1.values()) {
        std::cout << v.get<phone_number>() << "|";
        std::cout << v.get<timestamp>()  << std::endl;
    }
    std::cout << "size" << checker2.values().size() << std::endl;
    for (auto v : checker2.values()) {
        std::cout << v.get<phone_number>() << "|";
        std::cout << v.get<timestamp>()  << std::endl ;
    }
*/    

    ff::net::application app("pingpong");
    app.initialize(argc, argv);
    output_server s(hs.new_split_stream(), 8001);
    app.register_routine(&s);
    app.run();
    return 0;
}