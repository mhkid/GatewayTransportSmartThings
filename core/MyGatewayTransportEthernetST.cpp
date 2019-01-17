/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Tomas Hozza <thozza@gmail.com>
 * Copyright (C) 2015  Tomas Hozza
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "MyGatewayTransport.h"

// global variables
extern MyMessage _msgTmp;

// housekeeping, remove for 3.0.0
#ifdef MY_ESP8266_SSID
#warning MY_ESP8266_SSID is deprecated, use MY_WIFI_SSID instead!
#define MY_WIFI_SSID MY_ESP8266_SSID
#undef MY_ESP8266_SSID // cleanup
#endif

#ifdef MY_ESP8266_PASSWORD
#warning MY_ESP8266_PASSWORD is deprecated, use MY_WIFI_PASSWORD instead!
#define MY_WIFI_PASSWORD MY_ESP8266_PASSWORD
#undef MY_ESP8266_PASSWORD // cleanup
#endif

#ifdef MY_ESP8266_BSSID
#warning MY_ESP8266_BSSID is deprecated, use MY_WIFI_BSSID instead!
#define MY_WIFI_BSSID MY_ESP8266_BSSID
#undef MY_ESP8266_BSSID // cleanup
#endif

#ifdef MY_ESP8266_HOSTNAME
#warning MY_ESP8266_HOSTNAME is deprecated, use MY_HOSTNAME instead!
#define MY_HOSTNAME MY_ESP8266_HOSTNAME
#undef MY_ESP8266_HOSTNAME // cleanup
#endif

#ifndef MY_WIFI_BSSID
#define MY_WIFI_BSSID NULL
#endif

#if defined(MY_CONTROLLER_IP_ADDRESS)
IPAddress _ethernetControllerIP(MY_CONTROLLER_IP_ADDRESS);
#endif

#if defined(MY_IP_ADDRESS)
IPAddress _ethernetGatewayIP(MY_IP_ADDRESS);
#if defined(MY_IP_GATEWAY_ADDRESS)
IPAddress _gatewayIp(MY_IP_GATEWAY_ADDRESS);
#elif defined(MY_GATEWAY_ESP8266) || defined(MY_GATEWAY_ESP32)
// Assume the gateway will be the machine on the same network as the local IP
// but with last octet being '1'
IPAddress _gatewayIp(_ethernetGatewayIP[0], _ethernetGatewayIP[1], _ethernetGatewayIP[2], 1);
#endif /* End of MY_IP_GATEWAY_ADDRESS */
#if defined(MY_IP_SUBNET_ADDRESS)
IPAddress _subnetIp(MY_IP_SUBNET_ADDRESS);
#elif defined(MY_GATEWAY_ESP8266) || defined(MY_GATEWAY_ESP32)
IPAddress _subnetIp(255, 255, 255, 0);
#endif /* End of MY_IP_SUBNET_ADDRESS */
#endif /* End of MY_IP_ADDRESS */

uint8_t _ethernetGatewayMAC[] = { MY_MAC_ADDRESS };
uint16_t _ethernetGatewayPort = MY_PORT;
MyMessage _ethernetMsg;

#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))

typedef struct {
	// Suppress the warning about unused members in this struct because it is used through a complex
	// set of preprocessor directives
	// cppcheck-suppress unusedStructMember
	char string[MY_GATEWAY_MAX_RECEIVE_LENGTH];
	// cppcheck-suppress unusedStructMember
	uint8_t idx;
} inputBuffer;

EthernetServer _ethernetServer(80, MY_GATEWAY_MAX_CLIENTS);

static EthernetClient client = EthernetClient();
static EthernetClient clients[MY_GATEWAY_MAX_CLIENTS];
static bool clientsConnected[MY_GATEWAY_MAX_CLIENTS];
static inputBuffer inputString[MY_GATEWAY_MAX_CLIENTS];

static EthernetClient httpClient;

String readString = "";
bool currentLineIsBlank = true;

#define _w5100_spi_en(x)

void sendToStHub(char *hubMessage)
{
	GATEWAY_DEBUG(PSTR("GWT:MSG:STH sendToStHub incoming message: %s\n"), hubMessage);

	if (httpClient.connect(_ethernetControllerIP, MY_PORT))
	{
		GATEWAY_DEBUG(PSTR("GWT:MSG:STH POST to hub\n"));

		httpClient.println("POST / HTTP/1.1");
		httpClient.print("HOST: ");
		httpClient.print(_ethernetControllerIP);
		httpClient.print(":");
		httpClient.println(MY_PORT);
		httpClient.println("CONTENT-TYPE: text");
		httpClient.print("CONTENT-LENGTH: ");
		httpClient.println(strlen(hubMessage));
		httpClient.println();
		httpClient.println(hubMessage);

		// read the hub response
		while (httpClient.connected())
		{
			char c = httpClient.read();
			//read by char HTTP response
			readString += c;

			// if you've gotten to the end of the line (received a newline
			// character) and the line is blank, the http request has ended,
			// so you can send a reply
			if (c == '\n')
			{
				// you're starting a new line
				currentLineIsBlank = true;
			}
			else if (c != '\r')
			{
				// you've gotten a character on the current line
				currentLineIsBlank = false;
			}

		}
		GATEWAY_DEBUG(PSTR("GWT:MSG:STH hub response %s\n"), readString);
   		httpClient.stop();
	}
	else
	{
		GATEWAY_DEBUG(PSTR("!GWT:MSG:STH not connected to hub\n"));
	}
}

