#pragma once

#include "trace_src.h"

#ifdef DEBUG

#define DO_ASSERTMSG(_msg, _exp) \
    ((!(_exp)) ? \
        ::LogMessage(EVENT_LEVEL_ERROR, __FUNCTION__, __LINE__, "Assert (%s): %s", #_exp, _msg), \
        assert(_exp), TRUE \
        : TRUE)

#else // !DEBUG

#define DO_ASSERTMSG(_msg, _exp)

#endif // DEBUG

#define DO_ASSERT(_exp)     DO_ASSERTMSG("Failed", _exp)
