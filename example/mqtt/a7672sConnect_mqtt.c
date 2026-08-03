/**
 * Part of a7672s-connect project subjected to terms of
 * MIT license agreement. A license file is distributed with
 * the project.
 * @Author: Ritesh Sharma
 * @Date: 3-1-2026
 * @Detail: a7672s-connect mqtt connection example source file, 
 * Contain example code to connect and publish subscribe to a 
 * mqtt broker
 */
/*****************INCLUDES****************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "a7672sConnect.h"
#include "ritJson.h"

#define MODEM_APN               "jionet"
#define MQTT_CLIENT             "example"
#define MQTT_URL                "mosquitto"
#define MQTT_PORT               1883
#define MQTT_KEEPALIVE          1800
#define MQTT_USER               "random user"
#define MQTT_PASSWORD           " "
#define MQTT_TOPIC              "random topic"
#define MQTT_QOS                0

typedef enum
{
    MODEM_RESET=0,
    MODEM_INIT,
    MODEM_NET_CONNECT,
    MODEM_MQTT_CONNECT,
    MODEM_MQTT_PUBLISH
} modem_state_t;
/*****************MAIN FUNCTION****************/
int main()
{
    /* data */
    uint8_t soc=50;
    int voltage=6550;
    char* batt_name="ABCDE";
    float current=-21.10;

    /* buffer, make sure size is sufficient */
    char buffer[512] = {0};

    /* start serialization */
    ritJson_Open(buffer);

    /**
     * add various json objects 
     * @parameters (output string buffer, json key, adress of value, data type)
     */

    ritJson_addObject(buffer, "SOC", &soc, RITJSON_TYPE_UINT8);
    ritJson_addObject(buffer, "VOLTAGE", &voltage, RITJSON_TYPE_INT);
    ritJson_addObject(buffer, "NAME", batt_name, RITJSON_TYPE_STRING);
    ritJson_addObject(buffer, "CURRENT", &current, RITJSON_TYPE_FLOAT);

    /* end serialization */
    ritJson_Close(buffer);

    /* print Json formatted serial buffer*/
    printf("ritJson Serialized JSON Data:\n%s\n", buffer);

    modem_state_t modem_state = MODEM_RESET;
    uint8_t error_count_u8 = 0U;
    uint8_t init_count_u8 = 0U;

    while(true)
    {
        switch(modem_state)
        {
            case MODEM_RESET:
                a7672s_modemStart(true);
                modem_state = MODEM_INIT;
                break;

            case MODEM_INIT:
                if (init_count_u8 < 3)
                {
                    init_count_u8 += 1U;
                    a7672s_modemStart(false);
                    modem_state = MODEM_NET_CONNECT;
                }
                else
                {
                    init_count_u8 = 0U;
                    modem_state = MODEM_RESET;
                }
                break;
            
            case MODEM_NETCONNECT:
                if (true == a7672s_netConnect(MODEM_APN))
                {
                    error_count_u8 = 0U;
                    modem_state = MODEM_MQTT_CONNECT;
                }
                else
                {
                    error_count_u8 += 1U;
                }
                break;
            
            case MODEM_MQTT_CONNECT:
                if (true == a7672s_netConnect(MQTT_CLIENT, MQTT_URL, 
                    MQTT_PORT, MQTT_KEEPALIVE, MQTT_USER, MQTT_PASSWORD))
                {
                    error_count_u8 = 0U;
                    modem_state = MODEM_MQTT_PUBLISH;
                }
                else
                {
                    error_count_u8 += 1U;
                }
                break;
            
            case MODEM_MQTT_PUBLISH:
                if (true == a7672s_mqttPublish(buffer, strlen(buffer), 
                    MQTT_TOPIC, MQTT_QOS))
                {
                    error_count_u8 = 0U;
                }
                else
                {
                    error_count_u8 += 1U;
                }
                break;
        }

        if (error_count_u8 >= 10U)
        {
            error_count_u8 = 0U;
            modem_state = MODEM_INIT;
        }
    }
}