#include "protocol_registry.h"

void ProtocolRegistry::analyse(
    const LogicEventCapture& cap,
    std::vector<ProtocolCandidate>& out)
{
    (void)cap;
    (void)out;

    // Empty for now.
    // Later this will call UART/PWM analysers.
}