Please visit https://www.mysensors.org for more information on the MySensors library

Refer to the repo this was cloned from for information about the [MySensors library](https://github.com/mysensors/MySensors)
This was created based on MySensors Library v2.3.2-alpha in the develop branch.

## Description
This is a custom gateway transport for between MySensors and the SmartThings hub.

### Important Notes
* The transport was written for a gateway running on a Raspberry Pi 3.  This has not been tested to work on other hardware.
* This is an Ethernet transport and uses the http protocol to make a post call API call to your SmartThings hub.
* Currently the communication is only one way, from the gateway to the SmartThings hub.  In the past I have had communication working from the SmartThings hub to the gateway but not a Raspberry Pi.  Will eventually look into this but I believe it's probably because of the custom version of the Ethernet.h/EthernetServer.h for MySensors.
* For this to work on the hub SmartThings, device handlers must be installed/created through the SmartThings developer IDE (classic version).  That repo can be found on my [SmartThings repo](https://github.com/mhkid/IoT)
* This is still a work in progress but feel free to contribute or provide feedback.

## Usage
Below are the steps for setting this transport up for MySensors to SmartThings integration.  

1. [Install](https://www.mysensors.org/build/raspberry) a MySensors gateway on a Raspberry Pi keeping the following in mind:

  * Clone the MySensors repo as the instructions say, then clone this repo (*git clone https://github.com/mhkid/GatewayTransportSmartThings.git --master*)

  * All the rest of the steps should be the same for setting up your Raspberry Pi gateway.
  
2. Copy the files from this repo into the matching folders in the MySensors repo.  If you want to branch your MySensors repo first, go for it.  For example:

  * ../GatewayTransportSmartThings/core/MyGatewayTransportSmartThings.cpp  -> ../MySensors/core/
  * ../GatewayTransportSmartThings/linux-examples/mysgw.cpp  -> ../MySensors/linux-examples/
  * ../GatewayTransportSmartThings/configure  -> ../MySensors/
  * ../GatewayTransportSmartThings/MySensors.h  -> ..MySensors/
  
3. Edit the mysgw.cpp file and modify the following lines:
  
     *#define MY_SMARTTHINGS_ENABLED*
     *#define MY_IP_ADDRESS 192,168,1,41  // ip of raspberry pi gateway*
     
4. Continue on with the rest of the directions on the MySensors website for building your Raspberry Pi gateway.
