#pragma once

#include <NimBLEDevice.h>
#include <memory>
#include <functional>
#include <Arduino.h>
#include <vector>

#include "Utils.h"
#include "Settings.h"
#include "MessageBase.h"
#include "RoboMessageCreator.h"
#include "RsRoboMessageCreator.h"

#define BLE_CALLBACK_SIGNATURE std::function<void(uint8_t* pData, size_t length)>

namespace Robo
{
	static NimBLEUUID RoboServiceUUID("ff00a501-d020-913c-1234-56d97200a6a6");
	static NimBLEUUID AuthCharUUID("ff00a502-d020-913c-1234-56d97200a6a6");
	static NimBLEUUID DataCharUUID("ff00a503-d020-913c-1234-56d97200a6a6");
	static NimBLEUUID NotifyCharUUID("ff00a506-d020-913c-1234-56d97200a6a6");

	class RoboBLE
		  : public NimBLEAdvertisedDeviceCallbacks
          , public NimBLEClientCallbacks
	{
		bool doConnect = false;
		bool connected = false;

        NimBLEClient* bleClient = nullptr;
        NimBLEAdvertisedDevice* bleDevice = nullptr;

        NimBLERemoteService* remoteService = nullptr;
		NimBLERemoteCharacteristic* authCharacteristic = nullptr;
		NimBLERemoteCharacteristic* notifyCharacteristic = nullptr;
		NimBLERemoteCharacteristic* dataCharacteristic = nullptr;

        // Polling
        uint16_t      cmdCounter  = 1;
        unsigned long lastPollMs  = 0;
        static constexpr unsigned long POLL_INTERVAL_MS = 3000UL;

        void AutoPoll();

		void onConnect(NimBLEClient* pClient) override;
		void onDisconnect(NimBLEClient* pClient) override;
		void notifyCallback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
		void onResult(NimBLEAdvertisedDevice* advertisedDevice) override;

		void ConnectToServer();
		void DoAuth();
		bool GetService();
		bool GetAuthCharacteristic();
		bool GetNotifyCharacteristic();
		bool GetDataCharacteristic();

		protected:
			BLE_CALLBACK_SIGNATURE currentCallback;
			std::string deviceName;

		public:
			explicit RoboBLE(const char* deviceName);

            std::unique_ptr<Robo::RoboMessageCreator> MessageCreator;

            // Current mower state – updated after every poll response
            MowerState mowerState;

            // Optional callback on state change (e.g. for MQTT publish)
            std::function<void(const MowerState&)> onStateUpdate;

			void Setup();
			void Scan();
            void Connect();
			void Loop();
			void GetRobotConfiguration();
			void SendMessage(std::vector<byte>& data, BLE_CALLBACK_SIGNATURE callback);

            // Sends start/stop/base command (LSOperationModes: Stop=0, Edge=1, Scan=2, Base=3)
            void SendCommand(uint8_t mode);
    };
}