//g++ -I/opt/ypc/include/ nt_set.cpp

#include "ff/util/ntobject.h"
#include "hpda/hpda.h"
#include <iostream>

define_nt(email, std::string, "email");
define_nt(uid, uint64_t, "ui");
define_nt(uname, std::string, "uname");
define_nt(kname, std::string, "kname");

int main()
{
    typedef ff::util::ntobject<email, uid, uname> myobj_t;
    myobj_t obj;
    obj.set<uname>("xuepeng");
    obj.set<uid>(122);
    obj.set<email>("xp@example.com");
    //obj.set<kname>("xp2@example.com"); // will faile
    std::cout << "uname is " << obj.get<uname>() << std::endl;
}
