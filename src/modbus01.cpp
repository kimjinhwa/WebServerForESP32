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
uint8_t data_ready = 0;
int modbusErrorCode=0 ;

uint32_t request_time;
ModbusClientRTU MB(Serial);
static char TAG[] = "MODBUS";

int telnet_write(const char *format, va_list ap);

extern modBusData_t modBusData;
extern  nvsSystemSet ipAddress_struct;
extern QueueHandle_t h_queue;
extern QueueHandle_t h_sendSocketQueue;
extern WiFiClient Client;
extern SemaphoreHandle_t xMutex;

const int16_t char_12_AlarmMask = 0x00 | BIT(0) | BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(8) | BIT(9) | BIT(10) | BIT(11) | BIT(12) | BIT(13) | BIT(14);
const int16_t char_13_AlarmMask = 0x00 | BIT(0) | BIT(1) | BIT(5) | BIT(6) | BIT(8);
const int16_t char_14_AlarmMask = 0b0111111111111110;
const int16_t char_15_AlarmMask = 0b0000011000000010;
struct structEvent
{
    int16_t char_12 = 0;
    int16_t char_13 = 0;
    int16_t char_14 = 0;
    int16_t char_15 = 0;
};
struct structEvent oldEvent; // = {00,00,00,00, 00};

/*
1	1200	bt: 1	충전기 과열 이상
2	1202	bt: 1	입력 M/C 닫힘	이벤트 전용 //1 : Close 0: OPEN
1	1203	bt: 1	충전기 퓨즈 이상
1	1205	bt: 1	밧데리 (+) 지락 발생
1	1206	bt: 1	밧데리 (-) 지락 발생
2	1207	bt: 1	릴레이 접점 리셋	이벤트 전용
1	1208	bt: 1	긴급 이상
2	1209	bt: 1	CB1 차단기 닫힘	이벤트 전용
1	1209	bt: 0	CB1 차단기 열림
1	1210	bt: 0	CB3 차단기 열림
2	1210	bt: 1	CB3 차단기 닫힘	이벤트 전용
1	1212	bt: 1	충전기 R상 GDU 이상
1	1213	bt: 1	충전기 S상 GDU 이상
1	1214	bt: 1	충전기 T상 GDU 이상
*/
void logwrite_event()
{
    time_t nowTime;
    modBusData.logTime = time(&nowTime);
    if (Client.connected())
        Client.println("logwrite_event in");
    FILE *fp = fopen("/spiffs/logFile.dat", "a+");
    fwrite((byte *)&modBusData, sizeof(byte), sizeof(modBusData_t), fp);
    fclose(fp);
    if (Client.connected())
        Client.println("logwrite_event out");
}
//}

void log_status_change()
{
    int16_t changed_bits_12 = oldEvent.char_12 ^ modBusData.Data[12];
    int16_t changed_bits_13 = oldEvent.char_13 ^ modBusData.Data[13];
    int16_t changed_bits_14 = oldEvent.char_14 ^ modBusData.Data[14];
    int16_t changed_bits_15 = oldEvent.char_15 ^ modBusData.Data[15];
    if(changed_bits_12 || changed_bits_13 || changed_bits_14 || changed_bits_15 ){
        logwrite_event();
    };
    // if(char_12_AlarmMask & changed_bits_12 ) //if changed bit is for alarm
    // {
    //   // send alarm messge to server
    // }

    // 경보이벤트가 발생 했는지 단순이벤트인지를 체크한다
    // changed_bits 가 셋팅 되었다면 값이 변경된것이다
    // 이 값을 mask해서 결정한다.
    oldEvent.char_12 = modBusData.Data[12];
    oldEvent.char_13 = modBusData.Data[13];
    oldEvent.char_14 = modBusData.Data[14];
    oldEvent.char_15 = modBusData.Data[15];
}

