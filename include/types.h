#ifndef _TYPES_H_
#define _TYPES_H_

#include <cstdint>

using blocknum_t = uint64_t;
using workernum_t = uint32_t;
using timedelta_t = uint64_t;
using timestamp_t = uint64_t;

static constexpr blocknum_t BLOCK_INF = static_cast<blocknum_t>(-1);
static constexpr workernum_t WORKER_ALL = static_cast<workernum_t>(-1);
static constexpr timedelta_t TIME_NOW = static_cast<timedelta_t>(0);

#endif
