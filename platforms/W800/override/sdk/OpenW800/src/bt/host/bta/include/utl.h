/******************************************************************************
 *
 *  Copyright (C) 2003-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Basic utility functions.
 *
 ******************************************************************************/
#ifndef UTL_H
#define UTL_H

#include "bt_types.h"
#include "bt_utils.h"

/*****************************************************************************
**  Constants
*****************************************************************************/
/*** class of device settings ***/
#define BTA_UTL_SET_COD_MAJOR_MINOR     0x01
#define BTA_UTL_SET_COD_SERVICE_CLASS   0x02 /* only set the bits in the input */
#define BTA_UTL_CLR_COD_SERVICE_CLASS   0x04
#define BTA_UTL_SET_COD_ALL             0x08 /* take service class as the input (may clear some set bits!!) */
#define BTA_UTL_INIT_COD                0x0a

/*****************************************************************************
**  Type Definitions
*****************************************************************************/

/** for utl_set_device_class() **/
typedef struct {
    uint8_t       minor;
    uint8_t       major;
    uint16_t      service;
} tBTA_UTL_COD;


#ifdef __cplusplus
extern "C"
{
#endif

/*****************************************************************************
**  External Function Declarations
*****************************************************************************/

/*******************************************************************************
**
** Function         utl_str2int
**
** Description      This utility function converts a character string to an
**                  integer.  Acceptable values in string are 0-9.  If invalid
**                  string or string value too large, -1 is returned.
**
**
** Returns          Integer value or -1 on error.
**
*******************************************************************************/
extern int16_t utl_str2int(const char *p_s);

/*******************************************************************************
**
** Function         utl_strucmp
**
** Description      This utility function compares two strings in uppercase.
**                  String p_s must be uppercase.  String p_t is converted to
**                  uppercase if lowercase.  If p_s ends first, the substring
**                  match is counted as a match.
**
**
** Returns          0 if strings match, nonzero otherwise.
**
*******************************************************************************/
extern int utl_strucmp(const char *p_s, const char *p_t);

/*******************************************************************************
**
** Function         utl_itoa
**
** Description      This utility function converts a uint16_t to a string.  The
**                  string is NULL-terminated.  The length of the string is
**                  returned.
**
**
** Returns          Length of string.
**
*******************************************************************************/
extern uint8_t utl_itoa(uint16_t i, char *p_s);

/*******************************************************************************
**
** Function         utl_set_device_class
**
** Description      This function updates the local Device Class.
**
** Parameters:
**                  p_cod   - Pointer to the device class to set to
**
**                  cmd     - the fields of the device class to update.
**                            BTA_UTL_SET_COD_MAJOR_MINOR, - overwrite major, minor class
**                            BTA_UTL_SET_COD_SERVICE_CLASS - set the bits in the input
**                            BTA_UTL_CLR_COD_SERVICE_CLASS - clear the bits in the input
**                            BTA_UTL_SET_COD_ALL - overwrite major, minor, set the bits in service class
**                            BTA_UTL_INIT_COD - overwrite major, minor, and service class
**
** Returns          TRUE if successful, Otherwise FALSE
**
*******************************************************************************/
extern uint8_t utl_set_device_class(tBTA_UTL_COD *p_cod, uint8_t cmd);

/*******************************************************************************
**
** Function         utl_isintstr
**
** Description      This utility function checks if the given string is an
**                  integer string or not
**
**
** Returns          TRUE if successful, Otherwise FALSE
**
*******************************************************************************/
extern uint8_t utl_isintstr(const char *p_s);

/*******************************************************************************
**
** Function         utl_isdialstr
**
** Description      This utility function checks if the given string contains
**                  only dial digits or not
**
**
** Returns          TRUE if successful, Otherwise FALSE
**
*******************************************************************************/
extern uint8_t utl_isdialstr(const char *p_s);

#ifdef __cplusplus
}
#endif

#endif /* UTL_H */
