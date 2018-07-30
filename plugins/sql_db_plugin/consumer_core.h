/**
 *  @file
 *  @copyright defined in bes/LICENSE.txt
 */

#pragma once

#include <vector>

namespace besio {

template<typename T>
class consumer_core {
public:
    virtual ~consumer_core() {}
    virtual void consume(const std::vector<T>& elements) = 0;
};

} // namespace


