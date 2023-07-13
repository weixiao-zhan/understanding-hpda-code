/***********************************************
  The MIT License (MIT)

  Copyright (c) 2012 Athrun Arthur <athrunarthur@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
 *************************************************/
#include "ff/sql/table.h"
#include "ff/util/ntobject.h"

template <typename NT> struct my_struct {

  define_nt(email, std::string, "email");
  define_nt(uid, NT);
  define_nt(uname, std::string, "uname");

}; // namespace mn

int main(int argc, char *argv[]) {

  using mn = my_struct<int64_t>;
  using email = mn::email;
  using uid = mn::uid;
  using uname = mn::uname;

  typedef ff::util::ntobject<email, uid, uname> myobj_t;
  myobj_t obj;
  obj.set<uname>("xuepeng");
  obj.set<uid>(122);
  obj.set<email>("xp@example.com");

  typedef ff::util::ntarray<email, uid, uname> theobjects_t;
  typedef typename theobjects_t::row_type theobject_t;
  theobject_t t;
  t.set<email, uid, uname>(obj.get<email>(), obj.get<uid>(), obj.get<uname>());
  theobjects_t obs;

  obs.push_back(std::move(t));

  std::cout << obs.size() << std::endl;

  return 0;
}
