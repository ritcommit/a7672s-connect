/**
 * Part of a7672s-connect project subjected to terms of
 * MIT license agreement. A license file is distributed with
 * the project.
 * @Author: Ritesh Sharma
 * @Date: 3-1-2026
 * @Detail: a7672s-connect source file, contains methods for using
 * simcom's a7672s 4g LTE+GNSS+BLE module. Application layer code 
 * translating user's APIs to AT commands
 */
/**********************************INCLUDES***********************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "a7672sConnect.h"

/***********************************MACROS************************************/
#define A7672S_CMD_SIZE_MAX				    (64)
#define A7672S_RESP_SIZE_MAX 				(32)
#define A7672S_MQTT_CONNECT_CMD_SIZE        (128)
#define A7672S_MQTT_QOS                		(0)
#define A7672S_MQTT_CLEANSESSION       		(1)
#define MODEM_NL           				    "\r\n"
#define MODEM_OK						    "OK"
#define A7672S_APN							"update_with_your_apn"
#define CLIENT_ID                           "update_with_your_client_id"
#define IOT_RX_BUFFER_MAX                   (128)

/**********************************TYPEDEFS***********************************/
typedef bool (*a7672sHandler_t)(const char* cmd, const char* resp, uint32_t timeout); 
typedef void(*a7672s_state_cb_t)(void);

typedef enum{
	A7672S_NETCONN_INIT, /* check a7672s response */
	A7672S_NETCONN_E0, /* echo off */
	A7672S_NETCONN_CMEE, /* numeric error code enable */
	A7672S_NETCONN_CMFUN, /* set phone functionality */
	A7672S_NETCONN_CSQ, /* check signal quality */
	A7672S_NETCONN_CGDCONT, /* define PDP context */
	A7672S_NETCONN_CGDACT, /* activate PDP context */
	A7672S_NETCONN_CREG, /* set network registration */
	A7672S_NETCONN_CREGQ, /* query network registration */
	A7672S_NETCONN_DONE
} a7672s_nc_states_t;

typedef enum{
	A7672S_MQTTCONN_MQTTDISC=0,
	A7672S_MQTTCONN_MQTTREL,
	A7672S_MQTTCONN_MQTTSTOP,
	A7672S_MQTTCONN_MQTTSTART,
	A7672S_MQTTCONN_MQTTACCQ,
	A7672S_MQTTCONN_MQTTCFG,
	A7672S_MQTTCONN_DONE
} a7672s_mc_states_t;

typedef enum
{
	NOT_REGISTERED=0,
	REGISTERED_HOME,
	NOT_REGISTERED_SEARCHING,
	REGISTRATION_DENIED,
	UNKNOWN,
	REGISTERED_ROAMING,
	REGISTERED_SMS_ONLY_HOME
} a7672s_reg_stat_t;

typedef enum{
	A7672S_GPSCONN_GET_PWR=0,
	A7672S_GPSCONN_SET_PWR_HIGH,
	A7672S_GPSCONN_SET_PWR_LOW,
	A7672S_GPSCONN_COLD_START,
	A7672S_GPSCONN_GET_AGPS_DATA,
	A7672S_GPSCONN_GET_GNSS_DATA,
	A7672S_GPSCONN_DONE
} a7672s_gc_states_t;

typedef struct{
	char cmd[A7672S_CMD_SIZE_MAX];
	char resp[A7672S_RESP_SIZE_MAX];
	uint32_t timeout;
	a7672sHandler_t rx_handler;
} a7672s_cmdresp_t;

/*************************LOCAL FUNCTION PROTOTYPES***************************/
static bool A7672S_Bool_Handler(const char* cmd, const char* resp, uint32_t tout);
static bool A7672S_SignalQ_Handler(const char* cmd, const char* resp, uint32_t tout);
static bool A7672S_Creg_Handler(const char* cmd, const char* resp, uint32_t tout);
static bool A7672S_Gps_Handler(const char* cmd, const char* resp, uint32_t tout);
static char* A7672S_Receive_Response(const char* resp, uint32_t tout);
static void A7672S_Subscribe_Handler(void);
static int string_to_int(const char *str);
static void a7672s_softStart(void);
static void a7672s_hardReset(void);

/******************************GLOBAL VARIABLES*******************************/

/******************************LOCAL VARIABLES********************************/
static a7672s_states_t a7672s_4g_state = A7672S_STATE_PWR_OFF;
static a7672s_gps_states_t a7672s_gps_state = A7672S_GPS_STATE_POWER_OFF;
static char signal_quality_str[5] = {0};
static char receive_data[IOT_RX_BUFFER_MAX] = {0};

