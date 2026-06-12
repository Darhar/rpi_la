#pragma once

#include "../logic_events.h"
#include "protocol_candidate.h"

#include <vector>

class ProtocolRegistry {
public:
    void analyse(
        const LogicEventCapture& cap,
        std::vector<ProtocolCandidate>& out);

private:
    void analysePwm(...);
    void analyseUart(...);
};
