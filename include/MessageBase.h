#pragma once

#include <bitsery.h>
#include <ext/inheritance.h>

namespace Robo {
    struct Message {
        uint8_t Header, Length, Divider, MessageType;

        explicit Message(uint8_t messageType) {
            Header = 170;
            Length = 0;
            Divider = 31;
            MessageType = messageType;
        }
    };

    template<typename S>
    void serialize(S &s, Message &m) {
        s.value1b(m.Header);
        s.value1b(m.Length);
        s.value1b(m.Divider);
        s.value1b(m.MessageType);
    }


    struct MessageWithComCount
            : Message {
        uint16_t ComCount;

        explicit MessageWithComCount(uint8_t messageType)
                : Message(messageType) {
            this->ComCount = 0;
        }
    };

    template<typename S>
    void serialize(S &s, MessageWithComCount &mwc) {
        s.ext(mwc, bitsery::ext::BaseClass<Message>{});
        s.value2b(mwc.ComCount);
    }
}