static a7672s_cmdresp_t netconnect[A7672S_NETCONN_DONE] = {
	{.cmd= "AT"MODEM_NL,.resp= MODEM_OK,.timeout= 1000U,.rx_handler= A7672S_Bool_Handler},
	{.cmd= "ATE0"MODEM_NL,.resp= MODEM_OK,.timeout= 1000U,.rx_handler= A7672S_Bool_Handler},
	{.cmd= "AT+CMEE=1"MODEM_NL,.resp= MODEM_OK,.timeout= 1000U,.rx_handler= A7672S_Bool_Handler},
	{.cmd= "AT+CFUN=1"MODEM_NL,.resp= MODEM_OK,.timeout= 1000U,.rx_handler= A7672S_Bool_Handler},
	{.cmd= "AT+CSQ"MODEM_NL,.resp= "+CSQ: ",.timeout= 1000U,.rx_handler= A7672S_SignalQ_Handler},
	{.cmd= "AT+CGDCONT=1,\"IP\",\""A7672S_APN"\""MODEM_NL,.resp= MODEM_OK,.timeout= 1000U,.rx_handler= A7672S_Bool_Handler},
	{.cmd= "AT+CGACT=1,1"MODEM_NL,.resp= MODEM_OK,.timeout= 1000U,.rx_handler= A7672S_Bool_Handler},
	{.cmd= "AT+CREG=1"MODEM_NL,.resp= MODEM_OK,.timeout= 1000U,.rx_handler= A7672S_Bool_Handler},
	{.cmd= "AT+CREG?"MODEM_NL,.resp= "+CREG: ",.timeout= 1000U,.rx_handler= A7672S_Creg_Handler}
};

static a7672s_cmdresp_t mqttconnect[A7672S_MQTTCONN_DONE] = {
	{.cmd= "AT+CMQTTDISC=0,120"MODEM_NL,.resp= "+CMQTTDISC: 0,0",.timeout= 1000U,.rx_handler= A7672S_Bool_Handler},
	{.cmd= "AT+CMQTTREL=0"MODEM_NL,.resp= MODEM_OK,.timeout= 1000U,.rx_handler= A7672S_Bool_Handler},
	{.cmd= "AT+CMQTTSTOP"MODEM_NL,.resp= "+CMQTTSTOP: 0",.timeout= 1000U,.rx_handler= A7672S_Bool_Handler},
	{.cmd= "AT+CMQTTSTART"MODEM_NL,.resp= "+CMQTTSTART: 0",.timeout= 1000U,.rx_handler= A7672S_Bool_Handler},
	{.cmd= "AT+CMQTTACCQ=0,\""CLIENT_ID"\",0"MODEM_NL,.resp= MODEM_OK,.timeout= 1000U,.rx_handler= A7672S_Bool_Handler},
	{.cmd= "AT+CMQTTCFG=\"argtopic\",0,1,1"MODEM_NL,.resp= MODEM_OK,.timeout= 1000U,.rx_handler= A7672S_Bool_Handler},
};

static a7672s_cmdresp_t gpsconnect[A7672S_GPSCONN_DONE] = {
	{.cmd= "AT+CGNSSPWR?"MODEM_NL,.resp= "+CGNSSPWR: 1",.timeout= 1000,.rx_handler= A7672S_Bool_Handler},
	{.cmd= "AT+CGNSSPWR=1,1,1"MODEM_NL,.resp= "+CGNSSPWR: READY!",.timeout= 12000,.rx_handler= A7672S_Bool_Handler},
	{.cmd= "AT+CGNSSPWR=0,1,1"MODEM_NL,.resp= MODEM_OK,.timeout= 1000,.rx_handler= A7672S_Bool_Handler},
	{.cmd= "AT+CGPSCOLD"MODEM_NL,.resp= MODEM_OK,.timeout= 1000,.rx_handler= A7672S_Bool_Handler},
	{.cmd= "AT+CAGPS"MODEM_NL,.resp= MODEM_OK,.timeout= 1000,.rx_handler= A7672S_Bool_Handler},
	{.cmd= "AT+CGNSSINFO"MODEM_NL,.resp= "+CGNSSINFO: ",.timeout= 9000,.rx_handler= A7672S_Gps_Handler},
};

/******************************GLOBAL FUNCTIONS*******************************/
__weak void a7672s_delay_ms(uint32_t delaytime)
{
    /* NOTE : This function Should not be modified here,
            this Should be implemented in the user file
   */
  (void)delaytime;
}

