#pragma once

#include <NimBLEDevice.h>
#include <memory>
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

        NimBLEClient* bleClient;
        NimBLEAdvertisedDevice* bleDevice;

        NimBLERemoteService* remoteService;
		NimBLERemoteCharacteristic* authCharacteristic;
		NimBLERemoteCharacteristic* notifyCharacteristic;
		NimBLERemoteCharacteristic* dataCharacteristic;

		void onConnect(NimBLEClient* pClient) override; // BLEClientCallbacks
		void onDisconnect(NimBLEClient* pClient) override; // BLEClientCallbacks
		void notifyCallback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify); // NotifyCallback
		void onResult(NimBLEAdvertisedDevice* advertisedDevice) override; // BLEAdvertisedDeviceCallbacks

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

			void Setup();
			void Scan();
            void Connect();
			void Loop();
			void GetRobotConfiguration();
			void SendMessage(std::vector<byte>& data, BLE_CALLBACK_SIGNATURE callback);
    };
}