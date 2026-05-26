#pragma once

#include <Arduino.h>
#include <bitsery.h>
#include <adapter/buffer.h>
#include <traits/vector.h>

#include "RSMessages.h"
#include "ConfigMessages.h"

namespace Robo
{
    // ─── MowerState ──────────────────────────────────────────────────────────────
    // Populated by ParseStateResponse (RobotDataMiscellaneousRs.java)
    struct MowerState {
        uint8_t battery  = 0;      // 0-100 %  (byte10 bits 0-6)
        bool    charging = false;  // byte8 bits 0-1 != 0 → chargeSource active
        bool    mowing   = false;  // byte8 bit 5 = MowMotorActive
        uint8_t mode     = 0;      // byte9 bits 0-2 = SystemMode (0=Stop,1=Edge,2=Scan,3=Base)
        bool    valid    = false;
    };

    // ─── RobotState query (RbleMiscellaneousRs, msgId=0x16=22, type=0x0B=11) ───
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

    // ─── RobotState response ─────────────────────────────────────────────────────
    // Response: [AA, 10, 1E, 16, ctr_hi, ctr_lo, 00, 0B, byte8..byte14, chk]
    struct MiscStateMessageResponse
            : MessageWithComCount {

        uint16_t MiscType;
        uint8_t payload[7]{};  // byte8..byte14

        MiscStateMessageResponse()
            : MessageWithComCount(22) {
        }

        bool getBit(uint8_t byteIndex, uint8_t bitIndex) const {
            return (payload[byteIndex] >> bitIndex) & 0x01;
        }
    };

    template<typename S>
    void serialize(S &s, MiscStateMessageResponse &m) {
        s.ext(m, bitsery::ext::BaseClass<MessageWithComCount>{});
        s.value2b(m.MiscType);
        s.value1b(m.payload[0]);  // byte8:  Status flags
        s.value1b(m.payload[1]);  // byte9:  OperationalState
        s.value1b(m.payload[2]);  // byte10: Battery charge
        s.value1b(m.payload[3]);  // byte11: MinutesUntilNextDeparture hi
        s.value1b(m.payload[4]);  // byte12: MinutesUntilNextDeparture lo
        s.value1b(m.payload[5]);  // byte13: AutoOpDuration hi
        s.value1b(m.payload[6]);  // byte14: AutoOpDuration lo
    }

    // ─── Start/stop command (RbleAutomaticOperationRc, msgId=0x15=21) ────────────
    // Format: [AA, 09, 1F, 15, ctr_hi, ctr_lo, mode, 0xFF, chk]
    // LSOperationModes: Stop=0, Edge=1, Scan/Mow=2, Base=3
    struct AutoOperationMessage
            : MessageWithComCount {
        uint8_t Mode;
        uint8_t Zone;  // 0xFF = main zone / all zones

        explicit AutoOperationMessage(uint8_t mode, uint8_t zone = 0xFF)
            : MessageWithComCount(21) {
            Mode = mode;
            Zone = zone;
        }
    };

    template<typename S>
    void serialize(S &s, AutoOperationMessage &m) {
        s.ext(m, bitsery::ext::BaseClass<MessageWithComCount>{});
        s.value1b(m.Mode);
        s.value1b(m.Zone);
    }

    // ─── RoboMessageCreator ───────────────────────────────────────────────────────
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
            static MiscStateMessageResponse GetMiscStateResponseMessage(byte* pData, size_t length);

            // Parses a RobotState response directly from the raw buffer (without Bitsery overhead)
            static MowerState ParseStateResponse(byte* pData, size_t length);

            template<typename TMessage>
            std::vector<byte> SerializeMessage(TMessage message);

            virtual MiscStateMessageRequest GetRoboStateMessage() = 0;
            AutoOperationMessage GetAutoOperationMessage(uint8_t mode);
    };
}
