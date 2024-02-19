#include "WifiMqtt.h"
#include "Settings.h"

namespace Robo
{
    WifiMqtt::WifiMqtt()
            : lastWillTopic(nullptr)
    {
    }

    void WifiMqtt::setupMqtt()
    {
        IPAddress mqttServer;
        mqttServer.fromString(MQTTIP);
        this->mqttClient.setServer(mqttServer, MQTTPORT);
        this->mqttClient.setKeepAlive(60);
    }

    bool WifiMqtt::connectToWiFi() const
    {
        Serial.println("WiFi connecting...");

        WiFiClass::mode(WIFI_MODE_STA);
        WiFiClass::setHostname(HOSTNAME);
        WiFi.begin(WifiSSID, WifiPW);
        WiFi.setAutoReconnect(true);
        WiFi.persistent(true);

        int count = 0;
        while (WiFiClass::status() != WL_CONNECTED && count <= 100) {
            delay(100);
            count++;
        }

        if (WiFiClass::status() != WL_CONNECTED) {
            Serial.println("WiFi not connected!!!");
        }
        else {
            Serial.println("WiFi connected!");
        }

        return WiFiClass::status() == WL_CONNECTED;
    }

	void WifiMqtt::connectMqtt(const char* willTopic, const std::vector<const char*>& topicsList, MQTT_CALLBACK_SIGNATURE)
	{
		this->lastWillTopic = willTopic;
		this->topics = topicsList;
		this->mqttClient.setCallback(std::move(callback));

		this->reconnectMqtt();
	}

    void WifiMqtt::reconnectMqtt()
    {
        Serial.println("reconnectMqtt start");

        // Loop until we're reconnected
        while (WiFiClass::status() == WL_CONNECTED && !this->mqttClient.connected()) {
            String clientId = HOSTNAME;
            clientId += String(random(0xffff), HEX);

            Serial.println("Connecting to MQTT...");
            if (mqttClient.connect(clientId.c_str(), this->lastWillTopic, 0, true, "Offline"))
            {
                Serial.println("Connected to MQTT.");
                mqttClient.publish(this->lastWillTopic, "Online", true);

                Serial.println("Subscribe to topics...");
                for (unsigned int i = 0; i < this->topics.size(); i++)
                {
                    Serial.println(topics[i]);
                    mqttClient.subscribe(topics[i]);
                }
                Serial.println("Subscribed to topics on MQTT.");
            }
            else
            {
                delay(100);
            }
        }
    }

	void WifiMqtt::loop()
	{
		this->checkWifiConnection();
		this->checkMqttConnection();
	}

	void WifiMqtt::checkWifiConnection()
	{
		// checking for WiFi connection
		while (WiFiClass::status() != WL_CONNECTED) {
			WiFi.disconnect();
			WiFi.reconnect();

            Serial.print("Status: ");
            Serial.println(WiFiClass::status());
			Serial.println("wifi reconnect...");

			int count = 0;
			while (WiFiClass::status() != WL_CONNECTED && count <= 100) {
				delay(100);
				count++;
			}
		}
	}

	void WifiMqtt::checkMqttConnection()
	{
		if (!this->mqttClient.connected()) {
			this->reconnectMqtt();
		}
		else
		{
			this->mqttClient.loop();
		}
	}
}