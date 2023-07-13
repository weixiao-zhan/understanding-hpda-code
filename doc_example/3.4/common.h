#pragma once

#include "ff/util/ntobject.h"
#include "hpda/hpda.h"
#include <iostream>

define_nt(phone_number,long long int);
define_nt(longitude, float);
define_nt(latitude, float);
define_nt(timestamp, long long int);
typedef ff::util::ntobject<phone_number, longitude, latitude, timestamp> data_entry;