#include "Robomow.h"

Robo::RoboBLE ble(HOSTNAME);
Robo::WifiMqtt wifiMqtt;

void mqttCallback(char* topic, byte* message, unsigned int length)
{
	Serial.printf("MQTT Message received on %s", topic);
	Serial.println();

	if (strcmp(topic, "robomow/action") == 0)
	{
		if (memcmp(message, "scan", length) == 0)
		{
			ble.Scan();
		}
        else if (memcmp(message, "connect", length) == 0)
        {
            ble.Connect();
        }
        else if (memcmp(message, "state", length) == 0)
        {
            auto data = ble.MessageCreator->GetRoboStateMessage();
            auto buffer = ble.MessageCreator->SerializeMessage(data);
            ble.SendMessage(buffer, [] (byte* pData, size_t length)
            {
                Serial.println(length);
            });
        }
		else if (memcmp(message, "reset", length) == 0)
		{
			ESP.restart();
		}
	}
	else if (strcmp(topic, "robomow/config") == 0)
	{
	}
}

void setup()
{
	Serial.begin(115200);

	if (!wifiMqtt.connectToWiFi()) return;
	wifiMqtt.setupMqtt();

	const std::vector<const char*> topics = { "robomow/action", "robomow/config" };
	wifiMqtt.connectMqtt("robomow/status", topics, mqttCallback);

	ble.Setup();
}

unsigned long LastMillis = 0;
void loop()
{
	wifiMqtt.loop();
	ble.Loop();

	if (millis() - LastMillis > 10000)
	{
		wifiMqtt.mqttClient.publish("robomow/memory/heap", String(ESP.getFreeHeap()).c_str());
		LastMillis = millis();
	}
}