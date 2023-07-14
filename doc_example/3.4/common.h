#pragma once
#include "hpda/hpda.h"
#include "ff/util/ntobject.h"
#include "ff/network.h"

define_nt(phone_number,long long int);
define_nt(longitude, float);
define_nt(latitude, float);
define_nt(timestamp, long long int);
//typedef ff::util::ntobject<phone_number, longitude, latitude, timestamp> data_entry;

define_nt(idx, long long int);
typedef ff::net::ntpackage<0, idx> client_request_msg;
typedef ff::net::ntpackage<2, phone_number, longitude, latitude, timestamp> nt_data_entry;
typedef ff::net::ntpackage<3> nt_no_data_entry;