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

IPAddress _ethernetControllerIP(MY_CONTROLLER_IP_ADDRESS);

IPAddress _ethernetGatewayIP(MY_IP_ADDRESS);

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

EthernetServer _ethernetServer(_ethernetGatewayPort, MY_GATEWAY_MAX_CLIENTS);

static inputBuffer inputString;
static EthernetClient client = EthernetClient();

#ifndef MY_IP_ADDRESS
void gatewayTransportRenewIP();
#endif

// On W5100 boards with SPI_EN exposed we can use the real SPI bus together with radio
// (if we enable it during usage)
#define _w5100_spi_en(x)

bool gatewayTransportInit(void)
{
	_w5100_spi_en(true);


	if (client.connect(_ethernetControllerIP, MY_PORT)) {
		GATEWAY_DEBUG(PSTR("GWT:TIN:ETH OK\n"));
		_w5100_spi_en(false);
		gatewayTransportSend(buildGw(_msgTmp, I_GATEWAY_READY).set(MSG_GW_STARTUP_COMPLETE));
		_w5100_spi_en(true);
		presentNode();
	} else {
		client.stop();
		GATEWAY_DEBUG(PSTR("!GWT:TIN:ETH FAIL\n"));
	}

	_w5100_spi_en(false);
	return true;
}

bool gatewayTransportSend(MyMessage &message)
{
	int nbytes = 0;
	char *_ethernetMsg = protocolFormat(message);

	setIndication(INDICATION_GW_TX);

	_w5100_spi_en(true);
	if (!client.connected()) {
		client.stop();
		if (client.connect(_ethernetControllerIP, MY_PORT)) {
			GATEWAY_DEBUG(PSTR("GWT:TPS:ETH OK\n"));
			_w5100_spi_en(false);
			gatewayTransportSend(buildGw(_msgTmp, I_GATEWAY_READY).set(MSG_GW_STARTUP_COMPLETE));
			_w5100_spi_en(true);
			presentNode();
		} else {
			// connecting to the server failed!
			GATEWAY_DEBUG(PSTR("!GWT:TPS:ETH FAIL\n"));
			_w5100_spi_en(false);
			return false;
		}
	}
	nbytes = client.write((const uint8_t*)_ethernetMsg, strlen(_ethernetMsg));
	_w5100_spi_en(false);
	return (nbytes > 0);
}

bool _readFromClient(void)
{
	while (client.connected() && client.available()) {
		const char inChar = client.read();
		if (inputString.idx < MY_GATEWAY_MAX_RECEIVE_LENGTH - 1) {
			// if newline then command is complete
			if (inChar == '\n' || inChar == '\r') {
				// Add string terminator and prepare for the next message
				inputString.string[inputString.idx] = 0;
				GATEWAY_DEBUG(PSTR("GWT:RFC:MSG=%s\n"), inputString.string);
				inputString.idx = 0;
				if (protocolParse(_ethernetMsg, inputString.string)) {
					return true;
				}

			} else {
				// add it to the inputString:
				inputString.string[inputString.idx++] = inChar;
			}
		} else {
			// Incoming message too long. Throw away
			GATEWAY_DEBUG(PSTR("!GWT:RFC:MSG TOO LONG\n"));
			inputString.idx = 0;
			// Finished with this client's message. Next loop() we'll see if there's more to read.
			break;
		}
	}
	return false;
}

bool gatewayTransportAvailable(void)
{
	_w5100_spi_en(true);
#if !defined(MY_IP_ADDRESS) && defined(MY_GATEWAY_W5100)
	// renew IP address using DHCP
	gatewayTransportRenewIP();
#endif

	if (!client.connected()) {
		client.stop();
		#if defined(MY_CONTROLLER_URL_ADDRESS)
		if (client.connect(MY_CONTROLLER_URL_ADDRESS, MY_PORT)) {
		#else
		if (client.connect(_ethernetControllerIP, MY_PORT)) {
		#endif /* End of MY_CONTROLLER_URL_ADDRESS */
			GATEWAY_DEBUG(PSTR("GWT:TSA:ETH OK\n"));
			_w5100_spi_en(false);
			gatewayTransportSend(buildGw(_msgTmp, I_GATEWAY_READY).set(MSG_GW_STARTUP_COMPLETE));
			_w5100_spi_en(true);
			presentNode();
		} else {
			GATEWAY_DEBUG(PSTR("!GWT:TSA:ETH FAIL\n"));
			_w5100_spi_en(false);
			return false;
		}
	}
	if (_readFromClient()) {
		setIndication(INDICATION_GW_RX);
		_w5100_spi_en(false);
		return true;
	}
	_w5100_spi_en(false);
	return false;
}

MyMessage& gatewayTransportReceive(void)
{
	// Return the last parsed message
	return _ethernetMsg;
}
