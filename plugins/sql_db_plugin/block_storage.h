/**
 *  @file
 *  @copyright defined in bes/LICENSE.txt
 */

#pragma once

#include "consumer_core.h"

#include <besio/chain/block_state.hpp>

namespace besio {

class block_storage : public consumer_core<chain::block_state_ptr>
{
public:
    void consume(const std::vector<chain::block_state_ptr>& blocks) override;
};

} // namespace
