/*
Name:		esp8266.ino
Created:	10/4/2017 10:50:40 PM
Author:	alist
*/

#include "Esp32ScopeFunc.h"
#include <WiFi.h>          
#include <esp_wpa2.h>
#include "wifi_secrets.h"


//needed for library
#include <WiFiClient.h>
#include <DNSServer.h>


#include <IoTaDeviceHub.h>
#include "iota_defines.h"
#include "heartbeat.h"
#include "LedSerialMaster.h"
#include "DataCapsule.h"



#include "Map.h"
#include "iota_util.h"

#define HUB_SIZE 4
#define MAX_CLIENTS 10
#define MAX_UNAUTH_CLIENTS 3

WiFiServer server(2812);
IoTaDeviceHub *hub;
Heartbeat hb;
Esp32ScopeFunc scope;

long uuid;



//enum for what an authednticated client is waiting to recieve next
enum connStatusEnum
{
	NOT_CONNECTED,
	FIRST_CONNECT,
	MAGIC_BYTE_RX,
	CLIENT_ID_RX,
	CONNECTED
};

enum unAuthEnum
{
	timeIn,
	authProg,
	unAuthClient
};
//Tracking unauthenticated connections,
//@1st, int = time connected in micros
//@2nd, connStatusEnum = what stage in connection the unAuthClient is in
//@3rd, WiFIClient = unAuthClient in question
std::tuple<uint, connStatusEnum, WiFiClient> *unAuthClients;

int nextFreeSpot = 0;


iota::Map<long, WiFiClient*> *clientMap;

static const size_t bufferSize = 1024;
static uint8_t rxBuf[bufferSize];
static uint8_t txBuf[bufferSize];

std::pair<int, int> tMarkers;


const char* ssid = "symnet01";
const char* password = "GreenM00n";



void setup() {
	Serial.begin(115200);

	Serial.println("Starting IoTa ESP32");
	Serial.print("Connecting to ");
	Serial.print(ssid);



	

#ifdef UNIWIFI
	WiFi.disconnect(true);
	esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)UNI_SSID, strlen(EAP_ID));
	esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_USERNAME, strlen(EAP_USERNAME));
	esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
	esp_wifi_sta_wpa2_ent_enable();
#else
	WiFi.begin(ssid, password);
	
#endif


	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	Serial.println("Setting up hub");
	byte mac[6];
	WiFi.macAddress(mac);
	Serial.printf("Mac address: %h %h %h %h %h %h \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	Serial.printf("Chip id: %d\n", ESP.getEfuseMac());
	uuid = (ESP.getEfuseMac() << 32 | mac[0] << 24 | mac[1] << 16 | mac[2] << 9 | mac[3]);
	Serial.printf("UUID : %l", uuid);

	hub = new IoTaDeviceHub(uuid, 10);
	hub->addFunc(&hb);
	hub->addFunc(&scope);

	unAuthClients = new std::tuple<uint, connStatusEnum, WiFiClient>[MAX_UNAUTH_CLIENTS];
	for (int i = 0; i < MAX_UNAUTH_CLIENTS; i++) {
		std::get<authProg>(unAuthClients[i]) = NOT_CONNECTED;
	}

	clientMap = new iota::Map<long, WiFiClient*>(10);

	Serial.println("Starting TCP server on 2812");
	server.begin();
	scope.init();
	Serial.println("Setup complete!");
	tMarkers.first = INT_MAX;
	tMarkers.second = 0;
}



