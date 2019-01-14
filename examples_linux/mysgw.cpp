/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2018 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <iostream>
#include <cstdio>
#include <unistd.h>

// For more options run ./configure --help

// Config file
//#define MY_LINUX_CONFIG_FILE "/etc/mysensors.conf"

// How many clients should be able to connect to this gateway (default 1)
#define MY_GATEWAY_MAX_CLIENTS 10

// Serial config
// Enable this if you are using an Arduino connected to the USB
//#define MY_LINUX_SERIAL_PORT "/dev/ttyUSB0"
// Enable this if you need to connect to a controller running on the same device
// You also need to define MY_LINUX_SERIAL_PORT above with the symlink name for the PTY device
//#define MY_LINUX_SERIAL_IS_PTY
// Grant access to the specified system group for the serial device
//#define MY_LINUX_SERIAL_GROUPNAME "tty"

// MQTT options
//#define MY_CONTROLLER_IP_ADDRESS 192, 168, 178, 68
//#define MY_PORT 1883
//#define MY_MQTT_CLIENT_ID "mysensors-1"
//#define MY_MQTT_PUBLISH_TOPIC_PREFIX "mygateway1-out"
//#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "mygateway1-in"

// Enable these if your MQTT broker requires username/password
//#define MY_MQTT_USER "username"
//#define MY_MQTT_PASSWORD "password"

// Flash leds on rx/tx/err
//#define MY_DEFAULT_ERR_LED_PIN 12  // Error LED pin
//#define MY_DEFAULT_RX_LED_PIN  16  // Receive LED pin
//#define MY_DEFAULT_TX_LED_PIN  18  // Transmit LED pin
// Inverse the blinking feature
//#define MY_WITH_LEDS_BLINKING_INVERSE

// Enable software signing
//#define MY_SIGNING_SOFT
// Enable signing related debug
//#define MY_DEBUG_VERBOSE_SIGNING
// Enable this to request signatures
//#define MY_SIGNING_REQUEST_SIGNATURES
// Enable this to to weaken security for gradual deployment purpose
// (see signing documentation for details)
//#define MY_SIGNING_WEAK_SECURITY

// Enables RF24 encryption (all nodes and gateway must have this enabled, and all must be
// personalized with the same AES key)
//#define MY_RF24_ENABLE_ENCRYPTION

// Enable inclusion mode if your HA Controller supports it (e.g. Vera Controller)
//#define MY_INCLUSION_MODE_FEATURE
// Enable Inclusion mode button on gateway
//#define MY_INCLUSION_BUTTON_FEATURE
// Set inclusion mode duration (in seconds)
//#define MY_INCLUSION_MODE_DURATION 60
// Digital pin used for inclusion mode button
//#define MY_INCLUSION_MODE_BUTTON_PIN  3

#include <MySensors.h>

#define ARDUINO 100
// This space is intended to be used to include arduino libraries

#undef ARDUINO

EthernetClient httpClient;

void setup()
{
	// Setup locally attached sensors
}

void presentation()
{
	// Present locally attached sensors here
}

void loop()
{
	// Send locally attached sensors data here
}

void SendToHub(const MyMessage &msg)
{
	char convBuf[61];
	char body[61];

	// TODO remove hard coding
	IPAddress hubIp(192, 168, 1, 3);
	int hubPort = 39500;

  	String readString;
  	//bool currentLineIsBlank = true;
	String hubMsg;

	// store command to an 8 bit unsigned integer
	// this will allow it to be tested and only send
	// certain commands to the hub
	uint8_t command = msg.getCommand();

	// put logic here to only send specified commnads to the hub

	// convert to a ; delimited string to send to the hub
	sprintf(body, "%d;%d;%d;%d;%d;%s\n", msg.sender, msg.sensor, command, msg.isAck(), msg.type, msg.getString(convBuf));

	hubMsg = body;

	GATEWAY_DEBUG(PSTR("GWT:MSG:ECF hubMsg %s"), hubMsg);

	if (httpClient.connect(hubIp, hubPort))
	{
		// send message to hub
		GATEWAY_DEBUG(PSTR("GWT:MSG:ECF SendToHub Connected to server\n"));
		httpClient.println("POST / HTTP/1.1");
		httpClient.print("HOST: ");
		httpClient.print(hubIp);
		httpClient.print(":");
		httpClient.println(hubPort);
		httpClient.println("CONTENT-TYPE: text");
		httpClient.print("CONTENT-LENGTH: ");
		httpClient.println(hubMsg.length());
		httpClient.println();
		httpClient.println(hubMsg);
/*
		// read hub response
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

		Serial.print("Hub Response: ");
		Serial.println(readString);
*/
		httpClient.stop();
		GATEWAY_DEBUG(PSTR("GWT:MSG:ECF httpClient stopped\n"));
	}
	else
	{
		GATEWAY_DEBUG(PSTR("!GWT:MSG:ECF Not connected to hub\n"));
	}
}

void receive(const MyMessage &msg) 
{
	GATEWAY_DEBUG(PSTR("GWT:MSG:ECF receive started\n"));
	// Send it to SmartThings hub
	SendToHub(msg);
}