__weak void a7672s_powerKey_Off(void)
{
    /* NOTE : This function Should not be modified here,
            this Should be implemented in the user file
   */
}

__weak void a7672s_powerKey_On(void)
{
    /* NOTE : This function Should not be modified here,
            this Should be implemented in the user file
   */
}

__weak void a7672s_resetKey_Off(void)
{
    /* NOTE : This function Should not be modified here,
            this Should be implemented in the user file
   */
}

__weak void a7672s_resetKey_On(void)
{
    /* NOTE : This function Should not be modified here,
            this Should be implemented in the user file
   */
}

__weak void a7672s_serial_send(const uint8_t* buff, size_t len)
{
    /* NOTE : This function Should not be modified here,
            this Should be implemented in the user file
   */
    (void)buff;
    (void)len;
}

__weak uint32_t a7672s_serial_receive(uint8_t* buff, size_t max_bytes, uint32_t timeout)
{
    /* NOTE : This function Should not be modified here,
            this Should be implemented in the user file
   */
    (void)buff;
    (void)max_bytes;
    (void)timeout;
}

void a7672s_modemStart(bool hard_reset_b)
{
    if(true == hard_reset_b)
    {
        a7672s_hardReset();
    }
    a7672s_softStart();
}

void a7672s_modemStop(void)
{
    a7672s_powerKey_Off();
}

bool a7672s_netConnect(const char* apn)
{
    bool ret_val = false;
    uint8_t err_netconnect=0U;
    a7672s_nc_states_t nc_state = A7672S_NETCONN_INIT;

    snprintf(netconnect[A7672S_NETCONN_CGDCONT].cmd,
        A7672S_CMD_SIZE_MAX,
        "AT+CGDCONT=1,\"IP\",\"%s\""MODEM_NL,
        apn);

    while ((err_netconnect < 10U) && (nc_state < A7672S_NETCONN_DONE))
    {
        if (true == netconnect[nc_state].rx_handler(netconnect[nc_state].cmd, netconnect[nc_state].resp, netconnect[nc_state].timeout))
        {
            nc_state += 1U;
            err_netconnect = 0U;
        }
        else
        {
            err_netconnect += 1U;
        }
    }
    if (A7672S_NETCONN_DONE == nc_state)
    {
        ret_val = true;
    }
    return ret_val;
}

bool a7672s_bleConnect(void)
{
    /* TODO */
}

bool a7672s_gpsConnect(void)
{
    /* TODO */
}

bool a7672s_mqttConnect(const char* client_id, const char* url, int port, int keepalive, const char* user, const char* passwd)
{
    bool ret_val = false;
    uint8_t err_mqttconnect=0U;
    a7672s_mc_states_t mc_state = A7672S_MQTTCONN_MQTTDISC;
    char mqtt_connect_cmd[A7672S_MQTT_CONNECT_CMD_SIZE] = {0};
    char netconnect_apn_cmd[A7672S_CMD_SIZE_MAX] = {0};

    int cmd_len = snprintf(mqtt_connect_cmd,
        A7672S_MQTT_CONNECT_CMD_SIZE,
        "AT+CMQTTCONNECT=0,\"%s:%d\",%d,%d,\"%s\",\"%s\""MODEM_NL,
        url, port, keepalive, A7672S_MQTT_CLEANSESSION, user, passwd);

    snprintf(mqttconnect[A7672S_MQTTCONN_MQTTACCQ].cmd,
        A7672S_CMD_SIZE_MAX,
        "AT+CMQTTACCQ=0,\"%s\",0"MODEM_NL,
        client_id);
    
    while ((err_mqttconnect < 3U) && (mc_state < A7672S_MQTTCONN_DONE))
    {
        if (true == mqttconnect[mc_state].rx_handler(mqttconnect[mc_state].cmd, mqttconnect[mc_state].resp, mqttconnect[mc_state].timeout))
        {
            mc_state += 1U;
            err_mqttconnect = 0U;
        }
        else
        {
            if (mc_state > A7672S_MQTTCONN_MQTTSTOP)
            {
                err_mqttconnect += 1U;
            }
            else
            {
                mc_state += 1U;
            }
        }
    }

    if ((A7672S_MQTTCONN_DONE == mc_state) && ( true == A7672S_Bool_Handler(mqtt_connect_cmd, "+CMQTTCONNECT: 0,0", 9000U) ) )
    {
        ret_val = true;
    }
    return ret_val;
}

