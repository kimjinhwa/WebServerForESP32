#ifndef MAINGROBALDEF_H
#define MAINGROBALDEF_H
#ifndef modBus60_t
typedef int modBus60_t[60];
#endif

#ifndef modBusData_t
typedef struct
{
    uint32_t logTime;
    modBus60_t Data;
} modBusData_t;
#endif
#endif