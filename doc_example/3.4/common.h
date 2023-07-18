#pragma once
#include "hpda/hpda.h"
#include "ff/util/ntobject.h"
#include "ff/network.h"

define_nt(phone_number,long long int);
define_nt(longitude, float);
define_nt(latitude, float);
define_nt(timestamp, long long int);
define_nt(distance, float);
typedef ff::util::ntobject<longitude, latitude, timestamp> NTO_loc_info;
define_nt(loc_info, NTO_loc_info);

/*
typedef ff::util::ntobject<phone_number, NTO_loc_info> NTO_data_entry; // comile error on init
define_nt(nto_loc_info, ff::util::ntobject<longitude, latitude, timestamp> NTO_loc_info); // compile error on type check
*/

typedef ff::util::ntobject<phone_number, loc_info> NTO_data_entry;
define_nt(loc_history, std::vector<NTO_loc_info>); // should I use define_NT field, or NTobject
typedef ff::util::ntobject<phone_number, loc_history> NTO_grouped_data_entry;
typedef ff::util::ntobject<phone_number, distance> NTO_distance_entry;

define_nt(idx, long long int);
define_nt(data_entry, NTO_data_entry);
typedef ff::net::ntpackage<2, idx, data_entry> NTP_data_entry;
typedef ff::net::ntpackage<3> NTP_no_more_data_flag;

namespace ff { namespace net
{
    template <>
    class archive_helper<NTO_data_entry>
    {
    public:
        static uint32_t serialize(char *buf, const NTO_data_entry &d, size_t len)
        {
            uint offset = 0;
            auto f1 = d.get<phone_number>();
            auto f2 = d.get<loc_info>().get<longitude>();
            auto f3 = d.get<loc_info>().get<latitude>();
            auto f4 = d.get<loc_info>().get<timestamp>();
            
            memcpy(buf+offset, &f1, sizeof(f1));
            offset += sizeof(f1);
            memcpy(buf+offset, &f2, sizeof(f2));
            offset += sizeof(f2);
            memcpy(buf+offset, &f3, sizeof(f3));
            offset += sizeof(f3);
            memcpy(buf+offset, &f4, sizeof(f4));
            offset += sizeof(f4);
            return offset;
        }
        static uint32_t deserialize(const char *buf, NTO_data_entry &d, size_t len)
        {
            uint offset = 0;
            ff::util::internal::nt_traits<phone_number>::type f1;
            memcpy(&f1, buf+offset, sizeof(f1));
            offset += sizeof(f1);
            ff::util::internal::nt_traits<longitude>::type f2;
            memcpy(&f2, buf+offset, sizeof(f2));
            offset += sizeof(f2);
            ff::util::internal::nt_traits<latitude>::type f3;
            memcpy(&f3, buf+offset, sizeof(f3));
            offset += sizeof(f3);
            ff::util::internal::nt_traits<timestamp>::type f4;
            memcpy(&f4, buf+offset, sizeof(f4));
            offset += sizeof(f4);

            
            d.set<phone_number>(f1);
            NTO_loc_info tmp;
            tmp.set<longitude>(f2);
            tmp.set<latitude>(f3);
            tmp.set<timestamp>(f4);
            d.set<loc_info>(tmp);
            
            return sizeof(d);
        }

        static uint32_t length(NTO_data_entry &d) { 
            return \
                sizeof(d.get<phone_number>() ) + \
                sizeof(d.get<loc_info>().get<longitude>() ) + \
                sizeof(d.get<loc_info>().get<latitude>() ) + \
                sizeof(d.get<loc_info>().get<timestamp>());
        }
    };

    template <>
    class archive_helper<NTO_distance_entry>
    {
    public:
        static uint32_t serialize(char *buf, const NTO_distance_entry &d, size_t len)
        {
            uint offset = 0;
            auto f1 = d.get<phone_number>();
            auto f2 = d.get<distance>();
            
            memcpy(buf+offset, &f1, sizeof(f1));
            offset += sizeof(f1);
            memcpy(buf+offset, &f2, sizeof(f2));
            offset += sizeof(f2);
            return offset;
        }
        static uint32_t deserialize(const char *buf, NTO_distance_entry &d, size_t len)
        {
            uint offset = 0;
            ff::util::internal::nt_traits<phone_number>::type f1;
            memcpy(&f1, buf+offset, sizeof(f1));
            offset += sizeof(f1);
            ff::util::internal::nt_traits<distance>::type f2;
            memcpy(&f2, buf+offset, sizeof(f2));
            offset += sizeof(f2);
            
            d.set<phone_number>(f1);
            d.set<distance>(f2);
            
            return sizeof(d);
        }

        static uint32_t length(NTO_distance_entry &d) { 
            return \
                sizeof(d.get<phone_number>() ) + \
                sizeof(d.get<distance>() );
        }
    };
} }