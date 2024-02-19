#include "RoboMessageCreator.h"

namespace Robo
{
	byte RoboMessageCreator::CreateChecksum(std::vector<byte>& data)
	{
		byte temp = 0;
		for (const byte b : data)
		{
			temp += b;
		}

		return ~temp;
	}

    template<typename TMessage>
    std::vector<byte> RoboMessageCreator::SerializeMessage(TMessage message)
    {
        std::vector<byte> buffer(sizeof(TMessage));
        bitsery::quickSerialization<OutputAdapter>(buffer, message);
        return buffer;
    }

    template std::vector<byte> RoboMessageCreator::SerializeMessage(MiscStateMessageRequest message);

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
}