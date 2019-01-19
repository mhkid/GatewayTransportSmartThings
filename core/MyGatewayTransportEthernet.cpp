/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Eric Frame <eric.c.frame@gmail.com>
 * Based on the MyGatewayTransporEthernet as noted below.
 * Some things to note:
 *   - Intended to work with a SmartThings hub
 *   - Assumes gateway is a raspberry pi 3
 *   - Assumes my_controller_address is defined as described in raspberry pi setup
 *     as described here: https://www.mysensors.org/build/raspberry
 *   - Assumes DHCP is not used
 *   - Compiler directives and defines were removed from original transport code
 *     for reability purposes and also because of the assumption this is running
 *     on a pi.  Feel free to adapt for other platforms or more widely adaptable for
 *     other hardware.
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

EthernetServer _ethernetServer(80, MY_GATEWAY_MAX_CLIENTS);

static inputBuffer inputString;
static EthernetClient client = EthernetClient();
static EthernetClient httpClient;
static bool currentLineIsBlank = true;


// On W5100 boards with SPI_EN exposed we can use the real SPI bus together with radio
// (if we enable it during usage)
// NOTE: W5100 not used on pi, but left in because it was in the original ethernet MyGatewayTransporEthernet.cppcheck
//       when a W5100 was not used.
#define _w5100_spi_en(x)

bool gatewayTransportInit(void)
{
	// run when the gateway is started
	_w5100_spi_en(true);

	GATEWAY_DEBUG(PSTR("GWT:TIN:STT Initializing\n"));
    _ethernetServer.begin(_ethernetGatewayIP);
	GATEWAY_DEBUG(PSTR("GWT:TIN:STT Pi server started\n"));
	_w5100_spi_en(false);
	gatewayTransportSend(buildGw(_msgTmp, I_GATEWAY_READY).set(MSG_GW_STARTUP_COMPLETE));
	_w5100_spi_en(true);
	presentNode();

	_w5100_spi_en(false);
	return true;
}

bool gatewayTransportSend(MyMessage &message)
{
	sprintf(hubMsg, "%d;%d;%d;%d;%d;%s\n",message.sender, message.sensor, command, message.isAck(), message.type, message.getString(convBuf));
	
	// sends to the hub
	char *_ethernetMsg = protocolFormat(message);
    String readString;
	char convBuf[61];
  	char hubMsg[61];
  	uint8_t command = message.getCommand();


	GATEWAY_DEBUG(PSTR("GWT:TPS:STT Connecting to hub %s\n"), _ethernetMsg);
    sprintf(hubMsg, "%d;%d;%d;%d;%d;%s\n",message.sender, message.sensor, command, message.isAck(), message.type, message.getString(convBuf));
	GATEWAY_DEBUG(PSTR("GWT:TPS:STT hubMsg %s\n"), hubMsg);

	setIndication(INDICATION_GW_TX);

    if (httpClient.connect(_ethernetControllerIP, _ethernetGatewayPort))
    {
        GATEWAY_DEBUG(PSTR("GWT:TPS:STT Connected to hub\n"));

        httpClient.println("POST / HTTP/1.1");
        httpClient.print("HOST: ");
        httpClient.print(_ethernetControllerIP);
        httpClient.print(":");
        httpClient.println(_ethernetGatewayPort);
        httpClient.println("CONTENT-TYPE: text");
        httpClient.print("CONTENT-LENGTH: ");
        httpClient.println(strlen(_ethernetMsg));
        httpClient.println();
        httpClient.println(_ethernetMsg);
        Serial.println(_ethernetMsg);

        // read hub response
        while (httpClient.connected()) {
            char c = httpClient.read();
            //read by char HTTP response
            readString += c;
          
            // if you've gotten to the end of the line (received a newline
            // character) and the line is blank, the http request has ended,
            // so you can send a reply
            if (c == '\n') {
                // you're starting a new line
                currentLineIsBlank = true;
            }
            else if (c != '\r') {
                // you've gotten a character on the current line
                currentLineIsBlank = false;
            }
        }

        GATEWAY_DEBUG(PSTR("GWT:TPS:STT Hub response: \n"));
        GATEWAY_DEBUG(PSTR("%s\n"), readString);
        httpClient.stop();
		GATEWAY_DEBUG(PSTR("GWT:TPS:STT Connection closed\n"));
    }
    else 
    {
        GATEWAY_DEBUG(PSTR("!GWT:TPS:STT Failed to connect to hub\n"));
    }

	return (strlen(_ethernetMsg) > 0);
}

bool _readFromClient(void)
{
// This is where an http request would be received by the webserver hosted on the raspberry pi.
// This isn't currently working and will need to look into why it's not receiving client requests correctly.
// Could be because there is a custom ethernet.h for the linux version of MySensors.  Will look into
// this in the future.  This means that this gateway transport can only send requests to the SmartThings hub
// and can't process incoming requests from SmartThings.  This works fine on an ESP8266 and an UNO
// with a W5100.  For now just always returning a true until this is fixed.  
	/*
	client = _ethernetServer.available();
	if (client)
	{
		GATEWAY_DEBUG(PSTR("GWT:TPS:STT Client created\n"));
		// reads an incoming request
		while (client.connected())
		{
			if (client.available())
			{
				const char inChar = client.read();
				// if newline then command is complete
				if (inChar == '\n' && currentLineIsBlank)
				{
					// send a standard http response header
					client.println("HTTP/1.1 200 OK");
					client.println("Content-Type: text/html");
					client.println("Connection: close");
					client.println();
					// send web page
					// for now just sending back a webpage
					client.println("<!DOCTYPE html>");
					client.println("<html>");
					client.println("<head>");
					client.println("<title>MySensors Gateway</title>");
					client.println("</head>");
					client.println("<body>");
					client.println("<h1>MySensors Gateway</h1>");
					client.println("<p>Connected</p>");
					client.println("</body>");
					client.println("</html>");
					break;

					// TODO leave this here as reminder to process incoming messages
					// Add string terminator and prepare for the next message
					//inputString.string[inputString.idx] = 0;
					//GATEWAY_DEBUG(PSTR("GWT:RFC:MSG=%s\n"), inputString.string);
					//inputString.idx = 0;
					//if (protocolParse(_ethernetMsg, inputString.string))
					//{
					//	return true;
					//}
				}
				// every line of text received from the client ends with \r\n
				if (inChar == '\n')
				{
					// last character on line of received text
					// starting new line with next character read
					currentLineIsBlank = true;
				}
				else if (inChar != '\r')
				{
					// a text character was received from client
					currentLineIsBlank = false;
					// add it to the inputString:
					inputString.string[inputString.idx++] = inChar;
				}
			}
		}
		delay(1);	  // give the web browser time to receive the data
		client.stop(); // close the connection
	}
	*/

	return true;
}

bool gatewayTransportAvailable(void)
{
	if (_readFromClient()) {
		setIndication(INDICATION_GW_RX);
		_w5100_spi_en(false);
		return true;
	}

	return true;

    /*
	_w5100_spi_en(true);

	if (!client.connected()) {
		client.stop();
		if (client.connect(_ethernetControllerIP, MY_PORT)) {
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
	*/

}

MyMessage& gatewayTransportReceive(void)
{
	// Return the last parsed message
	return _ethernetMsg;
}