bool gatewayTransportInit(void)
{
	_w5100_spi_en(true);

	Ethernet.begin(_ethernetGatewayMAC, _ethernetGatewayIP);
	_ethernetServer.begin();

	_w5100_spi_en(false);
	return true;
}

// send to controller
bool gatewayTransportSend(MyMessage &message)
{
	int nbytes = 0;
	char *_ethernetMsg = protocolFormat(message);

	GATEWAY_DEBUG(PSTR("GWT:MSG:ECF gatewayTransportSend\n"));
	GATEWAY_DEBUG(PSTR("GWT:MSG:ECF incoming message %s"), _ethernetMsg);

	setIndication(INDICATION_GW_TX);

	_w5100_spi_en(true);

/*
	if (!client.connected())
	{
		client.stop();
		if (client.connect(_ethernetControllerIP, MY_PORT))
		{
			GATEWAY_DEBUG(PSTR("GWT:TPS:ETH OK\n"));
			_w5100_spi_en(false);
			GATEWAY_DEBUG(PSTR("GWT:MSG:ECF gatewayTransportSend #1\n"));
			gatewayTransportSend(buildGw(_msgTmp, I_GATEWAY_READY).set(MSG_GW_STARTUP_COMPLETE));
			_w5100_spi_en(true);
			presentNode();
		}
		else
		{
			// connecting to the server failed!
			GATEWAY_DEBUG(PSTR("!GWT:TPS:ETH FAIL\n"));
			_w5100_spi_en(false);
			return false;
		}
	}
*/
	sendToStHub(_ethernetMsg);
	_w5100_spi_en(false);
	return (nbytes > 0);
}

// receive request from controller
bool _readFromClient(uint8_t i)
{
	clients[i] = _ethernetServer.available();
	
	while (clients[i].connected() && clients[i].available())
	{
		const char inChar = clients[i].read();
		if (inputString[i].idx < MY_GATEWAY_MAX_RECEIVE_LENGTH - 1)
		{
			// if newline then command is complete
			if (inChar == '\n' || inChar == '\r')
			{
				// Add string terminator and prepare for the next message
				inputString[i].string[inputString[i].idx] = 0;
				GATEWAY_DEBUG(PSTR("GWT:RFC:C=%" PRIu8 ",MSG=%s\n"), i, inputString[i].string);
				inputString[i].idx = 0;
				if (protocolParse(_ethernetMsg, inputString[i].string))
				{
					return true;
				}
			}
			else
			{
				// add it to the inputString:
				inputString[i].string[inputString[i].idx++] = inChar;
			}
		}
		else
		{
			// Incoming message too long. Throw away
			GATEWAY_DEBUG(PSTR("!GWT:RFC:C=%" PRIu8 ",MSG TOO LONG\n"), i);
			inputString[i].idx = 0;
			// Finished with this client's message. Next loop() we'll see if there's more to read.
			break;
		}
	}
	return false;
}

bool gatewayTransportAvailable(void)
{
	_w5100_spi_en(true);

/*
	if (!client.connected())
	{
		client.stop();
		if (client.connect(_ethernetControllerIP, MY_PORT))
		{
			GATEWAY_DEBUG(PSTR("GWT:TSA:ETH OK\n"));
			_w5100_spi_en(false);
			GATEWAY_DEBUG(PSTR("GWT:MSG:ECF gatewayTransportSend #3\n"));
			gatewayTransportSend(buildGw(_msgTmp, I_GATEWAY_READY).set(MSG_GW_STARTUP_COMPLETE));
			_w5100_spi_en(true);
			presentNode();
		}
		else
		{
			GATEWAY_DEBUG(PSTR("!GWT:TSA:ETH FAIL\n"));
			_w5100_spi_en(false);
			return false;
		}
	}
*/

	if (_readFromClient())
	{
		setIndication(INDICATION_GW_RX);
		_w5100_spi_en(false);
		return true;
	}
	_w5100_spi_en(false);
	return false;
}

MyMessage &gatewayTransportReceive(void)
{
	// Return the last parsed message
	return _ethernetMsg;
}
