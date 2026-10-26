#pragma once

// VKL060 segment LCD driver (soft I2C)
//
// Ported from src/lcd_lktmzl02.c in pvvx/ZigbeeTLc.

void VKL060_Init();
void VKL060_OnEverySecond();
void VKL060_AppendInformationToHTTPIndexPage(http_request_t *request, int bPreState);
void VKL060_StopDriver();
