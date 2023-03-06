#ifndef MAINGROBALDEF_H
#define MAINGROBALDEF_H
#ifndef modBus60_t
typedef int16_t modBus60_t[60];
#endif

#ifndef modBusData_t
typedef struct
{
    uint32_t logTime;
    modBus60_t Data;
} modBusData_t;

typedef struct
{
  uint32_t IPADDRESS;
  uint32_t GATEWAY;
  uint32_t SUBNETMASK;
  uint32_t WEBSOCKETSERVER;
  uint32_t DNS1;
  uint32_t DNS2;
  uint32_t NTP_1;
  uint32_t NTP_2;
  uint16_t WEBSERVERPORT;
  bool ntpuse;
  uint16_t BAUDRATE;
  uint16_t Q_INTERVAL;

} nvsSystemSet;
#endif
#endif