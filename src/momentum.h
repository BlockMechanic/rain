#ifndef SUPERCOIN_MOMENTUM_H
#define SUPERCOIN_MOMENTUM_H

#include <stdint.h>
#include <vector>

class uint256;

std::vector< std::pair<uint32_t,uint32_t> > momentum_search( uint256 midHash );
bool momentum_verify( uint256 midHash, uint32_t a, uint32_t b );
 
#endif
