#pragma once

#include <Arduino.h>
#include <bitsery.h>
#include <adapter/buffer.h>
#include <traits/vector.h>

#include "RSMessages.h"
#include "ConfigMessages.h"

namespace Robo
{
    struct MiscStateMessageRequest
            : MessageWithComCount {
        uint16_t MiscType;

        explicit MiscStateMessageRequest(uint16_t miscType)
            : MessageWithComCount(22) {
            MiscType = miscType;
        }
    };

    template<typename S>
    void serialize(S &s, MiscStateMessageRequest &m) {
        s.ext(m, bitsery::ext::BaseClass<MessageWithComCount>{});
        s.value2b(m.MiscType);
    }

    struct MiscStateMessageResponse
            : MessageWithComCount {

        uint16_t MiscType;
        uint8_t thirdValue : 1;

        MiscStateMessageResponse()
            : MessageWithComCount(22) {
        }
    };

    template<typename S>
    void serialize(S &s, MiscStateMessageResponse &m) {
        s.ext(m, bitsery::ext::BaseClass<MessageWithComCount>{});
        s.value2b(m.MiscType);
    }

	class RoboMessageCreator
	{
		 protected:
            struct InverseEndiannessConfig {
                static constexpr bitsery::EndiannessType Endianness = bitsery::EndiannessType::BigEndian;
                static constexpr bool CheckDataErrors = true;
                static constexpr bool CheckAdapterErrors = true;
            };

            using OutputAdapter = bitsery::OutputBufferAdapter<std::vector<byte>, InverseEndiannessConfig>;
            using InputAdapter = bitsery::InputBufferAdapter<byte*, InverseEndiannessConfig>;

		 public:
			static byte CreateChecksum(std::vector<byte>& data);
			static std::vector<byte> GetConfigRequestMessage();
			static ConfigMessageResponse GetConfigResponseMessage(byte* pData, size_t length);

            template<typename TMessage>
            std::vector<byte> SerializeMessage(TMessage message);

            virtual MiscStateMessageRequest GetRoboStateMessage() = 0;
    };
}