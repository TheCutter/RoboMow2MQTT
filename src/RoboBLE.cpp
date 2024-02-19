#include "RoboBLE.h"

namespace Robo
{
	RoboBLE::RoboBLE(const char* deviceName)
		: deviceName(deviceName)
	{
	}

	#pragma region BLE Stuff

	void RoboBLE::Setup()
	{
		Serial.println("Init BLE...");
		BLEDevice::init(this->deviceName);

        Serial.println("Creating BLESecurity...");
		BLESecurity bleSecurity;
		bleSecurity.setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
		bleSecurity.setCapability(ESP_IO_CAP_OUT);
		bleSecurity.setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

		Serial.println("BLE setup complete!");
	}

	void RoboBLE::Scan()
	{
		BLEScan* pBLEScan = BLEDevice::getScan();
		pBLEScan->setAdvertisedDeviceCallbacks(this);
		pBLEScan->setInterval(1349);
	    pBLEScan->setWindow(449);

		Serial.println("BLE Scan started...");
		pBLEScan->start(15);
	}

	// Called for each advertising BLE server.
	void RoboBLE::onResult(BLEAdvertisedDevice* advertisedDevice)
	{
        // We have found a device, let us now see if it contains the service we are looking for.
		if (!advertisedDevice->isAdvertisingService(RoboServiceUUID)) {
            return;
        }

		BLEDevice::getScan()->stop();
		this->bleDevice = advertisedDevice;
        Serial.println("Found Robomow device!");

        //this->doConnect = true;
	}

	void RoboBLE::Loop()
	{
        return;

		if (this->doConnect)
		{
			// If the flag "doConnect" is true then we have scanned for and found the desired
			// BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
			// connected we set the connected flag to be true.
			this->doConnect = false;
			this->ConnectToServer();
			this->DoAuth();
		}

		if (this->connected)
		{
			//if (millis() - this->lastNoopTime > 2000)
			//{
			//	byte noopData[] = {0xAA, 0x05, 0x1F, 0x1B, 0x16};
			//	this->lastNoopTime = millis();
			//	this->dataCharacteristic->writeValue(noopData, 5);
			//}
		}
	}

    void RoboBLE::Connect()
    {
        if (this->bleDevice == nullptr) {
            Serial.println("No Robomow Device set.");
            return;
        }

        this->ConnectToServer();
        this->DoAuth();
    }

	void RoboBLE::DoAuth()
	{
		Serial.println("Sending auth...");

		if (this->authCharacteristic->canWrite())
		{
			std::array<byte, 15> authData{ROBOSERIAL};
			this->authCharacteristic->writeValue(authData.data(), authData.size(), true);
			Serial.println("Auth Data send!");
		}
		else
		{
			Serial.println("The auth characteristic can not be written.");
		}

		// Read the value of the characteristic.
		if (this->authCharacteristic->canRead())
		{
			const byte value = this->authCharacteristic->readValue<uint8_t>();
			Serial.printf("The characteristic value was: %x.", value);

			if (value == 1)
			{
				this->connected = true;
                if (this->MessageCreator == nullptr) this->GetRobotConfiguration();
			}
		}
		else
		{
			Serial.println("The auth characteristic can not be read.");
		}
	}

	bool RoboBLE::GetService()
	{
		// Obtain a reference to the service we are after in the remote BLE server.
		this->remoteService = this->bleClient->getService(RoboServiceUUID);
		if (this->remoteService == nullptr)
		{
			Serial.printf("Failed to find our service UUID: %s.", RoboServiceUUID.toString().c_str());

			this->bleClient->disconnect();
			return false;
		}

		Serial.println("Found our service.");
		return true;
	}

	bool RoboBLE::GetAuthCharacteristic()
	{
		// Obtain a reference to the characteristic in the service of the remote BLE server.
		this->authCharacteristic = this->remoteService->getCharacteristic(AuthCharUUID);
		if (this->authCharacteristic == nullptr)
		{
			Serial.println("Failed to find auth characteristic.");

			this->bleClient->disconnect();
			return false;
		}
		if (!this->authCharacteristic->canWrite() || !this->authCharacteristic->canRead())
		{
			Serial.println("Can not read/write auth characteristic.");

			this->bleClient->disconnect();
			return false;
		}

		Serial.println("Found auth characteristic!");
		return true;
	}