DataCapsule createDataPacket(WiFiClient *c) {
	//max length of each message is 2^16 bytes
	if (c->available() > 0)
	{


		//uint8_t msgBuffer[256];
		rxBuf[0] = c->read();
		rxBuf[1] = c->read();
		short msgLen = (rxBuf[0] << 8 | rxBuf[1]);
		//Fixed read in length here, huge speed increase due to fixing timeout!
		c->readBytes(&rxBuf[2], msgLen - 2);

		long source = typeConv::bytes2long(&rxBuf[2]);
		long dest = typeConv::bytes2long(&rxBuf[10]);
		short func = typeConv::bytes2short(&rxBuf[18]);

		short size = typeConv::bytes2short(&rxBuf[20]);

		uint8_t * data = new uint8_t[size];
		memcpy(data, &rxBuf[22], size);
		/*
		for (int i = 0; i < msgLen; i++) {
			Serial.print(rxBuf[i], HEX);
			Serial.print(" ");

		}*/
		Serial.println();
		//data is now stored inside DataCapsule, data can be freed
		DataCapsule capsule(source, dest, func, size, data);


		delete data;
		return capsule;
	}

}

void printDebug() {
	Serial.print("Free heap: ");
	Serial.println(ESP.getFreeHeap());

	Serial.print("Clients connected: ");
	Serial.println(clientMap->size());
}

