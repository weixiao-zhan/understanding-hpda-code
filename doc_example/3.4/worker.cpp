#include "common.h"
#include <atomic>
#include <cmath>
#include <numeric>

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

class groupby_and_sort
    : public hpda::processor::internal::processor_base<NTO_data_entry, NTO_grouped_data_entry>
{
public:
    groupby_and_sort(hpda::internal::processor_with_output<NTO_data_entry> *upper_stream)
        : hpda::processor::internal::processor_base<NTO_data_entry, NTO_grouped_data_entry>(upper_stream)
    {   }

    bool process() override {
        if(has_input_value()){
            input_data.push_back(input_value());
            consume_input_value();
            return false;
        } else {
            if (!grouped_and_sorted) {
                do_groupby_and_sort();
            }
            if (output_data.empty()) {
            return false;
            }
            output_idx++;
            if (output_idx >= static_cast<int>(output_data.size())) {
                return false;
            }
            return true;
        }
    }

    NTO_grouped_data_entry output_value() override {
        return output_data[output_idx];
    } 

private:

    void do_groupby_and_sort() {
        std::unordered_map<ff::util::internal::nt_traits<phone_number>::type, 
                std::vector<NTO_loc_info>> groupMap;
        for (auto& entry: input_data) {
            auto key = entry.get<phone_number>();
            if (groupMap.count(key) > 0) {
                groupMap[key].push_back(entry.get<loc_info>());
            } else {
                groupMap[key] = std::vector<NTO_loc_info>{entry.get<loc_info>()};
            }
        }

        for (auto entry : groupMap) {
            // sort the grouped loc_info entry
            auto sorting_v = entry.second;
            quicksort(sorting_v);

            // assemble into ntoject
            NTO_grouped_data_entry e;
            e.set<::phone_number>(entry.first);
            e.set<::loc_history>(sorting_v);
            output_data.push_back(e);
        }
        grouped_and_sorted = true;
    }

    int partition(std::vector<NTO_loc_info>& arr, int low, int high) {
        NTO_loc_info pivot = arr[high];
        int i = low - 1;

        for (int j = low; j <= high - 1; j++) {
            if (arr[j].get<timestamp>() <= pivot.get<timestamp>()) {
                i++;
                NTO_loc_info tmp = arr[i].make_copy();
                arr[i] = arr[j];
                arr[j] = tmp;
            }
        }

        NTO_loc_info tmp = arr[i + 1].make_copy();
        arr[i + 1] = arr[high];
        arr[high] = tmp;
        return i + 1;
    }

    void quicksort(std::vector<NTO_loc_info>& arr, int low, int high) {
        if (low < high) {
            int pi = partition(arr, low, high);
            quicksort(arr, low, pi - 1);
            quicksort(arr, pi + 1, high);
        }
    }

    void quicksort(std::vector<NTO_loc_info>& arr) {
        quicksort(arr, 0, arr.size() - 1);
    }

    std::vector<NTO_data_entry> input_data;
    std::vector<NTO_grouped_data_entry> output_data;
    bool grouped_and_sorted = false;
    uint output_idx = 0;
};