	bool RoboBLE::GetNotifyCharacteristic()
	{
		this->notifyCharacteristic = this->remoteService->getCharacteristic(NotifyCharUUID);
		if (this->notifyCharacteristic == nullptr)
		{
			Serial.println("Failed to find notify characteristic.");

			this->bleClient->disconnect();
			return false;
		}

		if (this->notifyCharacteristic->canNotify())
		{
			this->notifyCharacteristic->subscribe(true,
				[this](BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify)
				{
					this->notifyCallback(pBLERemoteCharacteristic, pData, length, isNotify);
				});
			
			Serial.println("Registered notify characteristic for notify");
		}
		else
		{
			Serial.println("Notify characteristic can not notify.");
			this->bleClient->disconnect();
			return false;
		}

		if (!this->notifyCharacteristic->canRead())
		{
			Serial.println("Can not read notify characteristic.");
			this->bleClient->disconnect();
			return false;
		}

		Serial.println("Found notify characteristic!");
		return true;
	}

	bool RoboBLE::GetDataCharacteristic()
	{
		this->dataCharacteristic = this->remoteService->getCharacteristic(DataCharUUID);
		if (this->dataCharacteristic == nullptr)
		{
			Serial.println("Failed to find data characteristic.");

			this->bleClient->disconnect();
			return false;
		}

		if (!this->dataCharacteristic->canWrite() || !this->dataCharacteristic->canRead())
		{
			Serial.println("Can not read/write data characteristic.");

			this->bleClient->disconnect();
			return false;
		}

		Serial.println("Found data characteristic!");
		return true;
	}

	void RoboBLE::ConnectToServer()
	{
		Serial.printf("Forming a connection to %s.", this->bleDevice->getAddress().toString().c_str());

        /** Check if we have a client we should reuse first **/
        if (NimBLEDevice::getClientListSize())
        {
            /** Special case when we already know this device, we send false as the
             *  second argument in connect() to prevent refreshing the service database.
             *  This saves considerable time and power.
             */
            this->bleClient = NimBLEDevice::getClientByPeerAddress(this->bleDevice->getAddress());
            Serial.println("Reused Client found");

            if (this->bleClient)
            {
                if (!this->bleClient->connect(this->bleDevice, false))
                {
                    Serial.println("Reconnect failed");
                    return;
                }

                Serial.println("Reconnected client");
            }
            /** We don't already have a client that knows this device,
             *  we will check for a client that is disconnected that we can use.
             */
            else
            {
                this->bleClient = NimBLEDevice::getDisconnectedClient();
            }
        }

        /** No client to reuse? Create a new one. */
        if (!this->bleClient) {
            if (NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
                Serial.println("Max clients reached - no more connections available");
                return;
            }

            this->bleClient = NimBLEDevice::createClient();
            Serial.println("New client created");

            this->bleClient->setClientCallbacks(this, false);

            /** Set initial connection parameters: These settings are 15ms interval, 0 latency, 120ms timout.
             *  These settings are safe for 3 clients to connect reliably, can go faster if you have less
             *  connections. Timeout should be a multiple of the interval, minimum is 100ms.
             *  Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 51 * 10ms = 510ms timeout
             */
            this->bleClient->setConnectionParams(6, 36, 0, 500);

            /** Set how long we are willing to wait for the connection to complete (seconds), default is 30. */
            this->bleClient->setConnectTimeout(30);

            if (!this->bleClient->connect(this->bleDevice)) {
                /** Created a client but failed to connect, don't need to keep it as it has no data */
                NimBLEDevice::deleteClient(this->bleClient);
                Serial.println("Failed to connect, deleted client");
                return;
            }
        }

        if (!this->bleClient->isConnected()) {
            Serial.println("Failed to connect");
            return;
        }

        Serial.print("Connected to: ");
        Serial.println(this->bleClient->getPeerAddress().toString().c_str());

        Serial.print("RSSI: ");
        Serial.println(this->bleClient->getRssi());

		Serial.println("Connected! Getting Characteristics...");

		if (!GetService())
		{
			Serial.println("Could not get Service!");
			return;
		}
		if (!GetAuthCharacteristic())
		{
			Serial.println("Could not get Auth Characteristic!");
			return;
		}
		if (!GetDataCharacteristic())
		{
			Serial.println("Could not get Data Characteristic!");
			return;
		}
		if (!GetNotifyCharacteristic())
		{
			Serial.println("Could not get Notify Characteristic!");
			return;
		}

		Serial.println("Service and Characteristics created. Connected!");
	}