void handleData(ModbusMessage response, uint32_t token)
{
    // First value is on pos 3, after server ID, function code and length byte
    modbusErrorCode =0;
    response.get(1, data_ready); // function code
    uint16_t offs = 3;
    // if (Client.connected())
    //     Client.printf("data Received %d %d\r\n", data_ready, token);
    // Client.printf("\r\n");
    uint16_t readValue;
    if (data_ready == READ_INPUT_REGISTER)
    {
        xSemaphoreTake(xMutex, portMAX_DELAY);
        for (uint8_t i = 0; i < NUM_VALUES; ++i)
        {
            offs = response.get(offs, readValue);
            modBusData.Data[i] = readValue;
        }
        xSemaphoreGive(xMutex);
        log_status_change();
        // oldEvent.char_12 = modBusData.Data[12];
        // oldEvent.char_13 = modBusData.Data[13];
        // oldEvent.char_14 = modBusData.Data[14];
        // oldEvent.char_15 = modBusData.Data[15];
    }

    if (data_ready == WRITE_HOLD_REGISTER)
    {
        if (Client.connected())
        {
        }
    }
    request_time = token;
}
// Define an onError handler function to receive error responses
// Arguments are the error code returned and a user-supplied token to identify the causing request
void handleError(Error error, uint32_t token)
{
    // ModbusError wraps the error code and provides a readable error message for it
    ModbusError me(error);
    if (Client.connected())
        Client.printf("Error response: %02X - %s\r\n", (int)me, (const char *)me);
        {
           modbusErrorCode =  (int)me;
        }
    //  printf("Error response: %02X - %s\n", (int)me, (const char *)me);
    //ESP_LOGE(TAG, "Error response: %02X - %s\n", (int)me, (const char *)me);
}
void modBusRtuSetup()
{
    // stdout = funopen(NULL, NULL, &telnet_print, NULL, NULL);
    MB.onDataHandler(&handleData);
    MB.onErrorHandler(&handleError);
    MB.setTimeout(2000);
    MB.begin(Serial);
}
void WritHoldeRegister(int address,int len ){
   Error err = MB.addRequest((uint32_t)millis(), 1, WRITE_HOLD_REGISTER, address, len);
} 
void modbusRequest(void *parameter)
{
    // Issue the request
    // if (Client.connected())
    //     Client.printf("\r\nData send...");
    int LEDSTATUS = 1;
    modBusRtuSetup();
    int16_t rQuest[5];

    unsigned long previousmills = 0;
    int interval = 2000;
    interval = ipAddress_struct.Q_INTERVAL < 300? 300: ipAddress_struct.Q_INTERVAL;
    // time_t nowTime;
    unsigned long now;
    esp_log_set_vprintf(telnet_write);
    // oldEvent = {00,00,00,00, 00};
    while (1)
    {
        now = millis();
        interval = ipAddress_struct.Q_INTERVAL < 300? 300: ipAddress_struct.Q_INTERVAL;
        if (now - previousmills > interval)
        {
            data_ready = 0;
            Error err = MB.addRequest((uint32_t)millis(), 1, READ_INPUT_REGISTER, FIRST_REGISTER, NUM_VALUES);
            if (err != SUCCESS)
            {
                ModbusError me(err);
            }
            // time(&nowTime);
            previousmills = now;
            digitalWrite(33, LEDSTATUS = !LEDSTATUS);

            int16_t qSocketSendRequest[5];
            memset(qSocketSendRequest, 0x00, 5);
            qSocketSendRequest[0] = 0x06;
            qSocketSendRequest[1] = FIRST_REGISTER;
            qSocketSendRequest[2] = NUM_VALUES;
            xQueueSend(h_sendSocketQueue, &qSocketSendRequest, (TickType_t)0);
        };
        if (xQueueReceive(h_queue, &rQuest, (TickType_t)5))
        {
            data_ready = 0;
            Error err = MB.addRequest((uint32_t)millis(), 1, WRITE_HOLD_REGISTER, rQuest[1], rQuest[2]);
            // vTaskDelay(100);
            //err = MB.addRequest((uint32_t)millis(), 1, READ_INPUT_REGISTER, FIRST_REGISTER, NUM_VALUES);
        }
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

//   Serial.printf("Use the shown IP and port %d to send requests!\n", port);

// // Your output on the Serial monitor should start with:
// //      __ OK __
// //      .IP address: 192.168.178.74
// //      [N] 1324| ModbusServer.cpp     [ 127] listServer: Server   4:  00 06
// //      [N] 1324| ModbusServer.cpp     [ 127] listServer: Server   5:  03 04
// //      Use the shown IP and port 502 to send requests!
// }
// for (int i = 0; i < 16; i++)
// {
//     if (!(changed_bits & (1 << i)))
//         continue;

//     switch (i)
//     {
//     case 0:
//         if (modBusData.Data[12] & (1 << i))
//         {
//             logwrite_alarm(); // "컨버터 과온"},/*(0: 정상 , 1: 과온경보)	경보, 이벤트*/
//         }
//         else
//         {
//         }
//         break;
//     case 2:
//         if (modBusData.Data[12] & (1 << i)) //"	입력 MC 상태"},/*(0:열림, 1:닫힘)	이벤트만*/
//         {
//         }
//         else
//         {
//         }
//         break;
//     case 3:
//         if (modBusData.Data[12] & (1 << i)) //"	Main FUSE Fail"},/*(0: 정상 , 1: 이상)	경보, 이벤트*/
//         {
//             logwrite_alarm();
//         }
//         else
//         {
//         }
//         break;
//     case 12:
//         if (!(modBusData.Data[12] & (1 << i)))
//         {
//             logwrite_alarm("Bit 12 changed to 0");
//         }
//         break;
//     }
// }
// int16_t char_16 = 0;
// int16_t char_12_eventMask = 0x00 | CHARGER_HIGH_TEMP_ALARM | CHARGET_FUSE_ALARM | BAT_EARTH_P_ALARM | BAT_EARTH_N_ALARM |  EMERGENCY_ALARM | MCCB_CB1_CLOSE | MCCB_CB3_CLOSE | CHARGER_R_GDU_FAULT | CHARGER_S_GDU_FAULT | CHARGER_T_GDU_FAULT;
// int16_t char_13_eventMask = 0x00 | CHARGER_CB2_CLOSE | CHARGER_CB4_CLOSE | FAN_FAULT | CHARGER_OC_FAULT | CHARGER_OV_FAULT | BAT_UV_FAULT | OFFSET_FAULT;
// int16_t char_14_eventMask = 0x00 | CHARGER_SC_FAULT | BAT_OC_FAULT | BAT_OV_FAULT | BAT_UV_FAULTLIMIT | INPUT_PHASE_FAULT | INPUT_UV_FAULT | INPUT_OV_FAULT | INPUT_FREQ_FAULT | INPUT_SYNC_FAULT | UTILITY_LINE_FAULT | INPUT_MCCB_FAULT | LOST_POWER | RESTORE_POWER;
// int16_t char_15_eventMask = 0x00 | OUTPUT_OL_FAULT;

// int16_t char_12_AlarmMask = 0x00 | CHARGER_HIGH_TEMP_ALARM | INPUT_MC_CLOSE | CHARGET_FUSE_ALARM | BAT_EARTH_P_ALARM | BAT_EARTH_N_ALARM | EMERGENCY_ALARM | CHARGER_R_GDU_FAULT | CHARGER_S_GDU_FAULT | CHARGER_T_GDU_FAULT;
// int16_t char_13_AlarmMask = 0x00 | FAN_FAULT | CHARGER_OC_FAULT | CHARGER_OV_FAULT | BAT_UV_FAULT | OFFSET_FAULT;
// int16_t char_14_AlarmMask = 0x00 | CHARGER_SC_FAULT | BAT_OC_FAULT | BAT_OV_FAULT | BAT_UV_FAULTLIMIT | INPUT_PHASE_FAULT | INPUT_UV_FAULT | INPUT_OV_FAULT | INPUT_FREQ_FAULT | INPUT_SYNC_FAULT | UTILITY_LINE_FAULT | INPUT_MCCB_FAULT | LOST_POWER;
// int16_t char_15_AlarmMask = 0x00 | OUTPUT_OL_FAULT;
// // 1200
// #define CHARGER_HIGH_TEMP_ALARM BIT0 // 0:Normal ,1:High Temperature, ALARM_EVENT
// #define INPUT_MC_CLOSE          BIT2 // 1  trigger , evnet only
// #define CHARGET_FUSE_ALARM BIT3      // 1  trigger , evnet only
// #define BAT_EARTH_P_ALARM BIT5       // 1  trigger
// #define BAT_EARTH_N_ALARM BIT6       // 1  trigger
// #define EMERGENCY_ALARM BIT8         // 1  trigger
// #define MCCB_CB1_CLOSE BIT9          // 1 Close 0 oepn   1.Close Only Event
// #define MCCB_CB3_CLOSE BIT10         // 1 Close 0
// #define CHARGER_R_GDU_FAULT BIT12    // 1 trigger
// #define CHARGER_S_GDU_FAULT BIT13    // 1 trigger
// #define CHARGER_T_GDU_FAULT BIT14    // 1 trigger
// // 1300
// #define CHARGER_CB2_CLOSE BIT0 // 2	1300	bt: 1	CB2 차단기 닫힘	이벤트 전용
// #define CHARGER_CB4_CLOSE BIT1 // 1	//1301	bt: 0	CB4 차단기 열림
// #define FAN_FAULT BIT2         // 1	//1302	bt: 1	팬1 이상 발생
// #define CHARGER_OC_FAULT BIT5  // 1	//1305	bt: 1	충전기 과전류 이상
// #define CHARGER_OV_FAULT BIT6  // 1	//1306	bt: 1	축전지 과전압 이상
// #define BAT_UV_FAULT BIT8      // 1	//1308	bt: 1	축전지 저전압 이상
// // 1400
// #define CHARGER_SC_FAULT BIT1    // 1	1401	bt: 1	충전기 전류 제한 이상
// #define BAT_OC_FAULT BIT2        // 1	1402	bt: 1	축전지 전류 제한 이상
// #define BAT_OV_FAULT BIT3        // 1	1403	bt: 1	축전지 과전압 제한 이상
// #define BAT_UV_FAULTLIMIT BIT4   // 1	1404	bt: 1	축전지 저전압 제한 이상
// #define INPUT_PHASE_FAULT BIT5   // 1	1405	bt: 1	입력 상회전 이상
// #define INPUT_UV_FAULT BIT6      // 1	1406	bt: 1	입력 저전압 이상
// #define INPUT_OV_FAULT BIT7      // 1	1407	bt: 1	입력 과전압 이상
// #define POWER_MODULE_FAULT BIT8      // 1	1407	bt: 1	입력 과전압 이상
// #define INPUT_FREQ_FAULT BIT9    // 1	1409	bt: 1	입력 주파수 이상
// #define INPUT_SYNC_FAULT BIT10   // 1	1410	bt: 1	입력비동기 이상
// #define UTILITY_LINE_FAULT BIT11 // 1	1411	bt: 1	정전
// #define INPUT_MCCB_FAULT BIT12   // 1	1412	bt: 1	입력 M/C 이상
// #define LOST_POWER BIT13         // 1	1413	bt: 1	결상
// #define RESTORE_POWER BIT14      // 2	1414	bt: 1	복전	이벤트 전용
// // 1	1509	bt: 1	출력 과부하
// #define OUTPUT_OL_FAULT BIT1 //  1  Over Lord fault stop
// #define OUTPUT_OLT_LIMIT BIT9 /// 1  Over Load Time Limitation fault
// #define CHARGER_STATE_RUNorSTOP BIT10 /// 1 Charger Run/stop(0:stop, 1:RUN)

// extern WiFiClient Client;
//  static int telnet_print(void *cookie, const char *data, int size)
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