void loop() {

	// Check if a new unAuthClient has connected
	WiFiClient newClient = server.available();


	/*
	Connection process:
	1. unAuthClient appears at wifi server
	2. read first byte transmitted by unAuthClient connecting
	3. if unAuthClient sent MAGIC_BYTE (65) continue
	4. send back magic byte+1
	5. wait for unAuthClient to send an empty data packet addressed for FID
	6. add clientId to clientMap
	7. enjoy your fancy new connected unAuthClient!
	*/



	//New unAuthClient, looop through unath waiting zone to find first empty
	//spot and begin auth protocol
	if (newClient) {
		for (int i = 0; i < MAX_UNAUTH_CLIENTS; i++) {
			if (std::get<authProg>(unAuthClients[i]) == NOT_CONNECTED)
			{

				int now = micros();
				std::get<timeIn>(unAuthClients[i]) = now;
				std::get<authProg>(unAuthClients[i]) = FIRST_CONNECT;
				std::get<unAuthClient>(unAuthClients[i]) = newClient;
				i = MAX_UNAUTH_CLIENTS;
			}
		}
	}

	uint tnow = micros();
	uint timeout_us = 2000000;
	for (int i = 0; i < MAX_UNAUTH_CLIENTS; i++) {
		yield();
		if (tnow - std::get<timeIn>(unAuthClients[i]) > timeout_us && std::get<authProg>(unAuthClients[i]) != NOT_CONNECTED) {


			//remove due to lack of connections

			std::get<timeIn>(unAuthClients[i]) = 0;
			std::get<authProg>(unAuthClients[i]) = NOT_CONNECTED;
			std::get<unAuthClient>(unAuthClients[i]).stop();
		}
		else if (std::get<authProg>(unAuthClients[i]) != NOT_CONNECTED)
		{
			switch (std::get<authProg>(unAuthClients[i]))
			{
			case(FIRST_CONNECT):

				if (std::get<unAuthClient>(unAuthClients[i]).available() > 0) {

					uint8_t firstContact = std::get<unAuthClient>(unAuthClients[i]).read();


					if (firstContact == MAGIC_BYTE) {
						std::get<authProg>(unAuthClients[i]) = MAGIC_BYTE_RX;
					}
				}
				break;

			case(MAGIC_BYTE_RX):
				std::get<unAuthClient>(unAuthClients[i]).write(MAGIC_BYTE + 1);
				std::get<authProg>(unAuthClients[i]) = CLIENT_ID_RX;

				break;

			case(CLIENT_ID_RX):
				if (std::get<unAuthClient>(unAuthClients[i]).available() > 0) {
					DataCapsule cap = createDataPacket(&std::get<unAuthClient>(unAuthClients[i]));

					printCapsuleDetails(&cap);
					clientMap->put(cap.getSource(), &std::get<unAuthClient>(unAuthClients[i]));
					std::get<authProg>(unAuthClients[i]) = CONNECTED;

					DataCapsule outCap(uuid, cap.getSource(), FID_HUB, 0, NULL);


					size_t pLen = cap.getTcpPacketLength();
					uint8_t bytes[pLen];

					cap.createTcpPacket(bytes);
					for (int i = 0; i < pLen; i++) {
						Serial.print(" ");
						Serial.print(bytes[i], HEX);
					}
					Serial.println("<- empty capsule");
					std::get<unAuthClient>(unAuthClients[i]).write(&bytes[0], pLen);
					Serial.print(cap.getSource());
					Serial.println(" has auth'd");
					clientMap->put(cap.getSource(), &std::get<unAuthClient>(unAuthClients[i]));

				}

				break;

			case(CONNECTED): //client has been authenticated and moved to clientMap, time to clean up its spot
				std::get<timeIn>(unAuthClients[i]) = 0;
				std::get<authProg>(unAuthClients[i]) = NOT_CONNECTED;

				break;
			default:
				break;
			}

		}
	}







	//iterate through connected clients, if they have any data in buffer
	//read in a and process it
	for (int i = 0; i < clientMap->getMaxSize(); i++) {
		std::pair<long, WiFiClient*> *entries = clientMap->getEntryReference();

		if (entries[i].first != NULL) {
			if (!entries[i].second->connected()) {
				Serial.print(entries[i].first);
				Serial.println(" has disconnected");
				clientMap->remove(entries[i].first);

			}
			else if (entries[i].second->available() > 0) {

				DataCapsule cap = createDataPacket(entries[i].second);
				printCapsuleDetails(&cap);
				hub->processMessage(&cap);
			}
		}

	}

	yield();
	//process internal updates from hub
	hub->tick();
	yield();


	//get and broadcast state updates
	//while broadcasts waiting
	//tx to each attached unAuthClient

	while (hub->numCapsulesRemaining() > 0)
	{
		Serial.print("Responses left: ");
		Serial.println(hub->numCapsulesRemaining());
		DataCapsule *cap;
		hub->getNextOutputCapsule(&cap);

		if (cap->getDestination() == 0) {//broadcast to all connected clients
			Serial.println("Got to deliver a bcast cap");
			std::pair<long, WiFiClient*> * refs = clientMap->getEntryReference();
			int tcpLen = cap->getTcpPacketLength();
			uint8_t *txBuffer;
			txBuffer = new uint8_t[cap->getTcpPacketLength()];

			cap->createTcpPacket(txBuffer);
			
			for (int i = 0; i < clientMap->getMaxSize(); i++) {
				if (refs[i].first != 0) {
					//replace blank area with client id

					//or don't, we just need this to work atm					
					refs[i].second->write(&txBuffer[0], tcpLen);


				}
			}
			delete[] txBuffer;
		}
		else
		{
			uint8_t *txBuffer;
			txBuffer = new uint8_t[cap->getTcpPacketLength()];

			cap->createTcpPacket(txBuffer);

			WiFiClient *c = clientMap->get(cap->getDestination());
			/*
			for (int i = 0; i < cap->getTcpPacketLength(); i++) {
				Serial.print(txBuffer[i], HEX);
				Serial.print(" ");
			}
			Serial.println();
			*/
			c->write(&txBuffer[0], cap->getTcpPacketLength());

			delete[] txBuffer;
		}
	}


	//timed debug
	int deltaMicros = 5000000; //5 seconds
	tMarkers.second = micros();
	if ((tMarkers.first - tMarkers.second) >= deltaMicros) {
		tMarkers.first = micros();
		printDebug();
		tMarkers.second = micros();
	}

}

void printCapsuleDetails(DataCapsule *c) {
	Serial.print("Source: ");
	Serial.println(c->getSource());
	Serial.print("Destination: ");
	Serial.println(c->getDestination());
	Serial.print("Func id: ");
	Serial.println(c->getFuncId());
	Serial.print("Attached data size: ");
	Serial.println(c->getDataSize());
}