bool a7672s_mqttPublish(const char* data, size_t len, const char* topic, int qos)
{
    bool ret_val = false;
    static char publish_cmd[A7672S_CMD_SIZE_MAX] = {0};
    int cmd_len = snprintf(publish_cmd, A7672S_CMD_SIZE_MAX, "AT+CMQTTPUB=0,\"%s\",%d,%d" MODEM_NL, topic, qos, len);
    if (cmd_len < (A7672S_CMD_SIZE_MAX - 2))
    {
        if (true == A7672S_Bool_Handler(publish_cmd, ">", 1000U))
        {
            if (true == A7672S_Bool_Handler(data,  "+CMQTTPUB: 0,0", 1000U))
            {
                ret_val = true;
            }
        }
    }
    return ret_val;
}

bool a7672s_wsConnect(void)
{
    /* TODO */
}

int a7672s_getSigQ(void)
{
    a7672s_nc_states_t nc_state = A7672S_NETCONN_CSQ;
    if (true == netconnect[nc_state].rx_handler(netconnect[nc_state].cmd, netconnect[nc_state].resp, netconnect[nc_state].timeout))
    {
        return string_to_int(signal_quality_str);
    }
    return 0;
}

/*********************************LOCAL FUNCTIONS*****************************/
static void a7672s_softStart(void)
{
    a7672s_powerKey_Off();
    a7672s_delay_ms(4000);
    a7672s_powerKey_On();
    a7672s_delay_ms(4000);
}

static void a7672s_hardReset(void)
{
    a7672s_resetKey_Off();
    a7672s_delay_ms(3000);
    a7672s_resetKey_On();
    a7672s_delay_ms(6000);
}

static bool A7672S_Bool_Handler(const char* a7672s_cmd, const char* a7672s_resp, uint32_t a7672s_timeout)
{
	bool ret_val = false;
	a7672s_serial_send((const uint8_t*)a7672s_cmd, strlen(a7672s_cmd));
	if(A7672S_Receive_Response(a7672s_resp, a7672s_timeout) != NULL)
	{
		ret_val = true;
	}
	return ret_val;
}

static bool A7672S_SignalQ_Handler(const char* a7672s_cmd, const char* a7672s_resp, uint32_t a7672s_timeout)
{
	bool ret_val = false;
	a7672s_serial_send((const uint8_t*)a7672s_cmd, strlen(a7672s_cmd));
	char *index = A7672S_Receive_Response(a7672s_resp, a7672s_timeout);
	if ((index != NULL) && (strlen(index) > 6U))
	{
		char *signal_quality_ptr = strchr(index, ',');
		if (signal_quality_ptr != NULL)
		{
			uint32_t sig_len = signal_quality_ptr - &index[6];
			if (sig_len < sizeof(signal_quality_str))
			{
				if ( NULL != memcpy(signal_quality_str, &index[6], sig_len))
				{
					signal_quality_str[sig_len] = '\0';
				}
			}
		}
		if ((0 != strcmp(signal_quality_str, "0")) && (0 != strcmp(signal_quality_str, "99")))
		{
			ret_val = true;
		}
	}
	return ret_val;
}

static bool A7672S_Creg_Handler(const char* a7672s_cmd, const char* a7672s_resp, uint32_t a7672s_timeout)
{
	bool ret_val = false;
	a7672s_serial_send((const uint8_t*)a7672s_cmd, strlen(a7672s_cmd));
	char *index = A7672S_Receive_Response(a7672s_resp, a7672s_timeout);
	if ((index != NULL) && (strlen(index) > 9U))
	{
		a7672s_reg_stat_t network_stat = index[9] - '0';
		if ((network_stat == REGISTERED_HOME) || (network_stat == REGISTERED_ROAMING))
		{
			ret_val = true;
		}
	}
	return ret_val;
}

static bool A7672S_Gps_Handler(const char* cmd, const char* resp, uint32_t tout)
{
    (void)cmd;
    (void)resp;
    (void)tout;
    /* TODO */
}

static char* A7672S_Receive_Response(const char *a7672s_resp, uint32_t timeout)
{
	char *index = NULL;

    uint32_t bytes_rxd = a7672s_serial_receive((uint8_t *)(receive_data), (IOT_RX_BUFFER_MAX - 1U), timeout);
    receive_data[bytes_rxd] = '\0';
    index = strstr(receive_data, a7672s_resp);
	return index;
}

static int string_to_int(const char *str)
{
    int value = 0;
    int sign = 1;

    if (str == NULL)
    {
        return 0;
    }

    if (*str == '-')
    {
        sign = -1;
        str++;
    }
    else if (*str == '+')
    {
        str++;
    }

    while ((*str >= '0') && (*str <= '9'))
    {
        value = (value * 10) + (*str - '0');
        str++;
    }

    return value * sign;
}

/* EOF */