class cal_distance
    : public hpda::processor::internal::processor_base<NTO_grouped_data_entry, NTO_distance_entry>
{
public:
    cal_distance (hpda::internal::processor_with_output<NTO_grouped_data_entry> *upper_stream)
        : hpda::processor::internal::processor_base<NTO_grouped_data_entry, NTO_distance_entry>(upper_stream)
    {   }

    bool process() override
    {
        if (!has_input_value())
        {
            return false;
        }
        tmp.set<phone_number>(input_value().get<phone_number>());
        tmp.set<distance>(calculateDistance(input_value().get<loc_history>()));
        consume_input_value();
        return true;
    }

    NTO_distance_entry output_value() override {
        return tmp;
    }

private:
    NTO_distance_entry tmp;

    // Function to convert degrees to radians
    static double toRadians(double degrees) {
        return degrees * M_PI / 180.0;
    }

    // Function to calculate the distance between two points given their latitude and longitude
    static double calculateDistance(NTO_loc_info loc1, NTO_loc_info loc2) {
        double earthRadiusKm = 6371.0; // Radius of the Earth in kilometers

        // Convert latitude and longitude to radians
        double lon1Rad = toRadians(loc1.get<longitude>());
        double lat1Rad = toRadians(loc1.get<latitude>());
        double lon2Rad = toRadians(loc2.get<longitude>());
        double lat2Rad = toRadians(loc2.get<latitude>());

        // Calculate the differences between the latitudes and longitudes
        double latDiff = lat2Rad - lat1Rad;
        double lonDiff = lon2Rad - lon1Rad;

        // Calculate the distance using the Haversine formula
        double a = sin(latDiff / 2.0) * sin(latDiff / 2.0) +
                cos(lat1Rad) * cos(lat2Rad) *
                sin(lonDiff / 2.0) * sin(lonDiff / 2.0);
        double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
        double distance = earthRadiusKm * c;

        return distance;
    }

    double calculateDistance(std::vector<NTO_loc_info> v) {
        double dist = 0;
        if (v.size() > 1) {
            for(int i = 1; i < v.size(); i++) {
                dist += calculateDistance(v[i-1], v[i]);
            }
        }
        return dist;
    }
};

class max_n
    : public hpda::processor::internal::processor_base<NTO_distance_entry, NTO_distance_entry>
{
public:
    max_n(hpda::internal::processor_with_output<NTO_distance_entry> *upper_stream, uint size_limit)
        : hpda::processor::internal::processor_base<NTO_distance_entry, NTO_distance_entry>(upper_stream),
          size_limit(size_limit)
    {   }

    bool process() override {
        if(has_input_value()){
            insert(max_n_data, input_value());
            if(max_n_data.size() > size_limit) {
                max_n_data.pop_back();
            }

            consume_input_value();
            return false;
        }
        if(max_n_data.empty()) {
            return false;
        }
        output_idx++;
        if (output_idx >= max_n_data.size()) {
            return false;
        }
        return true;
    }

    NTO_distance_entry output_value() override {
        return max_n_data[output_idx];
    } 

private:
    void insert(std::vector<NTO_distance_entry>& l, NTO_distance_entry new_item) {
        l.push_back(new_item.make_copy()); // a place holder
        int i = l.size()-2;
        while(i>=0) {
            if(l[i].get<distance>() < new_item.get<distance>()){
                l[i+1] = l[i].make_copy();
            } else {
                break;
            }
            i--;
        }
        i++; //the place to actually insert
        if(i>=0){
            l[i] = new_item;
        }
        return;
    }

    static bool compare_by_distance(const NTO_distance_entry& a, const NTO_distance_entry& b) {
        return a.get<distance>() > b.get<distance>();    // descending order
    }

    std::vector<NTO_distance_entry> max_n_data;
    uint output_idx = -1;
    uint size_limit;
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
    extract_server es(8002);
    app.register_routine(&es);
    es.set_engine(&engine);

    groupby_and_sort gs(&es);
    cal_distance cd(&gs);
    max_n mn(&cd, 3);

    hpda::output::internal::memory_output_impl<NTO_distance_entry> checker( &mn );
    
    app.run();
    engine.run();

    std::cout << checker.values().size() << std::endl;
    for (auto v : checker.values()) {
        std::cout << v.get<phone_number>() << "|";
        std::cout << v.get<distance>() << std::endl;
        /*
        for (auto i : v.get<loc_history>()) {
            std::cout << "\t" << i.get<longitude>() << "," \
                << i.get<latitude>() << "," \
                << i.get<timestamp>() << "," \
                << std::endl;
        }
        */
        std::cout << std::endl;
    }
}
