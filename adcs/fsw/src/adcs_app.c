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
 * \file
 *   This file contains the source code for the Adcs App.
 */

/*
** Include Files:
*/
#include "adcs_app.h"
#include "adcs_cmds.h"
#include "adcs_utils.h"
#include "adcs_dispatch.h"
#include "adcs_tbl.h"

#include "adcs_eventids.h"

/*
** global data
*/
ADCS_AppData_t ADCS_AppData;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
/*                                                                            */
/* Application entry point and main process loop                              */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
void ADCS_AppMain(void)
{
    CFE_Status_t     status;
    CFE_SB_Buffer_t *SBBufPtr;

    /*
    ** Create the first Performance Log entry
    */
    CFE_ES_PerfLogEntry(ADCS_PERF_ID);

    /*
    ** Perform application-specific initialization
    ** If the Initialization fails, set the RunStatus to
    ** CFE_ES_RunStatus_APP_ERROR and the App will not enter the RunLoop
    */
    status = ADCS_AppInit();
    if (status != CFE_SUCCESS)
    {
        ADCS_AppData.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    /*
    ** Adcs App Runloop
    */
    while (CFE_ES_RunLoop(&ADCS_AppData.RunStatus) == true)
    {
        /*
        ** Performance Log Exit Stamp
        */
        CFE_ES_PerfLogExit(ADCS_PERF_ID);

        /* Pend on receipt of command packet */
        status = CFE_SB_ReceiveBuffer(&SBBufPtr, ADCS_AppData.CommandPipe, CFE_SB_PEND_FOREVER);

        /*
        ** Performance Log Entry Stamp
        */
        CFE_ES_PerfLogEntry(ADCS_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            ADCS_TaskPipe(SBBufPtr);
        }
        else
        {
            CFE_EVS_SendEvent(ADCS_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "ADCS APP: SB Pipe Read Error, App Will Exit");

            ADCS_AppData.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    /*
    ** Performance Log Exit Stamp
    */
    CFE_ES_PerfLogExit(ADCS_PERF_ID);

    CFE_ES_ExitApp(ADCS_AppData.RunStatus);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* Initialization                                                             */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t ADCS_AppInit(void)
{
    CFE_Status_t status;

    /* Zero out the global data structure */
    memset(&ADCS_AppData, 0, sizeof(ADCS_AppData));

    ADCS_AppData.RunStatus = CFE_ES_RunStatus_APP_RUN;

    /*
    ** Initialize app configuration data
    */
    ADCS_AppData.PipeDepth = ADCS_PIPE_DEPTH;

    strncpy(ADCS_AppData.PipeName, "ADCS_CMD_PIPE", sizeof(ADCS_AppData.PipeName));
    ADCS_AppData.PipeName[sizeof(ADCS_AppData.PipeName) - 1] = 0;

    /*
    ** Register the events
    */
    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);
    }
    else
    {
        /*
         ** Initialize housekeeping packet (clear user data area).
         */
        CFE_MSG_Init(CFE_MSG_PTR(ADCS_AppData.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(ADCS_HK_TLM_MID),
                     sizeof(ADCS_AppData.HkTlm));

        /*
         ** Initialize beacon packet (clear user data area).
         */
        CFE_MSG_Init(CFE_MSG_PTR(ADCS_AppData.BcnTlm.TelemetryHeader), CFE_SB_ValueToMsgId(ADCS_BCN_TLM_MID),
                     sizeof(ADCS_AppData.BcnTlm));

        /*
         ** Create Software Bus message pipe.
         */
        status = CFE_SB_CreatePipe(&ADCS_AppData.CommandPipe, ADCS_AppData.PipeDepth, ADCS_AppData.PipeName);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(ADCS_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Adcs App: Error creating SB Command Pipe, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to Housekeeping request commands
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(ADCS_SEND_HK_MID), ADCS_AppData.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(ADCS_SUB_HK_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Adcs App: Error Subscribing to HK request, RC = 0x%08lX", (unsigned long)status);
        }
    }

    // USER ADDED
    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to ADCS beacon command
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(ADCS_SEND_BCN_MID), ADCS_AppData.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(ADCS_SUB_HK_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Adcs App: Error Subscribing to BCN request, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to ground command packets
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(ADCS_CMD_MID), ADCS_AppData.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(ADCS_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Adcs App: Error Subscribing to Commands, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        /*
        ** Subscribe to LOOP by SCH
        */
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(ADCS_LOOP_MID), ADCS_AppData.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(ADCS_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Adcs App: Error Subscribing to Commands, RC = 0x%08lX", (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS) {
        // CAN Endpoint init
        CUBE_EndpointInit();
        /**
         * Create CubeADCS Event Listen Task...
         */
        status = CFE_ES_CreateChildTask(&ADCS_AppData.TaskId, "ADCS_EVS_TASK",
                                        ADCS_ListenEventTask, CFE_ES_TASK_STACK_ALLOCATE,
                                        ADCS_EVS_TASK_STACK_SIZE, ADCS_EVS_TASK_STACK_PRIORITY, 0);
    }

    if (status == CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(ADCS_INIT_INF_EID, CFE_EVS_EventType_INFORMATION, "ADCS App Succesfully Initialized");
    }

    return status;
}
