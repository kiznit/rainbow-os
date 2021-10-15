/*
 * Trompeloeil C++ mocking framework
 *
 * Copyright Björn Fahller 2014,2017
 *
 *  Use, modification and distribution is subject to the
 *  Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 *
 * Project home: https://github.com/rollbear/trompeloeil
 */

//Multiple IN_SEQUENCE does not make sense. You can list several sequence
#include <trompeloeil.hpp>

struct MS
{
  MAKE_MOCK0(f, int());
};

int main()
{
  trompeloeil::sequence seq;
  MS obj;
  REQUIRE_DESTRUCTION(obj)
    .IN_SEQUENCE(seq)
    .IN_SEQUENCE(seq);
}
