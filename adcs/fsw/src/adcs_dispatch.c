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
#include "adcs_dispatch.h"
#include "adcs_cmds.h"
#include "adcs_msgids.h"
#include "adcs_msg.h"
#include "adcs_utils.h"
#include "adcs_eventids.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* Verify command packet length                                               */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
bool ADCS_VerifyCmdLength(const CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
{
    bool              result       = true;
    size_t            ActualLength = 0;
    CFE_SB_MsgId_t    MsgId        = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_FcnCode_t FcnCode      = 0;

    CFE_MSG_GetSize(MsgPtr, &ActualLength);

    /*
    ** Verify the command packet length.
    */
    if (ExpectedLength != ActualLength)
    {
        CFE_MSG_GetMsgId(MsgPtr, &MsgId);
        CFE_MSG_GetFcnCode(MsgPtr, &FcnCode);

        CFE_EVS_SendEvent(ADCS_CMD_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Invalid Msg length: ID = 0x%X,  CC = %u, Len = %u, Expected = %u",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId), (unsigned int)FcnCode, (unsigned int)ActualLength,
                          (unsigned int)ExpectedLength);

        result = false;

        ADCS_AppData.ErrCounter++;

        /* RPT */
        ADCS_ReportTlm_t *BufPtr = (ADCS_ReportTlm_t *)CFE_SB_AllocateMessageBuffer(sizeof(ADCS_ReportTlm_t));
        if (BufPtr == NULL) goto cleanup;

        if (CFE_MSG_Init(CFE_MSG_PTR(BufPtr->TelemetryHeader), CFE_SB_ValueToMsgId(ADCS_REPORT_TLM_MID),
        sizeof(ADCS_ReportTlm_t)) != CFE_SUCCESS) {
            CFE_SB_ReleaseMessageBuffer((CFE_SB_Buffer_t *)BufPtr);
            goto cleanup;
        }
        BufPtr->Report.MsgID = (uint16_t)CFE_SB_MsgIdToValue(MsgId);
        BufPtr->Report.CommandCode = (uint8_t)FcnCode;
        BufPtr->Report.ReturnType = RPT_RETTYPE_APP;
        BufPtr->Report.ReturnCode = CFE_STATUS_WRONG_MSG_LENGTH; // Error code of `Length error`
        BufPtr->Report.ReturnDataSize = 2 * sizeof(uint32_t);
        
        uint32_t Temp32 = (uint32_t)ActualLength;
        memcpy(BufPtr->Report.ReturnValue, &Temp32, sizeof(uint32_t));
        Temp32 = (uint32_t)ExpectedLength;
        memcpy(BufPtr->Report.ReturnValue + sizeof(uint32_t), &Temp32, sizeof(uint32_t));

        CFE_SB_TimeStampMsg(CFE_MSG_PTR(BufPtr->TelemetryHeader));
        if (CFE_SB_TransmitBuffer((CFE_SB_Buffer_t *)BufPtr, true) != CFE_SUCCESS) {
            CFE_SB_ReleaseMessageBuffer((CFE_SB_Buffer_t *)BufPtr);
            goto cleanup;
        }
        /* End of RPT */
    }
cleanup:
    return result;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* ADCS ground commands                                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/

void ADCS_ProcessGroundCommand(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_MSG_FcnCode_t CommandCode = 0;

    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &CommandCode);

    /*
    ** Process ADCS app ground commands
    */
    switch (CommandCode)
    {
        case ADCS_NOOP_CC:
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_NoopCmd_t)))
            {
                ADCS_NoopCmd((const ADCS_NoopCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_RESET_COUNTERS_CC:
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_ResetCountersCmd_t)))
            {
                ADCS_ResetCountersCmd((const ADCS_ResetCountersCmd_t *)SBBufPtr);
            }
            break;
        
        /*
         * ADCS UTILS
        */
        case ADCS_GPIO_ENABLE_HIGH_CC:
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GpioEnHighCmd_t)))
            {
                // ADCS_EN_HighCmd();
            }            
            break;

        case ADCS_GPIO_ENABLE_LOW_CC:
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GpioEnLowCmd_t)))
            {
                // ADCS_EN_LowCmd();
            } 
            break;

        case ADCS_GPIO_BOOT_HIGH_CC:
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GpioBootHighCmd_t)))
            {
                // ADCS_Boot_HighCmd();
            } 
            break;
        
        case ADCS_GPIO_BOOT_LOW_CC:
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GpioBootLowCmd_t)))
            {
                // ADCS_Boot_LowCmd();
            } 
            break;

        case ADCS_EXIT_BOOTLOADER_CC:
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_ExitBootLoaderCmd_t)))
            {
                // ADCS_ExitBootloader();
            }
            break;

        /* * * * < ADCS Command Code for TC > * * * */
        case ADCS_SET_RESET_CC:
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_ResetCmd_t)))
            {
                ADCS_SetReset();
            }
            break;

        case ADCS_SET_CURRENT_UNIX_TIME_CC:
            // ID 2
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_CurrentUnixTimeCmd_t)))
            {
                ADCS_SetCurrentUnixTimeCmd((const ADCS_CurrentUnixTimeCmd_t *)SBBufPtr);
            }
            break;
        case ADCS_SET_ERROR_LOG_SETTING_CC:
            // ID 6
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_ErrorLogSettingCmd_t)))
            {
                ADCS_SetErrorLogSettingCmd((const ADCS_ErrorLogSettingCmd_t *)SBBufPtr);
            }
            break;            
        case ADCS_SET_PERSIST_CONFIG_CC:
            // ID 7
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_PersistConfigCmd_t))) {
                ADCS_SetPersistConfigCmd((const ADCS_PersistConfigCmd_t *)SBBufPtr);
            }
            break;
        
        case ADCS_SET_CONTROL_ESTIMATION_MODE_CC:
            // ID 42
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_ControlEstimationModeCmd_t))) {
                ADCS_SetControlEstimationModeCmd((const ADCS_ControlEstimationModeCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_DISABLE_MAG_RWL_MNT_MNG_CC:
            // ID 43
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_DisableMagRwlMntMngCmd_t))) {
                ADCS_SetDisableMagRwlMntMngCmd((const ADCS_DisableMagRwlMntMngCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_REFERENCE_IRC_VECTOR_CC:
            // ID 47
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_ReferenceIRCVectorCmd_t))) {
                ADCS_SetReferenceIRCVectorCmd((const ADCS_ReferenceIRCVectorCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_REFERENCE_LLH_TARGET_CC:
            // ID 48
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_ReferenceLLHTargetCmd_t))) {
                ADCS_SetReferenceLLHTargetCmd((const ADCS_ReferenceLLHTargetCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_ORBIT_MODE_CC:
            // ID 51
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_OrbitModeCmd_t))) {
                ADCS_SetOrbitModeCmd((const ADCS_OrbitModeCmd_t *)SBBufPtr);
            }
            break;
        
        case ADCS_SET_MAG_DEPLOY_CMD_CC:
            // ID 52
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_MagDeployCmd_t))) {
                ADCS_SetMagDeployCmd((const ADCS_MagDeployCmd_t *)SBBufPtr);
            }
            break;
        
        case ADCS_SET_REFERENCE_RPY_VALUES_CC:
            // ID 54
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_ReferenceRPYvaluesCmd_t))) {
                ADCS_SetReferenceRPYValuesCmd((const ADCS_ReferenceRPYvaluesCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_OPENLOOPCMD_MTQ_CC:
            // ID 55
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_OpenLoopCmdMTQCmd_t))) {
                ADCS_SetOpenLoopCmdMTQCmd((const ADCS_OpenLoopCmdMTQCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_POWER_STATE_CC:
            // ID 56
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_PowerStateCmd_t))) {
                ADCS_SetPowerStateCmd((const ADCS_PowerStateCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_RUN_MODE_CC:
            // ID 57
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_RunModeCmd_t))) {
                ADCS_SetRunModeCmd((const ADCS_RunModeCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_CONTROL_MODE_CC:
            // ID 58
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_ControlModeCmd_t))) {
                ADCS_SetControlModeCmd((const ADCS_ControlModeCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_WHL_CONFIG_CC:
            // ID 59
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_WhlConfigCmd_t))) {
                ADCS_SetWhlConfigCmd((const ADCS_WhlConfigCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_SATELLITE_CONFIG_CC:
            // ID 61
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_SatConfigCmd_t))) {
                ADCS_SetSatelliteConfigCmd((const ADCS_SatConfigCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_CONTROLLER_CONFIG_CC:
            // ID 62
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_ControllerConfig_t))) {
                ADCS_SetControllerConfigCmd((const ADCS_ControllerConfig_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_MAG0_MMT_CALIB_CONFIG_CC:
            // ID 63
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_Mag0MMTCalibConfigCmd_t))) {
                ADCS_SetMag0MMTCalibConfigCmd((const ADCS_Mag0MMTCalibConfigCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_DEFAULT_MODE_CONFIG_CC:
            // ID 64
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_DefaultModeConfigCmd_t))) {
                ADCS_SetDefaultModeConfigCmd((const ADCS_DefaultModeConfigCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_MOUNTING_CONFIG_CC:
            // ID 65
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_MountingConfigCmd_t))) {
                ADCS_SetMountingConfigCmd((const ADCS_MountingConfigCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_MAG1_MMT_CALIB_CONFIG_CC:
            // ID 66
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_Mag1MMTCalibConfigCmd_t))) {
                ADCS_SetMag1MMTCalibConfigCmd((const ADCS_Mag1MMTCalibConfigCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_ESTIMATOR_CONFIG_CC:
            // ID 67
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_EstimatorConfigCmd_t))) {
                ADCS_SetEstimatorConfigCmd((const ADCS_EstimatorConfigCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_SAT_ORBIT_PARAMS_CONFIG_CC:
            // ID 68
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_SatOrbitParamConfigCmd_t))) {
                ADCS_SetSatOrbitParamConfigCmd((const ADCS_SatOrbitParamConfigCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_NODE_SELECTION_CONFIG_CC:
            // ID 69
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_NodeSelectionConfigCmd_t))) {
                ADCS_SetNodeSelectionConfigCmd((const ADCS_NodeSelectionConfigCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_MTQ_CONFIG_CC:
            // ID 70
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_MTQConfigCmd_t))) {
                ADCS_SetMTQConfigCmd((const ADCS_MTQConfigCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_ESTIMATION_MODE_CC:
            // ID 71
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_EstimationModeCmd_t))) {
                ADCS_SetEstimationModeCmd((const ADCS_EstimationModeCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_OPERATIONAL_STATE_CC:
            // ID 72
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_OperationalStateCmd_t))) {
                ADCS_SetOperationalStateCmd((const ADCS_OperationalStateCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_MAG_SENSING_ELM_CONFIG_CC:
            // ID 77
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_MagSensingElmConfigCmd_t))) {
                ADCS_SetMagSensingElmConfigCmd((const ADCS_MagSensingElmConfigCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_UNSOLICIT_TLM_MSG_SETUP_CC:
            // ID 112
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_UnsolicitTlmMsgSetupCmd_t))) {
                ADCS_SetUnsolicitTlmMsgSetupCmd((const ADCS_UnsolicitTlmMsgSetupCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_UNSOLICIT_EVENT_MSG_SETUP_CC:
            // ID 116
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_UnsolicitEventMsgSetupCmd_t))) {
                ADCS_SetUnsolicitEventMsgSetupCmd((const ADCS_UnsolicitEventMsgSetupCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_SET_INITIATE_EVENT_LOG_TRANSFER_CC:
            // ID 120
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_InitiateEventLogTransferCmd_t))) {
                ADCS_SetInitiateEventLogTransferCmd((const ADCS_InitiateEventLogTransferCmd_t *)SBBufPtr);
            }
            break;
        
        /* 
        * Process Requested Telemetry
        */
        case ADCS_GET_ERROR_LOG_SETTING_CC:
            // ID 132
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetErrorLogSettingCmd_t))) {
                ADCS_GetErrorLogSettingCmd();
            }
            break;
        
        case ADCS_GET_CURRENT_UNIX_TIME_CC:
            // ID 133
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetCurrentUnixTimeCmd_t))) {
                ADCS_GetCurrentUnixTimeCmd();
            }
            break;
            
        case ADCS_GET_PERSIST_CONFIG_DIAGNOSTIC_CC:
            // ID 134
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetPersistConfigDiagnosticCmd_t))) {
                ADCS_GetPersistConfigDiagnosticCmd();
            }
            break;
        
        case ADCS_GET_COMMUNICATION_STATUS_CC:
            // ID 135
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetCommunicationStatusCmd_t))) {
                ADCS_GetCommunicationStatusCmd();
            }
            break;

        case ADCS_GET_CONTROL_ESTIMATION_MODE_CC:
            // ID 150
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetControlEstimationModeCmd_t))) {
                ADCS_GetControlEstimationModeCmd();
            }
            break;
        
        case ADCS_GET_REFERENCE_IRC_VECTOR_CC:
            // ID 156
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetReferenceIRCVectorCmd_t))) {
                ADCS_GetReferenceIRCVectorCmd();
            }
            break;
        
        case ADCS_GET_REFERENCE_LLH_TARGET_CC:
            // ID 157
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetReferenceLLHTargetCmd_t))) {
                ADCS_GetReferenceLLHTargetCmd();
            }
            break;

        case ADCS_GET_ORBIT_MODE_CC:
            // ID 162
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetOrbitModeCmd_t))) {
                ADCS_GetOrbitModeCmd();
            }
            break;

        case ADCS_GET_HEALTH_TLM_MMT_CC:
            // ID 167
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetHealthTlmMMTCmd_t))) {
                ADCS_GetHealthTlmMMTCmd();
            }
            break;

        case ADCS_GET_RAW_CUBESENSE_SUN_CC:
            // ID 170
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetRawCubeSenseSunCmd_t))) {
                ADCS_GetRawCubeSenseSunCmd();
            }
            break;

        case ADCS_GET_REFERENCE_RPY_VALUES_CC:
            // ID 181
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetReferenceRPYvaluesCmd_t))) {
                ADCS_GetReferenceRPYvaluesCmd();
            }
            break;

        case ADCS_GET_OPENLOOPCMD_MTQ_CC:
            // ID 182
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetOpenLoopCmdMTQCmd_t))) {
                ADCS_GetOpenLoopCmdMTQCmd();
            }
            break;

        case ADCS_GET_POWER_STATE_CC:
            // ID 183
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetPowerStateCmd_t))) {
                ADCS_GetPowerStateCmd();
            }
            break;
        
        case ADCS_GET_RUN_MODE_CC:
            // ID 184
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetRunModeCmd_t))) {
                ADCS_GetRunModeCmd();
            }
            break;

        case ADCS_GET_CONTROL_MODE_CC:
            // ID 185
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetControlModeCmd_t))) {
                ADCS_GetControlModeCmd();
            }
            break;

        case ADCS_GET_WHL_CONFIG_CC:
            // ID 186
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetWhlConfigCmd_t))) {
                ADCS_GetWhlConfigCmd();
            }
            break;

        case ADCS_GET_SATELLITE_CONFIG_CC:
            // ID 189
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetSatelliteConfigCmd_t))) {
                ADCS_GetSatelliteConfigCmd();
            }
            break;

        case ADCS_GET_CONTROLLER_CONFIG_CC:
            // ID 190
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetControllerConfigCmd_t))) {
                ADCS_GetControllerConfigCmd();
            }
            break;

        case ADCS_GET_MAG0_MMT_CALIB_CONFIG_CC:
            // ID 191
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetMag0MMTCalibConfigCmd_t))) {
                ADCS_GetMag0MMTCalibConfigCmd();
            }
            break;

        case ADCS_GET_DEFAULT_MODE_CONFIG_CC:
            // ID 192
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetDefaultModeConfigCmd_t))) {
                ADCS_GetDefaultModeConfigCmd();
            }
            break;

        case ADCS_GET_MOUNTING_CONFIG_CC:
            // ID 193
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetMountingConfigCmd_t))) {
                ADCS_GetMountingConfigCmd();
            }
            break;

        case ADCS_GET_MAG1_MMT_CALIB_CONFIG_CC:
            // ID 194
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetMag1MMTCalibConfigCmd_t))) {
                ADCS_GetMag1MMTCalibConfigCmd();
            }
            break;

        case ADCS_GET_ESTIMATOR_CONFIG_CC:
            // ID 195
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetEstimatorConfigCmd_t))) {
                ADCS_GetEstimatorConfigCmd();
            }
            break;

        case ADCS_GET_SAT_ORBIT_PARAM_CONFIG_CC:
            // ID 196
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetSatOrbitParamConfigCmd_t))) {
                ADCS_GetSatOrbitParamConfigCmd();
            }
            break;

        case ADCS_GET_NODE_SELECTION_CONFIG_CC:
            // ID 197
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetNodeSelectionConfigCmd_t))) {
                ADCS_GetNodeSelectionConfigCmd();
            }
            break;

        case ADCS_GET_MTQ_CONFIG_CC:
            // ID 198
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetMTQConfigCmd_t))) {
                ADCS_GetMTQConfigCmd();
            }
            break;

        case ADCS_GET_ESTIMATION_MODE_CC:
            // ID 199
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetEstimationModeCmd_t))) {
                ADCS_GetEstimationModeCmd();
            }
            break;

        case ADCS_GET_OPERATIONAL_STATE_CC:
            // ID 200
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetOperationalStateCmd_t))) {
                ADCS_GetOperationalStateCmd();
            }
            break;

        case ADCS_GET_RAW_CSS_SENSOR_CC:
            // ID 203
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetRawCSSSensorCmd_t))) {
                ADCS_GetRawCSSSensorCmd();
            }
            break;

        case ADCS_GET_RAW_GYR_SENSOR_CC:
            // ID 204
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetRawGYRSensorCmd_t))) {
                ADCS_GetRawGYRSensorCmd();
            }
            break;

        case ADCS_GET_CALIBRATED_GYR_SENSOR_CC:
            // ID 207
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetCalibratedGYRSensorCmd_t))) {
                ADCS_GetCalibratedGYRSensorCmd();
            }
            break;

        case ADCS_GET_MAG_SENSING_ELM_CONFIG_CC:
            // ID 221
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetMagSensingElmConfigCmd_t))) {
                ADCS_GetMagSensingElmConfigCmd();
            }
            break;

        case ADCS_GET_TLM_LOG_INCLMASK_CC:
            // ID 227
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetTlmLogInclMaskCmd_t))) {
                ADCS_GetTlmLogInclMaskCmd();
            }
            break;

        case ADCS_GET_UNSOLICIT_TLM_MSG_SETUP_CC:
            // ID 228
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetUnsolicitTlmMsgSetupCmd_t))) {
                ADCS_GetUnsolicitTlmMsgSetupCmd();
            }
            break;

        case ADCS_GET_UNSOLICIT_EVENT_MSG_SETUP_CC:
            // ID 233
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetUnsolicitEventMsgSetupCmd_t))) {
                ADCS_GetUnsolicitEventMsgSetupCmd();
            }
            break;

        case ADCS_GET_EVENT_LOG_STATUS_RESPONSE_CC:
            // ID 235
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetEventLogStatusReponseCmd_t))) {
                ADCS_GetEventLogStatusResponseCmd();
            }
            break;

        case ADCS_GET_PORTMAP_CC:
            // ID 239
            if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetPortMapCmd_t))) {
                ADCS_GetPortMapCmd();
            }
            break;

        case ADCS_SEQ_DTUMB_CC:
			if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_SequenceCmdDetumblingCmd_t))) {
                ADCS_SequenceCmd_Detumbling();
            }
            break;

		case ADCS_SEQ_SUNPT_CC:
			if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_SequenceCmdSunpointingCmd_t))) {
                ADCS_SequenceCmd_Sunpointing();
            }
            break;
		
		case ADCS_SEQ_VELPT_CC:
			if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_SequenceCmdVpointingCmd_t))) {
                ADCS_SequenceCmd_Vpointing();
            }
            break;

		case ADCS_SEQ_KSCPT_CC:
			if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_SequenceCmdKSCpointingCmd_t))) {
                ADCS_SequenceCmd_KSCpointing();
            }
            break;

		case ADCS_SEQ_LGCPT_CC:
			if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_SequenceCmdLGCpointingCmd_t))) {
                ADCS_SequenceCmd_LGCpointing();
            }
            break;

		case ADCS_SEQ_RPYPT_CC:
			if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_SequenceCmdRPYpointingCmd_t))) {
                ADCS_SequenceCmd_RPYpointing((const ADCS_SequenceCmdRPYpointingCmd_t *)SBBufPtr);
            }
            break;

		case ADCS_SET_ERROR_LOG_CLEAR_CC:
			if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_ErrorLogClearCmd_t))) {
                ADCS_SetErrorLogClearCmd((const ADCS_ErrorLogClearCmd_t *)SBBufPtr);
            }
            break;

        case ADCS_GET_CURRENT_UNIX_TIME_INTERNAL_CC:
            // ID 133
			if (ADCS_VerifyCmdLength(&SBBufPtr->Msg, sizeof(ADCS_GetCurrentUnixTimeCmd_t))) {
                ADCS_GetCurrentUnixTimeInternalCmd();
            }
            break;

		// case ADCS_LOOPTEST_CC:
		// 	ADCS_Loop();
		// 	break;
		
        /* default case already found during FC vs length test */
        default:
            CFE_EVS_SendEvent(ADCS_CC_ERR_EID, CFE_EVS_EventType_ERROR, "Invalid ground command code: CC = %d",
                              CommandCode);


            /* RPT */
            ADCS_ReportTlm_t *BufPtr = (ADCS_ReportTlm_t *)CFE_SB_AllocateMessageBuffer(sizeof(ADCS_ReportTlm_t));
            if (BufPtr == NULL) break;
            if(CFE_MSG_Init(CFE_MSG_PTR(BufPtr->TelemetryHeader), CFE_SB_ValueToMsgId(ADCS_REPORT_TLM_MID), sizeof(ADCS_ReportTlm_t) != CFE_SUCCESS)) {
                CFE_SB_ReleaseMessageBuffer((CFE_SB_Buffer_t *)BufPtr);
                break;
            }
            BufPtr->Report.MsgID = ADCS_CMD_MID;
            BufPtr->Report.CommandCode = (uint8_t)CommandCode;
            BufPtr->Report.ReturnType = RPT_RETTYPE_APP;
            BufPtr->Report.ReturnCode = CFE_STATUS_BAD_COMMAND_CODE;
            BufPtr->Report.ReturnDataSize = 0;
            CFE_SB_TimeStampMsg(CFE_MSG_PTR(BufPtr->TelemetryHeader));
            if(CFE_SB_TransmitBuffer((CFE_SB_Buffer_t *)BufPtr, true) != CFE_SUCCESS) {
                CFE_SB_ReleaseMessageBuffer((CFE_SB_Buffer_t *)BufPtr);
                break;
            }
            /* End of RPT */
            break;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the ADCS      */
