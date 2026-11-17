/************************************************************************
 * NASA Docket No. GSC-18,719-1, and identified as “core Flight System: Bootes”
 *
 * Copyright (c) 2020 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/**
 * @file
 *
 * Main header file for the Adcs application
 */

#ifndef ADCS_H
#define ADCS_H

/*
** Required header files.
*/
#include "cfe.h"
#include "cfe_config.h"

#include "adcs_mission_cfg.h"
#include "adcs_platform_cfg.h"
#include "adcs_perfids.h"
#include "adcs_msgids.h"
#include "adcs_msg.h"

/************************************************************************
** Type Definitions
*************************************************************************/

/*
** Global Data
*/
typedef struct
{
    /*
    ** Command interface counters...
    */
    uint8 CmdCounter; // 성공한 명령 수 
    uint8 ErrCounter; // 실패한 명령 수

     /*

    /*
    ** Housekeeping telemetry packet...
    */
    ADCS_HkTlm_t HkTlm; // 주기적으로 지상국에 보고하는 상태 패킷
    ADCS_BcnTlm_t BcnTlm; // 통신 제한 시 사용하는 경량 상태 패킷

    /*
    ** Run Status variable used in the main processing loop
    */
    uint32 RunStatus; // 메인 루프 실행/종료 상태

    /*
    ** Operational data (not reported in housekeeping)...
    */
    CFE_SB_PipeId_t CommandPipe; // SW Bus 명령 수신 파이프 ID

    /*
    ** Initialization data (not reported in housekeeping)...
    */
    char   PipeName[CFE_MISSION_MAX_API_LEN];
    uint16 PipeDepth;

    CFE_ES_TaskId_t TaskId;

    uint8_t BootUpCheckCounter;

} ADCS_AppData_t;

/*
** Global data structure
*/
extern ADCS_AppData_t ADCS_AppData;

/****************************************************************************/
/*
** Local function prototypes.
**
** Note: Except for the entry point (ADCS_Main), these
**       functions are not called from any other source module.
*/
void         ADCS_AppMain(void);
CFE_Status_t ADCS_AppInit(void);

#endif /* ADCS_H */
