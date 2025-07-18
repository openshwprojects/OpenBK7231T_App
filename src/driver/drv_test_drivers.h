#pragma once

#include "../httpserver/new_http.h"

void Test_Init();
void Test_RunQuickTick();
void Test_AppendInformationToHTTPIndexPage(http_request_t *request);

void Test_Power_Init(void);
void Test_Power_RunEverySecond(void);

void Test_LED_Driver_Init(void);
void Test_LED_Driver_RunEverySecond(void);
void Test_LED_Driver_OnChannelChanged(int ch, int value);


void Test_UART_Init();
void Test_UART_RunEverySecond();
void Test_UART_AppendInformationToHTTPIndexPage(http_request_t *request);

