#include "RoboMessageCreator.h"

namespace Robo
{
	byte RoboMessageCreator::CreateChecksum(std::vector<byte>& data)
	{
		uint32_t temp = 0;
		for (const byte b : data) temp += b;
		return (byte)((~temp) & 0xFF);
	}

    template<typename TMessage>
    std::vector<byte> RoboMessageCreator::SerializeMessage(TMessage message)
    {
        std::vector<byte> buffer(sizeof(TMessage));
        bitsery::quickSerialization<OutputAdapter>(buffer, message);
        return buffer;
    }

    template std::vector<byte> RoboMessageCreator::SerializeMessage(MiscStateMessageRequest message);
    template std::vector<byte> RoboMessageCreator::SerializeMessage(AutoOperationMessage message);

	std::vector<byte> RoboMessageCreator::GetConfigRequestMessage()
	{
		std::vector<byte> buffer(sizeof(Robo::ConfigMessageRequest));
		const Robo::ConfigMessageRequest request;
		bitsery::quickSerialization<OutputAdapter>(buffer, request);
		return buffer;
	}

	ConfigMessageResponse RoboMessageCreator::GetConfigResponseMessage(byte* pData, size_t length)
	{
		Robo::ConfigMessageResponse response;
		bitsery::quickDeserialization<InputAdapter>({pData, length}, response);
		return response;
	}

    MiscStateMessageResponse RoboMessageCreator::GetMiscStateResponseMessage(byte* pData, size_t length)
    {
        Robo::MiscStateMessageResponse response;
        bitsery::quickDeserialization<InputAdapter>({pData, length}, response);
        return response;
    }

    /**
     * Parses a RobotState response directly from the raw buffer.
     * Byte layout (RobotDataMiscellaneousRs.java):
     *   [0]    0xAA  Header
     *   [1]    Length
     *   [2]    0x1E  Response marker
     *   [3]    0x16  msgId = 22
     *   [4-5]  Counter
     *   [6-7]  Type (0x00, 0x0B = RobotState)
     *   [8]    Status flags: bit0-1=chargeSource, bit5=MowMotorActive
     *   [9]    OperationalState: bits 0-2 = SystemMode
     *   [10]   Battery charge: bits 0-6 = %, bit7 = AntiTheftActive
     *   [last] Checksum
     */
    MowerState RoboMessageCreator::ParseStateResponse(byte* pData, size_t length)
    {
        MowerState state;

        if (length < 16)              return state;
        if (pData[0] != 0xAA)         return state;
        if (pData[2] != 0x1E)         return state;  // Response marker
        if (pData[3] != 0x16)         return state;  // msgId = RbleMiscellaneousRs
        if (pData[6] != 0x00 || pData[7] != 0x0B) return state;  // type = RobotState

        // Verify checksum
        uint32_t sum = 0;
        for (size_t i = 0; i < length - 1; i++) sum += pData[i];
        if ((byte)((~sum) & 0xFF) != pData[length - 1]) {
            Serial.println("[RX] Checksum error, packet discarded");
            return state;
        }

        state.charging = (pData[8] & 0x03) == 0;  // chargeSource != 0
        state.mowing   = (pData[8] & 0x20) != 0;  // bit5 = MowMotorActive
        state.battery  =  pData[10] & 0x7F;        // bits 0-6
        state.mode     =  pData[9]  & 0x07;        // bits 0-2 = SystemMode
        state.valid    = true;
        return state;
    }

    AutoOperationMessage RoboMessageCreator::GetAutoOperationMessage(uint8_t mode)
    {
        return AutoOperationMessage(mode);
    }
}
