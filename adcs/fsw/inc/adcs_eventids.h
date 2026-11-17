/*
CFE 이벤트 로깅 시스템용 이벤트 ID (0~114)
초기화
, 명령 에러
, I2C 에러
, 부팅 체크 등
*/

/************************************************************************
**
**      GSC-18128-1, "Core Flight Executive Version 6.6"
**
**      Copyright (c) 2006-2019 United States Government as represented by
**      the Administrator of the National Aeronautics and Space Administration.
**      All Rights Reserved.
**
**      Licensed under the Apache License, Version 2.0 (the "License");
**      you may not use this file except in compliance with the License.
**      You may obtain a copy of the License at
**
**        http://www.apache.org/licenses/LICENSE-2.0
**
**      Unless required by applicable law or agreed to in writing, software
**      distributed under the License is distributed on an "AS IS" BASIS,
**      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**      See the License for the specific language governing permissions and
**      limitations under the License.
**
** File:
**    adcs_app_events.h 
**
** Purpose: 
**  Define ADCS App Events IDs
**
** Notes:
**
**
*************************************************************************/
#ifndef _ADCS_EVENTS_H_
#define _ADCS_EVENTS_H_


#define ADCS_RESERVED_EID      0
#define ADCS_INIT_INF_EID      1
#define ADCS_CC_ERR_EID        2
#define ADCS_NOOP_INF_EID      3
#define ADCS_RESET_INF_EID     4
#define ADCS_MID_ERR_EID       5
#define ADCS_CMD_LEN_ERR_EID   6
#define ADCS_PIPE_ERR_EID      7
#define ADCS_VALUE_INF_EID     8
#define ADCS_CR_PIPE_ERR_EID   9
#define ADCS_SUB_HK_ERR_EID    10
#define ADCS_SUB_CMD_ERR_EID   11
#define ADCS_TABLE_REG_ERR_EID 12

#define ADCS_STARTUP_INF_EID       14
#define ADCS_COMMAND_ERR_EID       15
#define ADCS_COMMANDNOP_INF_EID    16
#define ADCS_COMMANDRST_INF_EID    17
#define ADCS_CMD_ERR_EID            18
#define ADCS_INVALID_MSGID_ERR_EID  19
#define ADCS_LEN_ERR_EID            20
#define ADCS_I2C_INIT_ERR_EID       22
#define ADCS_BEACON_DATA_REQUEST_ERR_EID    23
#define ADCS_HOUSEKEEPING_DATA_REQUEST_ERR_EID 24
#define ADCS_ATTITUDE_ORBIT_DATA_REQUEST_ERR_EID 25
#define ADCS_ENABLE_OBC_ENABLE_PIN_ERR_EID 26
#define ADCS_DISABLE_OBC_ENABLE_PIN_ERR_EID 27
#define ADCS_ENABLE_OBC_BOOT_PIN_ERR_EID 28
#define ADCS_DISABLE_OBC_BOOT_PIN_ERR_EID 29

#define ADCS_BOOTUP_CHECK_ERR_EID          114

#endif /* _adcs_app_events_h_ */