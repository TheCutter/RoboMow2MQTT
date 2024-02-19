#pragma once

#include <bitsery.h>
#include <ext/inheritance.h>
#include "MessageBase.h"

namespace Robo {
    enum RoboFamily
            : unsigned char {
        Unknown = 0,
        RS = 1,
        RC = 2,
        RX = 3
    };

    struct ConfigMessageRequest
            : Message {
        ConfigMessageRequest()
                : Message(15) {
        }
    };

    template<typename S>
    void serialize(S &s, ConfigMessageRequest &m) {
        s.ext(m, bitsery::ext::BaseClass<Message>{});
    }


    struct ConfigMessageResponse
            : Message {
        RoboFamily Family{};
        uint8_t SoftwareVersion{};
        uint16_t SoftwareRelease{};
        uint8_t MainboardVersion{};

        ConfigMessageResponse()
                : Message(15) {
        }
    };

    template<typename S>
    void serialize(S &s, ConfigMessageResponse &m) {
        s.ext(m, bitsery::ext::BaseClass<Message>{});
        s.value1b(m.Family);
        s.value1b(m.SoftwareVersion);
        s.value2b(m.SoftwareRelease);
        s.value1b(m.MainboardVersion);
    }
}