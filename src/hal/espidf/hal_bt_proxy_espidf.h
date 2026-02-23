#pragma once

#if PLATFORM_ESPIDF

int HAL_BTProxy_GetScanStats(int* init_done, int* scan_active, int* total_packets, int* dropped_packets, int* buffered_packets);
int HAL_BTProxy_GetScanEntry(int newest_index, char* mac_buf, int mac_buf_len, int* rssi, int* adv_len, int* evt_type, int* age_ms);

#endif // PLATFORM_ESPIDF

