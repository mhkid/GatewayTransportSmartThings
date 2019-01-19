Please visit www.mysensors.org for more information

Refer to the repo this was forked from for information about the [MySensors library](https://github.com/mysensors/MySensors)
This was created based on MySensors Library v2.3.2-alpha

## Description
This is a forked version of the MySensors library to handle integration with the SmartThings hub.  This is done through a
custom gateway transport (MyGatewayTransportSmartThings.cpp)

### Important Notes
* The transport was written for a gateway running on a Raspberry Pi 3.  This has not been tested or expected to work on other hardware.
* This is an Ethernet transport and uses the http protocol to communicate with the SmartThings hub.
* Currently the communication is only one way, from the gateway to the SmartThings hub.  In the past I have had communication working from the SmartThings hub to the gateway but not a Raspberry Pi.  Will eventually look into this but I believe it's probably because of the customer version of the Ethernet.h/EthernetServer.h for MySensors.
* For this to work on the hub SmartThings, device handlers must be installed/created through the SmartThings developer IDE (classic version).  That repo can be found on my [SmartThings repo](https://github.com/mhkid/IoT)
* This is still a work in progress but feel free to contribute or provide feedback.

## Usage
Below are the steps for setting this transport up for MySensors to SmartThings integration.  

Read the first step completely before building your Raspberry Pi gateway
1. [Install and build](https://www.mysensors.org/build/raspberry) a MySensors gateway on a Raspberry Pi keeping the following in mind:
..* Clone the MySensors repo as the instructions say, then clone this repo (git clone https://github.com/mhkid/MySensors.git STTransport --master) and just manually add/merge the files.  However, just be aware if you upgrade your MySensors version and this SmartThings transport repo hasn't been upgraded it may not work.
..* All the rest of the steps should be the same for setting up your Raspberry Pi gateway.

