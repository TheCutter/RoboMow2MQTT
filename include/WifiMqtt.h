#pragma once

#include <WiFi.h>
#include <PubSubClient.h>
#include <vector>

namespace Robo
{
    class WifiMqtt
    {
        const char* lastWillTopic;
        std::vector<const char*> topics;
        WiFiClient wifiClient;

        void reconnectMqtt();
        void checkWifiConnection();
        void checkMqttConnection();

    public:
        WifiMqtt();
        PubSubClient mqttClient = PubSubClient(wifiClient);

        void setupMqtt();
        void loop();
        bool connectToWiFi() const;
        void connectMqtt(const char* lastWillTopic, const std::vector<const char*>& topics, MQTT_CALLBACK_SIGNATURE);
    };
}