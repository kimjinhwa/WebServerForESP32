#include <Arduino.h>
#include <stdio.h>
#include <ETH.h>
#include <Ethernet.h>
#include <Wifi.h>
#include <WiFiClient.h>
#include "mainGrobaldef.h"
#include "modbus01.h"
// Modbus bridge include
// #include "ModbusBridgeWiFi.h"
// // Modbus RTU client include
#include "ModbusClientRTU.h"

#define FIRST_REGISTER 0x0000
#define NUM_VALUES 60
#define READ_INTERVAL 5000
bool data_ready = false;

uint32_t request_time;
ModbusClientRTU MB(Serial);

extern modBusData_t modBusData;

// extern WiFiClient Client;
//  static int writeToUdp(void *cookie, const char *data, int size)
//  {
//      // udp.broadcastTo(data, 1234);
//      if (Client.connected())
//          Client.printf(data);
//      return 0;
//  }
//  Create a ModbusRTU client instance
//  The RS485 module has no halfduplex, so the second parameter with the DE/RE pin is required!

// Define an onData handler function to receive the regular responses
// Arguments are received response message and the request's token
extern WiFiClient Client;
extern SemaphoreHandle_t xMutex;
void handleData(ModbusMessage response, uint32_t token)
{
    // First value is on pos 3, after server ID, function code and length byte
    uint16_t offs = 3;
    // The device has values all as IEEE754 float32 in two consecutive registers
    // Read the requested in a loop
    if (Client.connected())
        Client.printf("data Received\r\n");
    for (uint8_t i = 0; i < NUM_VALUES; ++i)
    {
        xSemaphoreTake(xMutex, portMAX_DELAY); 
        offs = response.get(offs, modBusData.Data[i]);
        xSemaphoreGive(xMutex);
        Client.printf("%x ",modBusData.Data[i]);
    }
    // Signal "data is complete"
    request_time = token;
    data_ready = true;
}
// Define an onError handler function to receive error responses
// Arguments are the error code returned and a user-supplied token to identify the causing request
void handleError(Error error, uint32_t token)
{
    // ModbusError wraps the error code and provides a readable error message for it
    ModbusError me(error);
    // if (Client.connected())
    //     Client.printf("Error response: %02X - %s\n", (int)me, (const char *)me);
    // printf("Error response: %02X - %s\n", (int)me, (const char *)me);
    //  LOG_E("Error response: %02X - %s\n", (int)me, (const char *)me);
}
void modBusRtuSetup()
{
    // stdout = funopen(NULL, NULL, &writeToUdp, NULL, NULL);
    MB.onDataHandler(&handleData);
    MB.onErrorHandler(&handleError);
    MB.setTimeout(2000);
    MB.begin();
}
void modbusRequest(void *parameter)
{
    // Issue the request
    // if (Client.connected())
    //     Client.printf("\r\nData send...");
    int LEDSTATUS=1;
    modBusRtuSetup();
    while (1)
    {
        data_ready = false;
        Error err = MB.addRequest((uint32_t)millis(), 1, READ_INPUT_REGISTER, FIRST_REGISTER, NUM_VALUES);
        if (err != SUCCESS)
        {
            ModbusError me(err);
            // LOG_E("Error response: %02X - %s\n", (int)me, (const char *)me);
        }
        vTaskDelay(1000);
        digitalWrite(33,LEDSTATUS= !LEDSTATUS);
    }
}
// uint16_t port = 502;                       // port of modbus server

// // // Create a ModbusRTU client instance
//  ModbusClientRTU MB(Serial);
//  ModbusBridgeWiFi MBbridge;

// void modBusBridgeSetup()
// {

//   if( !ETH.linkUp() ) return;
// // Set RTU Modbus message timeout to 2000ms
//   MB.setTimeout(2000);
// // Start ModbusRTU background task on core 1
//   MB.begin(1);

// // Define and start WiFi bridge
// // ServerID 4: Server with remote serverID 1, accessed through RTU client MB
// //             All FCs accepted, with the exception of FC 06
//   MBbridge.attachServer(1, 1, ANY_FUNCTION_CODE, &MB);

// // ServerID 5: Server with remote serverID 4, accessed through RTU client MB
// //             Only FCs 03 and 04 accepted
// //   MBbridge.attachServer(5, 4, READ_HOLD_REGISTER, &MB);
// //   MBbridge.addFunctionCode(5, READ_INPUT_REGISTER);

// // Check: print out all combinations served to Serial
//   MBbridge.listServer();

// // Start the bridge. Port 502, 4 simultaneous clients allowed, 600ms inactivity to disconnect client
//   MBbridge.start(port, 4, 600);

//   Serial.printf("Use the shown IP and port %d to send requests!\n", port);

// // Your output on the Serial monitor should start with:
// //      __ OK __
// //      .IP address: 192.168.178.74
// //      [N] 1324| ModbusServer.cpp     [ 127] listServer: Server   4:  00 06
// //      [N] 1324| ModbusServer.cpp     [ 127] listServer: Server   5:  03 04
// //      Use the shown IP and port 502 to send requests!
// }