#include "Robomow.h"

Robo::RoboBLE ble(HOSTNAME);
Robo::WifiMqtt wifiMqtt;

// ─── Publish state as JSON to robomow/status ─────────────────────────────────

static void publishState(const Robo::MowerState& s)
{
    if (!wifiMqtt.mqttClient.connected()) return;

    char json[160];
    snprintf(json, sizeof(json),
        "{\"battery\":%d,\"charging\":%s,\"mowing\":%s,\"mode\":%d}",
        s.battery,
        s.charging ? "true" : "false",
        s.mowing   ? "true" : "false",
        s.mode
    );

    wifiMqtt.mqttClient.publish("robomow/status", json);
    Serial.printf("[MQTT] %s\n", json);
}

// ─── MQTT Callback ────────────────────────────────────────────────────────────

void mqttCallback(char* topic, byte* message, unsigned int length)
{
    Serial.printf("[MQTT] Message on %s\n", topic);

    // robomow/action – manual control (Scan, Connect, Reset)
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
        else if (memcmp(message, "reset", length) == 0)
        {
            ESP.restart();
        }
    }

    // robomow/command – mower commands: start / stop / base
    else if (strcmp(topic, "robomow/command") == 0)
    {
        char cmd[16] = {0};
        size_t copyLen = length < sizeof(cmd) - 1 ? length : sizeof(cmd) - 1;
        memcpy(cmd, message, copyLen);

        if (strcmp(cmd, "start") == 0) {
            Serial.println("[CMD] Start (Scan/Mow)");
            ble.SendCommand(2);  // LSOperationModes.Scan = 2
        }
        else if (strcmp(cmd, "stop") == 0) {
            Serial.println("[CMD] Stop");
            ble.SendCommand(0);  // LSOperationModes.Stop = 0
        }
        else if (strcmp(cmd, "base") == 0) {
            Serial.println("[CMD] To base station");
            ble.SendCommand(3);  // LSOperationModes.Base = 3
        }
        else {
            Serial.printf("[CMD] Unknown command: %s\n", cmd);
        }
    }
}

// ─── Setup ────────────────────────────────────────────────────────────────────

void setup()
{
    Serial.begin(115200);

    if (!wifiMqtt.connectToWiFi()) return;
    wifiMqtt.setupMqtt();

    const std::vector<const char*> topics = {
        "robomow/action",
        "robomow/command"
    };
    wifiMqtt.connectMqtt("robomow/status", topics, mqttCallback);

    // State callback: publish automatically on every poll response
    ble.onStateUpdate = publishState;

    ble.Setup();
    ble.Scan();
}

// ─── Loop ─────────────────────────────────────────────────────────────────────

static unsigned long lastHeapMs = 0;

void loop()
{
    wifiMqtt.loop();
    ble.Loop();

    if (millis() - lastHeapMs > 10000)
    {
        wifiMqtt.mqttClient.publish("robomow/memory/heap",
                                    String(ESP.getFreeHeap()).c_str());
        lastHeapMs = millis();
    }
}
