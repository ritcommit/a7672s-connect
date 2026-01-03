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

/*****************INCLUDES****************/
#include <stdint.h>
#include <stdbool.h>

/*****************GLOBAL FUNCTION PROTOTYPES****************/
bool a7672s_start();
bool a7672s_stop();
bool a7672s_softRestart();
bool a7672s_hardRestart();
bool a7672s_netConnect();
bool a7672s_bleConnect();
bool a7672s_gpsConnect();
bool a7672s_mqttConnect();
bool a7672s_mqttPublish();
bool a7672s_mqttSubscribe();
bool a7672s_wsConnect();

#endif /* EOF */