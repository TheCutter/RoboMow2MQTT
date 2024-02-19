#pragma once

#include "RoboMessageCreator.h"

namespace Robo
{
	class RsRoboMessageCreator
		: public RoboMessageCreator
	{
		public:
            MiscStateMessageRequest GetRoboStateMessage() override;
    };
}