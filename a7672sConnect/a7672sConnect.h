/**
 * Part of a7672s-connect project subjected to terms of
 * MIT license agreement. A license file is distributed with
 * the project.
 * @Author: Ritesh Sharma
 * @Date: 3-1-2026
 * @Detail: a7672s-connect header file, contains declarations of  
 * methods for using simcom's a7672s 4g LTE+GNSS+BLE module. 
 */
#ifndef A7672S_CONNECT_H
#define A7672S_CONNECT_H

/**********************************INCLUDES***********************************/
#include <stdint.h>
#include <stdbool.h>

/***********************************MACROS************************************/
#ifndef   __weak
  #define __weak           __attribute__((weak))
#endif

/**********************************TYPEDEFS***********************************/
typedef enum{
	A7672S_STATE_HARD_RESET=0,
	A7672S_STATE_PWR_OFF,
	A7672S_STATE_PWR_ON,
	A7672S_STATE_NET_CONNECT,
	A7672S_STATE_HIVE_CONNECT,
	A7672S_STATE_HIVE_SUBSCRIBE,
	A7672S_STATE_HIVE_PUBLISH,
	A7672S_STATE_MAX
} a7672s_states_t;

typedef enum
{
	A7672S_GPS_STATE_POWER_OFF,
	A7672S_GPS_STATE_POWER_ON,
	A7672S_GPS_STATE_COLD_START,
	A7672S_GPS_STATE_GET_AGPS,
	A7672S_GPS_STATE_GET_GPS,
	A7672S_GPS_STATE_MAX
} a7672s_gps_states_t;

/******************************GLOBAL VARIABLES*******************************/

/*************************GLOBAL FUNCTION PROTOTYPES**************************/
/**
 * WEAK Functions
 * NOTE: These functions Should not be modified here,
 *          these Should be implemented in the user file
 */
void a7672s_delay_ms(uint32_t delaytime);
void a7672s_powerKey_Off(void);
void a7672s_powerKey_On(void);
void a7672s_resetKey_Off(void);
void a7672s_resetKey_On(void);
void a7672s_serial_send(const uint8_t* buff, size_t len);
char* a7672s_serial_receive(uint8_t* buff, size_t max_bytes, uint32_t timeout);

/* Modem connection API */
bool a7672s_modemStart(bool hardreset);
bool a7672s_modemStop(void);
bool a7672s_netConnect(const char* apn);
bool a7672s_bleConnect(void);
bool a7672s_gpsConnect(void);
bool a7672s_mqttConnect(const char* client_id, const char* url, int port, int keepalive, const char* user, const char* passwd);
bool a7672s_mqttPublish(const char* data, size_t len, const char* topic, int qos);
bool a7672s_mqttSubscribe(void);
bool a7672s_wsConnect(void);
int  a7672s_getSigQ(void);

#endif /* EOF */