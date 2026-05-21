
#include <cstdio>
#include <cstdint>
#include <vector>
#include <iso15118/message/supported_app_protocol.hpp>
#include <iso15118/message/session_setup.hpp>
#include <iso15118/message/authorization.hpp>
#include <iso15118/message/service_discovery.hpp>
#include <iso15118/message/service_detail.hpp>
#include <iso15118/message/service_selection.hpp>
#include <iso15118/message/schedule_exchange.hpp>
#include <iso15118/message/ac_charge_parameter_discovery.hpp>
#include <iso15118/message/ac_charge_loop.hpp>
#include <iso15118/message/power_delivery.hpp>
#include <iso15118/message/session_stop.hpp>
#include <iso15118/message/variant.hpp>
#include "helper.hpp"

namespace dt = iso15118::message_20::datatypes;
using iso15118::serialize_helper;

static auto sid = std::array<uint8_t, 8>{0x3D, 0x4C, 0xBF, 0x93, 0x37, 0x4E, 0xD8, 0x9B};
static uint64_t ts = 1725456322;

static dt::RationalNumber rn(int16_t v, int8_t e = 0) { return {v, e}; }

int main() {
    // Just print a marker so we know it compiled
    printf("EXTRACTOR_READY
");
    return 0;
}
