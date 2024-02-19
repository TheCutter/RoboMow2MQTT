#include "RsRoboMessageCreator.h"

namespace Robo
{
    Robo::MiscStateMessageRequest RsRoboMessageCreator::GetRoboStateMessage() {
        const Robo::MiscStateMessageRequest request(11);
        return request;
    }
}