/*     command pipe.                                                          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void ADCS_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    switch (CFE_SB_MsgIdToValue(MsgId))
    {
        case ADCS_CMD_MID:
            ADCS_ProcessGroundCommand(SBBufPtr);
            break;

        case ADCS_SEND_HK_MID:
            ADCS_SendHkCmd((const ADCS_SendHkCmd_t *)SBBufPtr);
            break;

        case ADCS_SEND_BCN_MID:
            ADCS_SendBcnCmd((const ADCS_SendBcnCmd_t *)SBBufPtr);
            break;

        case ADCS_LOOP_MID:
            ADCS_Loop();
            break;

        default:
            CFE_EVS_SendEvent(ADCS_MID_ERR_EID, CFE_EVS_EventType_ERROR,
                              "ADCS: invalid command packet,MID = 0x%x", (unsigned int)CFE_SB_MsgIdToValue(MsgId));


             /* RPT */
            ADCS_ReportTlm_t *BufPtr = (ADCS_ReportTlm_t *)CFE_SB_AllocateMessageBuffer(sizeof(ADCS_ReportTlm_t));
            if (BufPtr == NULL) break;
            if(CFE_MSG_Init(CFE_MSG_PTR(BufPtr->TelemetryHeader), CFE_SB_ValueToMsgId(ADCS_REPORT_TLM_MID), sizeof(ADCS_ReportTlm_t) != CFE_SUCCESS)) {
                CFE_SB_ReleaseMessageBuffer((CFE_SB_Buffer_t *)BufPtr);
                break;
            }
            BufPtr->Report.MsgID = ADCS_CMD_MID;
            BufPtr->Report.CommandCode = 0;
            BufPtr->Report.ReturnType = RPT_RETTYPE_APP;
            BufPtr->Report.ReturnCode = CFE_STATUS_UNKNOWN_MSG_ID;
            BufPtr->Report.ReturnDataSize = sizeof(CFE_SB_MsgId_Atom_t);
            memcpy(BufPtr->Report.ReturnValue, &MsgId.Value, sizeof(CFE_SB_MsgId_Atom_t));
            CFE_SB_TimeStampMsg(CFE_MSG_PTR(BufPtr->TelemetryHeader));
            if(CFE_SB_TransmitBuffer((CFE_SB_Buffer_t *)BufPtr, true) != CFE_SUCCESS) {
                CFE_SB_ReleaseMessageBuffer((CFE_SB_Buffer_t *)BufPtr);
                break;
            }
            /* End of RPT */
            break;
    }
}