	#pragma endregion

	void RoboBLE::notifyCallback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify)
	{
		Serial.println("NotifyCallback called");

		char str[64] = "";
		Utils::array_to_string(pData, length, str);
		Serial.printf("Notify received with length '%d' and Data: %s.", length, str);
		Serial.println();

		if (pData[0] != 170)
		{
			Serial.println("Message does not start with byte 170");
			return;
		}

		if (pData[1] < length)
		{
			Serial.println("Message length not valid");
			return;
		}

		if (pData[1] > length)
		{
			//TODO: Long Messages / Multiple Notify Messages
			return;
		}
		
		if (this->currentCallback)
		{
			this->currentCallback(pData, length);
		}
	}

	void RoboBLE::onConnect(NimBLEClient* pClient)
	{
		Serial.println("Connected to server!");
        //pClient->updateConnParams(36,36,0,500);
    }

	void RoboBLE::onDisconnect(NimBLEClient* pClient)
	{
		Serial.println("Disconnected from server!");
		this->connected = false;
		this->doConnect = false;

        this->bleClient->disconnect();

        return;

//		Serial.println("Reset authCharacteristic");
//		this->authCharacteristic.reset();
//
//		Serial.println("Reset dataCharacteristic");
//		this->dataCharacteristic.reset();
//
//		Serial.println("Reset notifyCharacteristic");
//		this->notifyCharacteristic.reset();
//
//        Serial.println("Reset remoteService");
//		this->remoteService.reset();

        return;

        this->remoteService = nullptr;
        this->bleDevice = nullptr;

//		Serial.println("Reset bleDevice");
//		this->bleDevice.reset();
	}

	void RoboBLE::GetRobotConfiguration()
	{
		std::vector<byte> configMessage = RoboMessageCreator::GetConfigRequestMessage();

		this->SendMessage(configMessage, [this] (byte* pData, size_t length)
		{
			const Robo::ConfigMessageResponse response = RoboMessageCreator::GetConfigResponseMessage(pData, length);
			switch (response.Family)
			{
				case Robo::RS:
					this->MessageCreator.reset(new Robo::RsRoboMessageCreator());
                    Serial.println("MessageCreator set to RS");
                    break;
				default:
					this->MessageCreator.reset();
			}

			Serial.printf("Family set to: %d. Software Version: %d. Software Release: %d. Mainboard Version: %d.", response.Family, response.SoftwareVersion, response.SoftwareRelease, response.MainboardVersion);
			Serial.println();
		});
	}

	void RoboBLE::SendMessage(std::vector<byte>& data, BLE_CALLBACK_SIGNATURE callback)
	{
		Serial.println("SendMessage called");

		if (!this->connected)
		{ 
			Serial.println("Not connected...");
			return;
		}

		this->currentCallback = std::move(callback);

		if (data[1] == 0) data[1] = data.size() + 1;

		Serial.println("Converting Data to String...");
		byte checksum = RoboMessageCreator::CreateChecksum(data);
		data.push_back(checksum);

		char str[64] = "";
		Utils::array_to_string(data.data(), data.size(), str);
		Serial.printf("Sending data: %s with length '%d'", str, data.size());
		Serial.println();

		Serial.println("Writing message data...");
		this->dataCharacteristic->writeValue(data.data(), data.size(), true);
		Serial.println("Message data written.");
	}
}