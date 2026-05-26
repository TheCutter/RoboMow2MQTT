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
		NimBLEDevice::init(this->deviceName);
		Serial.println("BLE setup complete!");
	}

	void RoboBLE::Scan()
	{
		NimBLEScan* pBLEScan = NimBLEDevice::getScan();
		pBLEScan->setAdvertisedDeviceCallbacks(this);
		pBLEScan->setActiveScan(true);
		pBLEScan->setInterval(1349);
		pBLEScan->setWindow(449);

		Serial.println("[SCAN] Searching for mower (name starts with 'Mo')...");
		pBLEScan->start(15, false);
	}

	void RoboBLE::onResult(NimBLEAdvertisedDevice* advertisedDevice)
	{
		std::string name = advertisedDevice->getName();
		if (name.rfind("Mo", 0) != 0) return;

		if (!advertisedDevice->isAdvertisingService(RoboServiceUUID)) {
			return;
		}

		Serial.printf("[SCAN] Mower found: %s (%s)\n",
		              name.c_str(), advertisedDevice->getAddress().toString().c_str());

		NimBLEDevice::getScan()->stop();
		this->bleDevice  = advertisedDevice;
		this->doConnect  = true;
	}

	void RoboBLE::Loop()
	{
		if (this->doConnect)
		{
			this->doConnect = false;
			this->ConnectToServer();
			this->DoAuth();
		}

		if (this->connected)
		{
			AutoPoll();
		}
	}

	// ─── Auto-polling (every 3 seconds) ──────────────────────────────────────────

	void RoboBLE::AutoPoll()
	{
		if (!this->MessageCreator)   return;
		if (!this->dataCharacteristic) return;
		if (millis() - this->lastPollMs < POLL_INTERVAL_MS) return;
		this->lastPollMs = millis();

		auto req = this->MessageCreator->GetRoboStateMessage();
		req.ComCount = this->cmdCounter++;
		if (this->cmdCounter >= 65535) this->cmdCounter = 1;

		auto buffer = this->MessageCreator->SerializeMessage(req);

		this->SendMessage(buffer, [this](byte* pData, size_t length) {
			this->mowerState = RoboMessageCreator::ParseStateResponse(pData, length);
			if (this->mowerState.valid && this->onStateUpdate) {
				this->onStateUpdate(this->mowerState);
			}
		});
	}

	// ─── Start/stop/base command ──────────────────────────────────────────────────

	void RoboBLE::SendCommand(uint8_t mode)
	{
		if (!this->MessageCreator) {
			Serial.println("[CMD] MessageCreator not set");
			return;
		}
		if (!this->connected) {
			Serial.println("[CMD] Not connected");
			return;
		}

		auto cmd = this->MessageCreator->GetAutoOperationMessage(mode);
		cmd.ComCount = this->cmdCounter++;
		if (this->cmdCounter >= 65535) this->cmdCounter = 1;

		auto buffer = this->MessageCreator->SerializeMessage(cmd);

		Serial.printf("[CMD] Sending mode=%d\n", mode);
		this->SendMessage(buffer, nullptr);
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
		Serial.println("[AUTH] Starting authentication...");

		if (!this->authCharacteristic->canWrite()) {
			Serial.println("[AUTH] Auth char not writable");
			return;
		}

		// Serial number null-padded to 15 bytes (makeAuthConnection)
		uint8_t snBuf[15] = {0};
		size_t  snLen     = strlen(ROBOSERIAL);
		if (snLen > 15) snLen = 15;
		memcpy(snBuf, ROBOSERIAL, snLen);

		Serial.printf("[AUTH] Sending SN: %.15s\n", (char*)snBuf);

		if (!this->authCharacteristic->writeValue(snBuf, 15, true)) {
			Serial.println("[AUTH] Write failed");
			return;
		}

		if (!this->authCharacteristic->canRead()) {
			Serial.println("[AUTH] Auth char not readable");
			return;
		}

		// isAuthenticationSuccessful: all bytes != 0
		std::string val = this->authCharacteristic->readValue();
		if (val.empty()) {
			Serial.println("[AUTH] Response empty");
			return;
		}
		for (unsigned char c : val) {
			if (c == 0) {
				Serial.println("[AUTH] Failed (0-byte in response)");
				return;
			}
		}

		Serial.println("[AUTH] Success");
		this->connected = true;
		this->GetRobotConfiguration();
	}

	bool RoboBLE::GetService()
	{
		this->remoteService = this->bleClient->getService(RoboServiceUUID);
		if (this->remoteService == nullptr)
		{
			Serial.printf("[BLE] Service not found: %s\n", RoboServiceUUID.toString().c_str());
			this->bleClient->disconnect();
			return false;
		}
		Serial.println("[BLE] Service found");
		return true;
	}

	bool RoboBLE::GetAuthCharacteristic()
	{
		this->authCharacteristic = this->remoteService->getCharacteristic(AuthCharUUID);
		if (this->authCharacteristic == nullptr)
		{
			Serial.println("[BLE] Auth char not found");
			this->bleClient->disconnect();
			return false;
		}
		if (!this->authCharacteristic->canWrite() || !this->authCharacteristic->canRead())
		{
			Serial.println("[BLE] Auth char not readable/writable");
			this->bleClient->disconnect();
			return false;
		}
		Serial.println("[BLE] Auth char found");
		return true;
	}

	bool RoboBLE::GetNotifyCharacteristic()
	{
		// Primary: ff00a506
		this->notifyCharacteristic = this->remoteService->getCharacteristic(NotifyCharUUID);
		NimBLERemoteCharacteristic* notifyTarget = this->notifyCharacteristic;

		// Fallback: ff00a503 (older firmware)
		if (!notifyTarget || !notifyTarget->canNotify()) {
			Serial.println("[BLE] Notify char (a506) unavailable, trying fallback (a503)");
			notifyTarget = this->dataCharacteristic;
		}

		if (!notifyTarget || !notifyTarget->canNotify()) {
			Serial.println("[BLE] No notify char available");
			this->bleClient->disconnect();
			return false;
		}

		if (!notifyTarget->subscribe(true,
			[this](NimBLERemoteCharacteristic* c, uint8_t* d, size_t l, bool n) {
				this->notifyCallback(c, d, l, n);
			}))
		{
			Serial.println("[BLE] Notify subscribe failed");
			this->bleClient->disconnect();
			return false;
		}

		Serial.printf("[BLE] Notifications active (%s)\n",
		              notifyTarget->getUUID().toString().c_str());
		return true;
	}

	bool RoboBLE::GetDataCharacteristic()
	{
		this->dataCharacteristic = this->remoteService->getCharacteristic(DataCharUUID);
		if (this->dataCharacteristic == nullptr)
		{
			Serial.println("[BLE] Data char not found");
			this->bleClient->disconnect();
			return false;
		}
		if (!this->dataCharacteristic->canWrite())
		{
			Serial.println("[BLE] Data char not writable");
			this->bleClient->disconnect();
			return false;
		}
		Serial.println("[BLE] Data char found");
		return true;
	}

	void RoboBLE::ConnectToServer()
	{
		Serial.printf("[BLE] Connecting to %s ...\n",
		              this->bleDevice->getAddress().toString().c_str());

		if (NimBLEDevice::getClientListSize()) {
			this->bleClient = NimBLEDevice::getClientByPeerAddress(this->bleDevice->getAddress());
			if (this->bleClient) {
				if (!this->bleClient->connect(this->bleDevice, false)) {
					Serial.println("[BLE] Reconnect failed");
					return;
				}
				Serial.println("[BLE] Client reused");
			} else {
				this->bleClient = NimBLEDevice::getDisconnectedClient();
			}
		}

		if (!this->bleClient) {
			if (NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
				Serial.println("[BLE] Max. connections reached");
				return;
			}
			this->bleClient = NimBLEDevice::createClient();
			this->bleClient->setClientCallbacks(this, false);
			this->bleClient->setConnectionParams(6, 36, 0, 500);
			this->bleClient->setConnectTimeout(30);

			if (!this->bleClient->connect(this->bleDevice)) {
				NimBLEDevice::deleteClient(this->bleClient);
				this->bleClient = nullptr;
				Serial.println("[BLE] Connection failed");
				return;
			}
		}

		if (!this->bleClient->isConnected()) {
			Serial.println("[BLE] Not connected after connect()");
			return;
		}

		Serial.printf("[BLE] Connected. RSSI: %d\n", this->bleClient->getRssi());

		if (!GetService())            return;
		if (!GetAuthCharacteristic()) return;
		if (!GetDataCharacteristic()) return;
		if (!GetNotifyCharacteristic()) return;

		Serial.println("[BLE] All characteristics ready");
	}

	#pragma endregion

	void RoboBLE::notifyCallback(NimBLERemoteCharacteristic*, uint8_t* pData, size_t length, bool)
	{
		// Log raw data (signed as in Java logs)
		Serial.print("[RX]");
		for (size_t i = 0; i < length; i++)
			Serial.printf(" %d", (int8_t)pData[i]);
		Serial.println();

		if (length < 4 || pData[0] != 0xAA) {
			Serial.println("[RX] Invalid header");
			return;
		}

		if (this->currentCallback) {
			this->currentCallback(pData, length);
		}
	}

	void RoboBLE::onConnect(NimBLEClient*)
	{
		Serial.println("[BLE] onConnect");
	}

	void RoboBLE::onDisconnect(NimBLEClient*)
	{
		Serial.println("[BLE] Disconnected – reconnect on next scan");
		this->connected            = false;
		this->doConnect            = false;
		this->dataCharacteristic   = nullptr;
		this->notifyCharacteristic = nullptr;
		this->authCharacteristic   = nullptr;
		this->remoteService        = nullptr;
	}

	void RoboBLE::GetRobotConfiguration()
	{
		std::vector<byte> configMessage = RoboMessageCreator::GetConfigRequestMessage();

		this->SendMessage(configMessage, [this](byte* pData, size_t length)
		{
			const Robo::ConfigMessageResponse response =
			    RoboMessageCreator::GetConfigResponseMessage(pData, length);

			switch (response.Family)
			{
				case Robo::RS:
					this->MessageCreator.reset(new Robo::RsRoboMessageCreator());
					Serial.println("[BLE] MessageCreator = RS");
					break;
				default:
					this->MessageCreator.reset();
					Serial.printf("[BLE] Unknown family: %d\n", response.Family);
			}

			Serial.printf("[CFG] Family=%d SW=%d.%d MB=%d\n",
			              response.Family, response.SoftwareVersion,
			              response.SoftwareRelease, response.MainboardVersion);
		});
	}

	void RoboBLE::SendMessage(std::vector<byte>& data, BLE_CALLBACK_SIGNATURE callback)
	{
		if (!this->connected) {
			Serial.println("[TX] Not connected");
			return;
		}

		this->currentCallback = std::move(callback);

		if (data[1] == 0) data[1] = (byte)(data.size() + 1);

		byte checksum = RoboMessageCreator::CreateChecksum(data);
		data.push_back(checksum);

		Serial.print("[TX]");
		for (size_t i = 0; i < data.size(); i++)
			Serial.printf(" %d", (int8_t)data[i]);
		Serial.println();

		this->dataCharacteristic->writeValue(data.data(), data.size(), true);
	}
}
