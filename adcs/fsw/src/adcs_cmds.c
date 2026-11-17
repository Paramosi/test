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
 *   This file contains the source code for the Adcs App Ground Command-handling functions
 */

/*
** Include Files:
*/
#include "adcs_app.h"
#include "adcs_cmds.h"
#include "adcs_msgids.h"
#include "adcs_tbl.h"
#include "adcs_utils.h"
#include "adcs_msg.h"
#include "adcs_eventids.h"
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the Apps     */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
CFE_Status_t ADCS_SendHkCmd(const ADCS_SendHkCmd_t *Msg)
{
    /*
    ** Send housekeeping telemetry packet...
    */
    CFE_SB_TimeStampMsg(CFE_MSG_PTR(ADCS_AppData.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(ADCS_AppData.HkTlm.TelemetryHeader), true);

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SendBcnCmd(const ADCS_SendBcnCmd_t *Msg)
{
    int32 Status;
    ADCS_PowerStateTlm_Payload_t PwrStt = {0,};
    ADCS_ControlModeTlm_Payload_t CtrlMode = {0,};
    ADCS_CalibratedGYRSensorTlm_Payload_t CalGYR = {0,};

    Status = ADCS_GetPowerState(&PwrStt);
    if (Status == CFE_SUCCESS) {
        uint8_t CombinedPowerState = 0;
        CombinedPowerState |= ((PwrStt.RWL0 & 1u) << 6);
        CombinedPowerState |= ((PwrStt.RWL1 & 1u) << 5);
        CombinedPowerState |= ((PwrStt.RWL2 & 1u) << 4);
        CombinedPowerState |= ((PwrStt.MAG0 & 1u) << 3);
        CombinedPowerState |= ((PwrStt.GYR0 & 1u) << 2);
        CombinedPowerState |= ((PwrStt.FSS0 & 1u) << 1);
        CombinedPowerState |= ((PwrStt.HSS0 & 1u) << 0);

        ADCS_AppData.BcnTlm.Payload.PowerState = CombinedPowerState;
    }
    Status = ADCS_GetControlMode(&CtrlMode);
    if (Status == CFE_SUCCESS) {
        ADCS_AppData.BcnTlm.Payload.ControlMode = CtrlMode.ControlMode;
    }
    Status = ADCS_GetCalibratedGYRSensor(&CalGYR);
    if (Status == CFE_SUCCESS) {
        ADCS_AppData.BcnTlm.Payload.GYR0CalibratedRateXComponent = CalGYR.GYR0CalibratedRateX;
        ADCS_AppData.BcnTlm.Payload.GYR0CalibratedRateYComponent = CalGYR.GYR0CalibratedRateY;
        ADCS_AppData.BcnTlm.Payload.GYR0CalibratedRateZComponent = CalGYR.GYR0CalibratedRateZ;
    }
    
    CFE_SB_TimeStampMsg(CFE_MSG_PTR(ADCS_AppData.BcnTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(ADCS_AppData.BcnTlm.TelemetryHeader), true);

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* ADCS NOOP commands                                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
CFE_Status_t ADCS_NoopCmd(const ADCS_NoopCmd_t *Msg)
{
    ADCS_AppData.CmdCounter++;

    uint8_t Cmds[2] = {ADCS_AppData.CmdCounter, ADCS_AppData.ErrCounter};
    ADCS_HandleReport(CFE_SUCCESS, ADCS_NOOP_CC, Cmds, sizeof(Cmds));

    CFE_EVS_SendEvent(ADCS_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION, "ADCS: NOOP command received.");

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
CFE_Status_t ADCS_ResetCountersCmd(const ADCS_ResetCountersCmd_t *Msg)
{
    ADCS_AppData.CmdCounter = 0;
    ADCS_AppData.ErrCounter = 0;

    uint8_t Cmds[2] = {ADCS_AppData.CmdCounter, ADCS_AppData.ErrCounter};
    ADCS_HandleReport(CFE_SUCCESS, ADCS_RESET_COUNTERS_CC, Cmds, sizeof(Cmds));

    CFE_EVS_SendEvent(ADCS_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "ADCS: RESET command");

    return CFE_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* ADCS RELATED COMMANDS COME HERE                                            */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*
CFE_Status_t ADCS_EN_HighCmd(void) {
    CFE_Status_t               status;

    CFE_SRL_GPIO_Handle_t *Handle = CFE_SRL_ApiGetGpioHandle(CFE_SRL_ADCS_EN_GPIO_INDEXER);
    status = CFE_SRL_ApiGpioSet(Handle, true);

    ADCS_HandleReport(status, ADCS_GPIO_ENABLE_HIGH_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Enable Enable pin: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("GPIO EN HIGH success.\n");
	OS_TaskDelay(10000);

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_EN_LowCmd(void){
    CFE_Status_t               status;

    CFE_SRL_GPIO_Handle_t *Handle = CFE_SRL_ApiGetGpioHandle(CFE_SRL_ADCS_EN_GPIO_INDEXER);
    status = CFE_SRL_ApiGpioSet(Handle, false);

    ADCS_HandleReport(status, ADCS_GPIO_ENABLE_LOW_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Disable Enable pin: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("GPIO EN LOW success.\n");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_Boot_HighCmd(void){
    CFE_Status_t               status;

    CFE_SRL_GPIO_Handle_t *Handle = CFE_SRL_ApiGetGpioHandle(CFE_SRL_THRUSTER_GPIO_INDEXER);
    status = CFE_SRL_ApiGpioSet(Handle, true);

    ADCS_HandleReport(status, ADCS_GPIO_BOOT_HIGH_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Enable Boot pin: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("GPIO BOOT high success.\n");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_Boot_LowCmd(void){
    CFE_Status_t               status;

    CFE_SRL_GPIO_Handle_t *Handle = CFE_SRL_ApiGetGpioHandle(CFE_SRL_THRUSTER_GPIO_INDEXER);
    status = CFE_SRL_ApiGpioSet(Handle, false);

    ADCS_HandleReport(status, ADCS_GPIO_BOOT_LOW_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Disable Boot pin: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("GPIO BOOT Low success.\n");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_ExitBootloader(void){
    CFE_Status_t               status = 0;

    // status = setJumpToDefaultApp_CAN();

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Exit Bootloader: 0x%08lx", (unsigned long)status);
        return status;
    }

    return CFE_SUCCESS;
}
*/
CFE_Status_t ADCS_SetReset(void){
    // This command has no reply
    CFE_Status_t               status;

    status = ADCS_Reset();

    ADCS_HandleReport(status, ADCS_SET_RESET_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Reset: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("Set reset success.\n");

    return CFE_SUCCESS;
}


/********************************************************
 * 
 * COSMIC Actual invoked command function
 * Upper functions are just the references
 * 
 ********************************************************/

/* Set function */
CFE_Status_t ADCS_SetCurrentUnixTimeCmd(const ADCS_CurrentUnixTimeCmd_t *msg) {
    // ID 2
    CFE_Status_t               status;

    status = ADCS_SetCurrentUnixTime(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_CURRENT_UNIX_TIME_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Current Unix Time: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetErrorLogSettingCmd(const ADCS_ErrorLogSettingCmd_t *msg) {
    // ID 6
    CFE_Status_t               status;

    status = ADCS_SetErrorLogSetting(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_ERROR_LOG_SETTING_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Error Log Seetings: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetPersistConfigCmd(const ADCS_PersistConfigCmd_t *msg) {
    // ID 7
    CFE_Status_t               status;

    status = ADCS_SetPersistConfig();

    ADCS_HandleReport(status, ADCS_SET_PERSIST_CONFIG_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Satellite Orbit Param Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetControlEstimationModeCmd(const ADCS_ControlEstimationModeCmd_t *msg) {
    // ID 42
    CFE_Status_t               status;

    status = ADCS_SetControlEstimationMode(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_CONTROL_ESTIMATION_MODE_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Control Estimation Mode: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetDisableMagRwlMntMngCmd(const ADCS_DisableMagRwlMntMngCmd_t *msg) {
    // ID 43
    CFE_Status_t               status;

    status = ADCS_SetDisableMagRwlMntMng(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_DISABLE_MAG_RWL_MNT_MNG_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Disable Magnetic RWL Momentum Management: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetReferenceIRCVectorCmd(const ADCS_ReferenceIRCVectorCmd_t *msg) {
    // ID 47
    CFE_Status_t               status;

    status = ADCS_SetReferenceIRCVector(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_REFERENCE_IRC_VECTOR_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Reference IRC Vector: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetReferenceLLHTargetCmd(const ADCS_ReferenceLLHTargetCmd_t *msg) {
    // ID 48
    CFE_Status_t               status;

    status = ADCS_SetReferenceLLHTarget(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_REFERENCE_LLH_TARGET_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Reference LLH Target: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetOrbitModeCmd(const ADCS_OrbitModeCmd_t *msg) {
    // ID 51
    CFE_Status_t               status;

    status = ADCS_SetOrbitMode(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_ORBIT_MODE_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Orbit Mode: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetMagDeployCmd(const ADCS_MagDeployCmd_t *msg) {
    // ID 52
    CFE_Status_t               status;

    status = ADCS_SetMagDeploy(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_MAG_DEPLOY_CMD_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set MAG Deploy: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetReferenceRPYValuesCmd(const ADCS_ReferenceRPYvaluesCmd_t *msg) {
    // ID 54
    CFE_Status_t               status;

    status = ADCS_SetReferenceRPYValues(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_REFERENCE_RPY_VALUES_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Reference RPY Values: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetOpenLoopCmdMTQCmd(const ADCS_OpenLoopCmdMTQCmd_t *msg) {
    // ID 55
    CFE_Status_t               status;

    status = ADCS_SetOpenLoopCmdMTQ(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_OPENLOOPCMD_MTQ_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Open Loop Command MTQ: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetPowerStateCmd(const ADCS_PowerStateCmd_t *msg) {
    // ID 56
    CFE_Status_t               status;

    status = ADCS_SetPowerState(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_POWER_STATE_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Power State: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetRunModeCmd(const ADCS_RunModeCmd_t *msg) {
    // ID 57
    CFE_Status_t               status;

    status = ADCS_SetRunMode(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_RUN_MODE_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Satellite Orbit Param Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetControlModeCmd(const ADCS_ControlModeCmd_t *msg) {
    // ID 58
    CFE_Status_t               status;

    status = ADCS_SetControlMode(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_CONTROL_MODE_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Control Mode: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetWhlConfigCmd(const ADCS_WhlConfigCmd_t *msg) {
    // ID 59
    CFE_Status_t               status;

    status = ADCS_SetWhlConfig(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_WHL_CONFIG_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Wheel Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetSatelliteConfigCmd(const ADCS_SatConfigCmd_t *msg) {
    // ID 61
    CFE_Status_t               status;

    status = ADCS_SetSatelliteConfig(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_SATELLITE_CONFIG_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Satellite Orbit Param Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetControllerConfigCmd(const ADCS_ControllerConfig_t *msg) {
    // ID 62
    CFE_Status_t               status;

    status = ADCS_SetControllerConfig(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_CONTROLLER_CONFIG_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Satellite Orbit Param Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetMag0MMTCalibConfigCmd(const ADCS_Mag0MMTCalibConfigCmd_t *msg) {
    // ID 63
    CFE_Status_t               status;

    status = ADCS_SetMag0MMTCalibConfig(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_MAG0_MMT_CALIB_CONFIG_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set MAG0 MMT Calibration Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetDefaultModeConfigCmd(const ADCS_DefaultModeConfigCmd_t *msg) {
    // ID 64
    CFE_Status_t               status;

    status = ADCS_SetDefaultModeConfig(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_DEFAULT_MODE_CONFIG_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Satellite Orbit Param Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetMountingConfigCmd(const ADCS_MountingConfigCmd_t *msg) {
    // ID 65
    CFE_Status_t               status;

    status = ADCS_SetMountingConfig(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_MOUNTING_CONFIG_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Satellite Orbit Param Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetMag1MMTCalibConfigCmd(const ADCS_Mag1MMTCalibConfigCmd_t *msg) {
    // ID 66
    CFE_Status_t               status;

    status = ADCS_SetMag1MMTCalibConfig(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_MAG1_MMT_CALIB_CONFIG_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set MAG1 MMT Calibration Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetEstimatorConfigCmd(const ADCS_EstimatorConfigCmd_t *msg) {
    // ID 67
    CFE_Status_t               status;

    status = ADCS_SetEstimatorConfig(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_ESTIMATOR_CONFIG_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Estimator Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetSatOrbitParamConfigCmd(const ADCS_SatOrbitParamConfigCmd_t *msg) {
    // ID 68
    CFE_Status_t               status;

    status = ADCS_SetSatOrbitParamConfig(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_SAT_ORBIT_PARAMS_CONFIG_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Satellite Orbit Param Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetNodeSelectionConfigCmd(const ADCS_NodeSelectionConfigCmd_t *msg) {
    // ID 69
    CFE_Status_t               status;

    status = ADCS_SetNodeSelectionConfig(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_NODE_SELECTION_CONFIG_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Node Selection Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetMTQConfigCmd(const ADCS_MTQConfigCmd_t *msg) {
    // ID 70
    CFE_Status_t               status;

    status = ADCS_SetMTQConfig(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_MTQ_CONFIG_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set MTQ Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetEstimationModeCmd(const ADCS_EstimationModeCmd_t *msg) {
    // ID 71
    CFE_Status_t               status;

    status = ADCS_SetEstimationMode(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_ESTIMATION_MODE_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Estimation Mode: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetOperationalStateCmd(const ADCS_OperationalStateCmd_t *msg) {
    // ID 72
    CFE_Status_t               status;

    status = ADCS_SetOperationalState(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_OPERATIONAL_STATE_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Operational State: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetMagSensingElmConfigCmd(const ADCS_MagSensingElmConfigCmd_t *msg) {
    // ID 77
    CFE_Status_t               status;

    status = ADCS_SetMagSensingElmConfig(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_MAG_SENSING_ELM_CONFIG_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set MAG Sensing Element Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetUnsolicitTlmMsgSetupCmd(const ADCS_UnsolicitTlmMsgSetupCmd_t *msg) {
    // ID 112
    CFE_Status_t               status;

    status = ADCS_SetUnsolicitTlmMsgSetup(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_UNSOLICIT_TLM_MSG_SETUP_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Unsolicit Telemetry Messeage Setup: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetUnsolicitEventMsgSetupCmd(const ADCS_UnsolicitEventMsgSetupCmd_t *msg) {
    // ID 116
    CFE_Status_t               status;
    ADCS_UnsolicitEventMsgSetupCmd_InternalPayload_t Payload = {0,};

    if(msg->Payload.Flag) {
        Payload.InfoCAN = 1;
        Payload.MinorCAN = 1;
        Payload.MajorCAN = 1;
        Payload.CriticalCAN = 1;
    }

    status = ADCS_SetUnsolicitEventMsgSetup(&Payload);

    ADCS_HandleReport(status, ADCS_SET_UNSOLICIT_EVENT_MSG_SETUP_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Unsolicit Event Messeage Setup: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetInitiateEventLogTransferCmd(const ADCS_InitiateEventLogTransferCmd_t *msg) {
    // ID 120
    CFE_Status_t               status;

    status = ADCS_SetInitiateEventLogTransfer(&msg->Payload);

    ADCS_HandleReport(status, ADCS_SET_INITIATE_EVENT_LOG_TRANSFER_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Satellite Orbit Param Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

/********************************************************
 * 
 * COSMIC Actual Get Command Function (Get tlm)
 * 
 ********************************************************/
CFE_Status_t ADCS_GetErrorLogSettingCmd(void) {
    // ID 132
    CFE_Status_t               status;
    ADCS_ErrorLogSettingTlm_Payload_t RetVal = {0,};

    status = ADCS_GetErrorLogSetting(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_ERROR_LOG_SETTING_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Error Log Settings: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("ActiveState: %u || BufferFullAction: %u\n", RetVal.ActiveState, RetVal.BufferFullAction);
    
    return CFE_SUCCESS;
}

 CFE_Status_t ADCS_GetCurrentUnixTimeCmd(void) {
    // ID 133
    CFE_Status_t               status;
    ADCS_CurrentUnixTimeTlm_Payload_t RetVal = {0,};

    status = ADCS_GetCurrentUnixTime(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_CURRENT_UNIX_TIME_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Current Unix Time: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("Unix Time sec: %u || Unix Time subsec: %u\n", RetVal.CurrentUnixseconds, RetVal.CurrentUnixNanoseconds);
    
    return CFE_SUCCESS;
}


CFE_Status_t ADCS_GetCurrentUnixTimeInternalCmd(void) {
    // ID 133
    /* Check ADCS comm. status */
    /* Try 5 times, if all fail, Send EVS */
    /* HS will ingest this, and send Hard Reset to EPS */
    CFE_Status_t               status;
    ADCS_CurrentUnixTimeTlm_Payload_t RetVal = {0,};

    for (uint8_t i = 0; i < 5; i++) {
        status = ADCS_GetCurrentUnixTime(&RetVal);
        if (status != CFE_SUCCESS) {
            ADCS_AppData.BootUpCheckCounter ++;
        }
        else ADCS_AppData.BootUpCheckCounter = 0;

        OS_TaskDelay(100);
    }
    if (ADCS_AppData.BootUpCheckCounter == 5)
        CFE_EVS_SendErr(ADCS_BOOTUP_CHECK_ERR_EID, "ADCS Boot up check fail. Need S/C Power reset.\n");
    
    return CFE_SUCCESS;
}


CFE_Status_t ADCS_GetPersistConfigDiagnosticCmd(void) {
    // ID 134
    CFE_Status_t               status;
    ADCS_PersistConfigDiagnosticTlm_Payload_t RetVal = {0,};

    status = ADCS_GetPersistConfigDiagnostic(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_PERSIST_CONFIG_DIAGNOSTIC_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Persist Config Diagonostic: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("State: %u\nLast Result: %u\nTimeStamp: %u\n",
                RetVal.State, RetVal.LastResult, RetVal.Timestamp);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetCommunicationStatusCmd(void) {
    // ID 135
    CFE_Status_t               status;
    ADCS_CommunicationStatusTlm_Payload_t RetVal = {0,};

    status = ADCS_GetCommunicationStatus(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_COMMUNICATION_STATUS_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Communication Status: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("[CAN Status]\n");
    OS_printf("TC counter          : %u\n", RetVal.CAN_TcCnt);
    OS_printf("TM Request counter  : %u\n", RetVal.CAN_TlmCnt);
    OS_printf("Errors in SW checkes: %u\n", RetVal.CAN_ErrSW);
    OS_printf("Erross by HW flags  : %u\n", RetVal.CAN_ErrHW);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetControlEstimationModeCmd(void) {
    // ID 150
    CFE_Status_t               status;
    ADCS_ControlEstimationModeTlm_Payload_t RetVal = {0,};

    status = ADCS_GetControlEstimationMode(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_CONTROL_ESTIMATION_MODE_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Control Estimation Mode: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("Control Mode: %u || Main Estimator Mode: %u || Backup Estimator Mode: %u || Control Timeout: %u\n",
                RetVal.ControlMode, RetVal.MainEstimatorMode, RetVal.BackupEstimatorMode, RetVal.ControlTimeout);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetReferenceIRCVectorCmd(void) {
    // ID 156
    CFE_Status_t               status;
    ADCS_ReferenceIRCVectorTlm_Payload_t RetVal = {0,};

    status = ADCS_GetReferenceIRCVector(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_REFERENCE_IRC_VECTOR_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get ReferenceIRC Vector: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("ECI pointing vector X: %f || ECI pointing vector Y: %f || ECI pointing vector Z: %f\n", RetVal.ECIPointingVectorX, RetVal.ECIPointingVectorY, RetVal.ECIPointingVectorZ);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetReferenceLLHTargetCmd(void) {
    // ID 157
    CFE_Status_t               status;
    ADCS_ReferenceLLHTargetTlm_Payload_t RetVal = {0,};

    status = ADCS_GetReferenceLLHTarget(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_REFERENCE_LLH_TARGET_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Reference LLH Target: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("Latitude: %f || Longitude: %f || Altitude: %f\n", RetVal.Latitude, RetVal.Longitude, RetVal.Altitude);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetOrbitModeCmd(void) {
    // ID 162
    CFE_Status_t               status;
    ADCS_OrbitModeTlm_Payload_t RetVal = {0,};

    status = ADCS_GetOrbitMode(&RetVal);
    
    ADCS_HandleReport(status, ADCS_GET_ORBIT_MODE_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Orbit Mode: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("Orbit Mode: %u\n", RetVal.OrbitMode);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetHealthTlmMMTCmd(void) {
    // ID 167
    CFE_Status_t               status;
    ADCS_HealthTlmMMTTlm_Payload_t RetVal = {0,};

    status = ADCS_GetHealthTlmMMT(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_HEALTH_TLM_MMT_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Health Telemetry MMT: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("MAG0::\n");
    OS_printf("MCU Temp        : %d || MCU Current    : %u || MCU Voltage  : %u\n", RetVal.Mag0MCUTemperature, RetVal.Mag0MCUCurrent, RetVal.Mag0MCUVoltage);
    OS_printf("Primary Temp    : %d || Redundant Temp : %d || Burn Current : %u\n", RetVal.Mag0PrimaryTemperature, RetVal.Mag0RedundantTemperature, RetVal.Mag0BurnCurrent);
    OS_printf("DeployPinState  : %u || BurnPinState   : %u\n", RetVal.Mag0DeployPinState, RetVal.Mag0BurnPinState);
    OS_printf("BurnUnderCurrent: %u || BurnOverCurrent: %u || DeployTimeout: %u\n", RetVal.Mag0BurnUnderCurrent, RetVal.Mag0BurnOverCurrent, RetVal.Mag0DeployTimeout);

    OS_printf("MAG1::\n");
    OS_printf("MCU Temp        : %d || MCU Current    : %u || MCU Voltage  : %u\n", RetVal.Mag1MCUTemperature, RetVal.Mag1MCUCurrent, RetVal.Mag1MCUVoltage);
    OS_printf("Primary Temp    : %d || Redundant Temp : %d || Burn Current : %u\n", RetVal.Mag1PrimaryTemperature, RetVal.Mag1RedundantTemperature, RetVal.Mag1BurnCurrent);
    OS_printf("DeployPinState  : %u || BurnPinState   : %u\n", RetVal.Mag1DeployPinState, RetVal.Mag1BurnPinState);
    OS_printf("BurnUnderCurrent: %u || BurnOverCurrent: %u || DeployTimeout: %u\n", RetVal.Mag1BurnUnderCurrent, RetVal.Mag1BurnOverCurrent, RetVal.Mag1DeployTimeout);
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetRawCubeSenseSunCmd(void) {
    // ID 170
    CFE_Status_t               status;
    ADCS_RawCubeSenseSunTlm_Payload_t RetVal = {0,};

    status = ADCS_GetRawCubeSenseSun(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_RAW_CUBESENSE_SUN_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Raw CubeSense Sun: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("TimeSec: %u || TimeNanoSec: %u\n", RetVal.TimeSecond, RetVal.TimeNanoSecond);
    OS_printf("FSS0 Alpha: %d || Beta: %d || Capture Res: %u || Detection Res: %u\n", RetVal.FSS0AlphaAngle, RetVal.FSS0BetaAngle, RetVal.FSS0CaptureResult, RetVal.FSS0DetectionResult);
    OS_printf("FSS1 Alpha: %d || Beta: %d || Capture Res: %u || Detection Res: %u\n", RetVal.FSS1AlphaAngle, RetVal.FSS1BetaAngle, RetVal.FSS1CaptureResult, RetVal.FSS1DetectionResult);
    OS_printf("FSS2 Alpha: %d || Beta: %d || Capture Res: %u || Detection Res: %u\n", RetVal.FSS2AlphaAngle, RetVal.FSS2BetaAngle, RetVal.FSS2CaptureResult, RetVal.FSS2DetectionResult);
    OS_printf("FSS3 Alpha: %d || Beta: %d || Capture Res: %u || Detection Res: %u\n", RetVal.FSS3AlphaAngle, RetVal.FSS3BetaAngle, RetVal.FSS3CaptureResult, RetVal.FSS3DetectionResult);
    OS_printf("Valid Res: 0x%02X\n", RetVal.ValidResult);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetReferenceRPYvaluesCmd(void) {
    // ID 181
    CFE_Status_t               status;
    ADCS_ReferenceRPYvaluesTlm_Payload_t RetVal = {0,};

    status = ADCS_GetReferenceRPYvalues(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_REFERENCE_RPY_VALUES_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Reference RPY Values: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval    
    OS_printf("Roll: %f || Pitch: %f || Yaw: %f\n", RetVal.Roll, RetVal.Pitch, RetVal.Yaw);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetOpenLoopCmdMTQCmd(void) {
    // ID 182
    CFE_Status_t               status;
    ADCS_OpenLoopCmdMTQTlm_Payload_t RetVal = {0,};

    status = ADCS_GetOpenLoopCmdMTQ(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_OPENLOOPCMD_MTQ_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Open Loop Cmd MTQ: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval    
    OS_printf("MTQ0_OpenLoopCmd: %d || MTQ1_OpenLoopCmd: %d || MTQ2_OpenLoopCmd: %d\n", RetVal.MTQ0_OpenLoopCmd, RetVal.MTQ1_OpenLoopCmd, RetVal.MTQ2_OpenLoopCmd);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetPowerStateCmd(void) {
    // ID 183
    CFE_Status_t               status;
    ADCS_PowerStateTlm_Payload_t RetVal = {0,};

    status = ADCS_GetPowerState(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_POWER_STATE_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Power State: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("RWL0: %u || RWL1: %u || RWL2: %u || RWL3: %u\n", RetVal.RWL0, RetVal.RWL1, RetVal.RWL2, RetVal.RWL3);
    OS_printf("MAG0: %u || MAG1: %u || GYR0: %u || GYR1: %u\n", RetVal.MAG0, RetVal.MAG1, RetVal.GYR0, RetVal.GYR1);
    OS_printf("FSS0: %u || FSS1: %u || FSS2: %u || FSS3: %u\n", RetVal.FSS0, RetVal.FSS1, RetVal.FSS2, RetVal.FSS3);
    OS_printf("HSS0: %u || HSS1: %u || STR0: %u || STR1: %u\n", RetVal.HSS0, RetVal.HSS1, RetVal.STR0, RetVal.STR1);
    OS_printf("EXT: Sensor0 %u || Sensor1: %u || GYR0: %u || GYR1: %u\n", RetVal.ExtSensor0, RetVal.ExtSensor1, RetVal.ExtGYR0, RetVal.ExtGYR1);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetRunModeCmd(void) {
    // ID 184
    CFE_Status_t               status;
    ADCS_RunModeTlm_Payload_t RetVal = {0,};

    status = ADCS_GetRunMode(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_RUN_MODE_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Calibrated GYR Sensor: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("Run Mode: %u\n", RetVal.RunMode);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetControlModeCmd(void) {
    // ID 185
    CFE_Status_t               status;
    ADCS_ControlModeTlm_Payload_t RetVal = {0,};

    status = ADCS_GetControlMode(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_CONTROL_MODE_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Control Mode: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("Control Mode   : %u\n", RetVal.ControlMode);
    OS_printf("Control Timeout: %u\n", RetVal.ControlTimeout);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetWhlConfigCmd(void) {
    // ID 186
    CFE_Status_t               status;
    ADCS_WhlConfigTlm_Payload_t RetVal = {0,};

    status = ADCS_GetWhlConfig(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_WHL_CONFIG_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Wheel Config Telemetry: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("Rwl0Inertia: %f || Rwl0MaxMomentum: %f || Rwl0MaxToque: %f\n", RetVal.Rwl0Inertia, RetVal.Rwl0MaxMomentum, RetVal.Rwl0MaxToque);
    OS_printf("Rwl1Inertia: %f || Rwl1MaxMomentum: %f || Rwl1MaxToque: %f\n", RetVal.Rwl0Inertia, RetVal.Rwl0MaxMomentum, RetVal.Rwl0MaxToque);
    OS_printf("Rwl2Inertia: %f || Rwl2MaxMomentum: %f || Rwl2MaxToque: %f\n", RetVal.Rwl0Inertia, RetVal.Rwl0MaxMomentum, RetVal.Rwl0MaxToque);
    OS_printf("Rwl3Inertia: %f || Rwl3MaxMomentum: %f || Rwl3MaxToque: %f\n", RetVal.Rwl0Inertia, RetVal.Rwl0MaxMomentum, RetVal.Rwl0MaxToque);
    OS_printf("WheelRampTorque: %f\n", RetVal.WheelRampTorque);
    OS_printf("WheelScheme: %u || FailedWheelID: %u\n", RetVal.WheelScheme, RetVal.FailedWheelID);
        
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetSatelliteConfigCmd(void) {
    // ID 189
    CFE_Status_t               status;
    ADCS_SatelliteConfigTlm_Payload_t RetVal = {0,};

    status = ADCS_GetSatelliteConfig(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_SATELLITE_CONFIG_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Satellite Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("[MOI]\n");
    OS_printf("Ixx: %f || Iyy: %f || Izz: %f\n", RetVal.Ixx, RetVal.Iyy, RetVal.Izz);
    OS_printf("Ixy: %f || Ixz: %f || Iyz: %f\n", RetVal.Ixy, RetVal.Ixz, RetVal.Iyz);
    OS_printf("---------\n");
    OS_printf("[SunPointing Body Vector]\n");
    OS_printf("X_RAW : %d  || Y_RAW : %d  || Z_RAW : %d\n", RetVal.SunPointingBodyVectorX, RetVal.SunPointingBodyVectorY, RetVal.SunPointingBodyVectorZ);
    OS_printf("X_Real: %f || Y_Real: %f || Z_Real: %f\n", RetVal.SunPointingBodyVectorX*0.0001, RetVal.SunPointingBodyVectorY*0.0001, RetVal.SunPointingBodyVectorZ*0.0001);
    OS_printf("---------\n");
    OS_printf("[TargetTracking Body Vector]\n");
    OS_printf("X_RAW : %d  || Y_RAW : %d  || Z_RAW : %d\n", RetVal.TargetTrackingBodyVectorX, RetVal.TargetTrackingBodyVectorY, RetVal.TargetTrackingBodyVectorZ);
    OS_printf("X_Real: %f || Y_Real: %f || Z_Real: %f\n", RetVal.TargetTrackingBodyVectorX*0.0001, RetVal.TargetTrackingBodyVectorY*0.0001, RetVal.TargetTrackingBodyVectorZ*0.0001);
    OS_printf("---------\n");
    OS_printf("[SatTracking Body Vector]\n");
    OS_printf("X_RAW : %d  || Y_RAW : %d  || Z_RAW : %d\n", RetVal.SatTrackingBodyVectorX, RetVal.SatTrackingBodyVectorY, RetVal.SatTrackingBodyVectorZ);
    OS_printf("X_Real: %f || Y_Real: %f || Z_Real: %f\n", RetVal.SatTrackingBodyVectorX*0.0001, RetVal.SatTrackingBodyVectorY*0.0001, RetVal.SatTrackingBodyVectorZ*0.0001);

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetControllerConfigCmd(void) {
    // ID 190
    CFE_Status_t               status;
    ADCS_ControllerConfigTlm_Payload_t RetVal = {0,};

    status = ADCS_GetControllerConfig(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_CONTROLLER_CONFIG_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Controller Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("Default Control Mode         : %u\n", RetVal.DefaultControlMode);
    OS_printf("[Gains]\n");
    OS_printf("DetumblingDampingGain        : %f\n", RetVal.DetumblingDampingGain);
    OS_printf("SunSpinGain_Sunlit        : %f\n", RetVal.SunSpinGain_Sunlit);
    OS_printf("SunSpinGain_Eclipse        : %f\n", RetVal.SunSpinGain_Eclipse);
    OS_printf("DetumblingSpinGain        : %f\n", RetVal.DetumblingSpinGain);
    OS_printf("FastBDotGain        : %f\n", RetVal.FastBDotGain);
    OS_printf("YMomNutationDampingGain        : %f\n", RetVal.YMomNutationDampingGain);
    OS_printf("YMomNutationDampingQuatGain        : %f\n", RetVal.YMomNutationDampingQuatGain);
    OS_printf("XGGQuatGain        : %f\n", RetVal.XGGQuatGain);
    OS_printf("YGGQuatGain        : %f\n", RetVal.YGGQuatGain);
    OS_printf("ZGGQuatGain        : %f\n", RetVal.ZGGQuatGain);
    OS_printf("WheelDesatControlGain        : %f\n", RetVal.WheelDesatControlGain);
    OS_printf("YMomProportionalGain        : %f\n", RetVal.YMomProportionalGain);
    OS_printf("YMomDerivativeGain        : %f\n", RetVal.YMomDerivativeGain);
    OS_printf("RWheelProportionalGain        : %f\n", RetVal.RWheelProportionalGain);
    OS_printf("RWheelDerivativeGain        : %f\n", RetVal.RWheelDerivativeGain);
    OS_printf("TrackingProportionalGain        : %f\n", RetVal.TrackingProportionalGain);
    OS_printf("TrackingDerivativeGain        : %f\n", RetVal.TrackingDerivativeGain);
    OS_printf("TrackingIntegralGain        : %f\n", RetVal.TrackingIntegralGain);
    OS_printf("ReferenceSpinRate_degps        : %f\n", RetVal.ReferenceSpinRate_degps);
    OS_printf("ReferenceWheelMomentum_Nms        : %f\n", RetVal.ReferenceWheelMomentum_Nms);
    OS_printf("YWheelBiasMomentum_Nms        : %f\n", RetVal.YWheelBiasMomentum_Nms);
    OS_printf("RefSpinRate_RW_degps        : %f\n", RetVal.RefSpinRate_RW_degps);
    OS_printf("SunKeepOutAngle_deg        : %f\n", RetVal.SunKeepOutAngle_deg);
    OS_printf("RollLimitAngle_deg        : %f\n", RetVal.RollLimitAngle_deg);

    OS_printf("[Flags]\n");
    OS_printf("YawCompensationForEarthRotation: %u\n", RetVal.flags.YawCompensationForEarthRotation);
    OS_printf("Enable SunTracking In Eclipse: %u\n", RetVal.flags.EnableSunTrackingInEclipse);
    OS_printf("Enable SunAvoidance          : %u\n", RetVal.flags.EnableSunAvoidance);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetMag0MMTCalibConfigCmd(void) {
    // ID 191
    CFE_Status_t               status;
    ADCS_Mag0MMTCalibConfigTlm_Payload_t RetVal = {0,};

    status = ADCS_GetMag0MMTCalibConfig(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_MAG0_MMT_CALIB_CONFIG_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get MAG0 MMT Calibration Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("[Offset]\n");
    OS_printf("(RAW)  Channel 1: %d || Channel 2: %d || Channel 3: %d\n", RetVal.MMT_Ch1Offset, RetVal.MMT_Ch2Offset, RetVal.MMT_Ch3Offset);
    OS_printf("(Real) Channel 1: %f || Channel 2: %f || Channel 3: %f\n", RetVal.MMT_Ch1Offset*0.001, RetVal.MMT_Ch2Offset*0.001, RetVal.MMT_Ch3Offset*0.001);
    OS_printf("\n");
    OS_printf("[Sensitivity Matrix]\n");
    OS_printf("(RAW)\n");
    OS_printf("S11: %d || S12: %d || S13: %d\n", RetVal.MMT_SensitivityMAT_S11, RetVal.MMT_SensitivityMAT_S12, RetVal.MMT_SensitivityMAT_S13);
    OS_printf("S21: %d || S22: %d || S23: %d\n", RetVal.MMT_SensitivityMAT_S21, RetVal.MMT_SensitivityMAT_S22, RetVal.MMT_SensitivityMAT_S23);
    OS_printf("S31: %d || S32: %d || S33: %d\n", RetVal.MMT_SensitivityMAT_S31, RetVal.MMT_SensitivityMAT_S32, RetVal.MMT_SensitivityMAT_S33);
    OS_printf("(Real)\n");
    OS_printf("S11: %f || S12: %f || S13: %f\n", RetVal.MMT_SensitivityMAT_S11*0.001, RetVal.MMT_SensitivityMAT_S12*0.001, RetVal.MMT_SensitivityMAT_S13*0.001);
    OS_printf("S21: %f || S22: %f || S23: %f\n", RetVal.MMT_SensitivityMAT_S21*0.001, RetVal.MMT_SensitivityMAT_S22*0.001, RetVal.MMT_SensitivityMAT_S23*0.001);
    OS_printf("S31: %f || S32: %f || S33: %f\n", RetVal.MMT_SensitivityMAT_S31*0.001, RetVal.MMT_SensitivityMAT_S32*0.001, RetVal.MMT_SensitivityMAT_S33*0.001);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetDefaultModeConfigCmd(void) {
    // ID 192
    CFE_Status_t               status;
    ADCS_DefaultModeConfigTlm_Payload_t RetVal = {0,};

    status = ADCS_GetDefaultModeConfig(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_DEFAULT_MODE_CONFIG_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Default Mode COnfig: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("Default Run Mode            : %u\n", RetVal.DefaultRunMode);
    OS_printf("Default Operational State   : %u\n", RetVal.DefaultOperationalState);
    OS_printf("Default Control Mode in Safe: %u\n", RetVal.DefaultControlModeInOpStateSafe);
    OS_printf("Default Control Mode in Auto: %u\n", RetVal.DefaultControlModeInOpStateAuto);
    

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetMountingConfigCmd(void) {
    // ID 193
    CFE_Status_t               status;
    ADCS_MountingConfigTlm_Payload_t RetVal = {0,};

    status = ADCS_GetMountingConfig(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_MOUNTING_CONFIG_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Calibrated GYR Sensor: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("[Direction ENUM]\n");
    OS_printf("| +X | -X | +Y | -Y | +Z | -Z |\n");
    OS_printf("|  1 |  2 |  3 |  4 |  5 |  6 |\n");

    OS_printf("\n");
    OS_printf("[Stack]\n");
    OS_printf("X-Y-Z: %u-%u-%u\n", RetVal.StackX_mounting, RetVal.StackY_mounting, RetVal.StackZ_mounting);

    OS_printf("\n");
    OS_printf("[MTQ]\n");
    OS_printf("0-1-2: %u-%u-%u\n", RetVal.MTQ0_mounting, RetVal.MTQ1_mounting, RetVal.MTQ2_mounting);    
    
    OS_printf("\n");
    OS_printf("[Wheel]\n");
    OS_printf("0-1-2-3: %u-%u-%u-%u\n", RetVal.Wheel0_mounting, RetVal.Wheel1_mounting, RetVal.Wheel2_mounting, RetVal.Wheel3_mounting);
    OS_printf("Pyramid alpha-beta-gamma: %u-%u-%u\n", RetVal.PyramidRWL_alpha, RetVal.PyramidRWL_beta, RetVal.PyramidRWL_gamma);
    
    OS_printf("\n");
    OS_printf("[CSS]\n");
    OS_printf("0-1-2-3-4: %u-%u-%u-%u-%u\n", RetVal.CSS0_mounting, RetVal.CSS1_mounting, RetVal.CSS2_mounting, RetVal.CSS3_mounting, RetVal.CSS4_mounting);
    OS_printf("5-6-7-8-9: %u-%u-%u-%u-%u\n", RetVal.CSS5_mounting, RetVal.CSS6_mounting, RetVal.CSS7_mounting, RetVal.CSS8_mounting, RetVal.CSS9_mounting);

    OS_printf("\n");
    OS_printf("[FSS0]\n");
    OS_printf("(RAW)  alpha-beta-gamma: %d-%d-%d\n", RetVal.FSS0_alpha, RetVal.FSS0_beta, RetVal.FSS0_gamma);
    OS_printf("(Real) alpha-beta-gamma: %f-%f-%f\n", RetVal.FSS0_alpha*0.01, RetVal.FSS0_beta*0.01, RetVal.FSS0_gamma*0.01);

    OS_printf("\n");
    OS_printf("[FSS1]\n");
    OS_printf("(RAW)  alpha-beta-gamma: %d-%d-%d\n", RetVal.FSS1_alpha, RetVal.FSS1_beta, RetVal.FSS1_gamma);
    OS_printf("(Real) alpha-beta-gamma: %f-%f-%f\n", RetVal.FSS1_alpha*0.01, RetVal.FSS1_beta*0.01, RetVal.FSS1_gamma*0.01);

    OS_printf("\n");
    OS_printf("[HSS0]\n");
    OS_printf("(RAW)  alpha-beta-gamma: %d-%d-%d\n", RetVal.HSS0_alpha, RetVal.HSS0_beta, RetVal.HSS0_gamma);
    OS_printf("(Real) alpha-beta-gamma: %f-%f-%f\n", RetVal.HSS0_alpha*0.01, RetVal.HSS0_beta*0.01, RetVal.HSS0_gamma*0.01);

    OS_printf("\n");
    OS_printf("[HSS1]\n");
    OS_printf("(RAW)  alpha-beta-gamma: %d-%d-%d\n", RetVal.HSS1_alpha, RetVal.HSS1_beta, RetVal.HSS1_gamma);
    OS_printf("(Real) alpha-beta-gamma: %f-%f-%f\n", RetVal.HSS1_alpha*0.01, RetVal.HSS1_beta*0.01, RetVal.HSS1_gamma*0.01);

    OS_printf("\n");
    OS_printf("[MAG0]\n");
    OS_printf("(RAW)  alpha-beta-gamma: %d-%d-%d\n", RetVal.MAG0_alpha, RetVal.MAG0_beta, RetVal.MAG0_gamma);
    OS_printf("(Real) alpha-beta-gamma: %f-%f-%f\n", RetVal.MAG0_alpha*0.01, RetVal.MAG0_beta*0.01, RetVal.MAG0_gamma*0.01);

    OS_printf("\n");
    OS_printf("[MAG1]\n");
    OS_printf("(RAW)  alpha-beta-gamma: %d-%d-%d\n", RetVal.MAG1_alpha, RetVal.MAG1_beta, RetVal.MAG1_gamma);
    OS_printf("(Real) alpha-beta-gamma: %f-%f-%f\n", RetVal.MAG1_alpha*0.01, RetVal.MAG1_beta*0.01, RetVal.MAG1_gamma*0.01);


    OS_printf("\n");
    OS_printf("[STR0]\n");
    OS_printf("(RAW)  alpha-beta-gamma: %d-%d-%d\n", RetVal.STR0_alpha, RetVal.STR0_beta, RetVal.STR0_gamma);
    OS_printf("(Real) alpha-beta-gamma: %f-%f-%f\n", RetVal.STR0_alpha*0.01, RetVal.STR0_beta*0.01, RetVal.STR0_gamma*0.01);

    OS_printf("\n");
    OS_printf("[STR1]\n");
    OS_printf("(RAW)  alpha-beta-gamma: %d-%d-%d\n", RetVal.STR1_alpha, RetVal.STR1_beta, RetVal.STR1_gamma);
    OS_printf("(Real) alpha-beta-gamma: %f-%f-%f\n", RetVal.STR1_alpha*0.01, RetVal.STR1_beta*0.01, RetVal.STR1_gamma*0.01);

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetMag1MMTCalibConfigCmd(void) {
    // ID 194
    CFE_Status_t               status;
    ADCS_Mag1MMTCalibConfigTlm_Payload_t RetVal = {0,};

    status = ADCS_GetMag1MMTCalibConfig(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_MAG1_MMT_CALIB_CONFIG_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get MAG1 MMT Calibration Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("[Offset]\n");
    OS_printf("(RAW)  Channel 1: %d || Channel 2: %d || Channel 3: %d\n", RetVal.MMT_Ch1Offset, RetVal.MMT_Ch2Offset, RetVal.MMT_Ch3Offset);
    OS_printf("(Real) Channel 1: %f || Channel 2: %f || Channel 3: %f\n", RetVal.MMT_Ch1Offset*0.001, RetVal.MMT_Ch2Offset*0.001, RetVal.MMT_Ch3Offset*0.001);
    OS_printf("\n");
    OS_printf("[Sensitivity Matrix]\n");
    OS_printf("(RAW)\n");
    OS_printf("S11: %d || S12: %d || S13: %d\n", RetVal.MMT_SensitivityMAT_S11, RetVal.MMT_SensitivityMAT_S12, RetVal.MMT_SensitivityMAT_S13);
    OS_printf("S21: %d || S22: %d || S23: %d\n", RetVal.MMT_SensitivityMAT_S21, RetVal.MMT_SensitivityMAT_S22, RetVal.MMT_SensitivityMAT_S23);
    OS_printf("S31: %d || S32: %d || S33: %d\n", RetVal.MMT_SensitivityMAT_S31, RetVal.MMT_SensitivityMAT_S32, RetVal.MMT_SensitivityMAT_S33);
    OS_printf("(Real)\n");
    OS_printf("S11: %f || S12: %f || S13: %f\n", RetVal.MMT_SensitivityMAT_S11*0.001, RetVal.MMT_SensitivityMAT_S12*0.001, RetVal.MMT_SensitivityMAT_S13*0.001);
    OS_printf("S21: %f || S22: %f || S23: %f\n", RetVal.MMT_SensitivityMAT_S21*0.001, RetVal.MMT_SensitivityMAT_S22*0.001, RetVal.MMT_SensitivityMAT_S23*0.001);
    OS_printf("S31: %f || S32: %f || S33: %f\n", RetVal.MMT_SensitivityMAT_S31*0.001, RetVal.MMT_SensitivityMAT_S32*0.001, RetVal.MMT_SensitivityMAT_S33*0.001);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetEstimatorConfigCmd(void) {
    // ID 195
    CFE_Status_t               status;
    ADCS_EstimatorConfigTlm_Payload_t RetVal = {0,};

    status = ADCS_GetEstimatorConfig(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_ESTIMATOR_CONFIG_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Estimator Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("[Default Estimator Mode]\n");
    OS_printf("Main / Backup: %u / %u\n", RetVal.DefaultMainEstimatorMode, RetVal.DefaultBackupEstimatorMode);

    OS_printf("[Measurement Noise]\n");
    OS_printf("MAG: %f\n", RetVal.MAGMeasurementNoise);
    OS_printf("CSS: %f\n", RetVal.CSSMeasurementNoise);
    OS_printf("FSS: %f\n", RetVal.FSSMeasurementNoise);
    OS_printf("HSS: %f\n", RetVal.HSSMeasurementNoise);
    OS_printf("STR: %f\n", RetVal.STRMeasurementNoise);
    OS_printf("MMTRKF: %f\n", RetVal.MMTRKFSystemNoise);
    OS_printf("EKFSys: %f\n", RetVal.EKFSystemNoise);

    OS_printf("[Nutation]\n");
    OS_printf("Nut Eps: %f\n", RetVal.NutationEpsilonCorrection);
    OS_printf("Nut Psi: %f\n", RetVal.NutationPsiCorrection);

    OS_printf("[Use Sensors in EKF]\n");
    OS_printf("FSS: %u\n", RetVal.UseFSSinEKF);
    OS_printf("CSS: %u\n", RetVal.UseFSSinEKF);
    OS_printf("HSS: %u\n", RetVal.UseFSSinEKF);
    OS_printf("STR: %u\n", RetVal.UseFSSinEKF);
    
    OS_printf("[Triad Vector]\n");
    OS_printf("Vector 1 / 2: %u / %u\n", RetVal.TriadVector1, RetVal.TriadVector2);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetSatOrbitParamConfigCmd(void) {
    // ID 196
    CFE_Status_t               status;
    ADCS_SatOrbitParamConfigTlm_Payload_t RetVal = {0,};

    status = ADCS_GetSatOrbitParamConfig(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_SAT_ORBIT_PARAM_CONFIG_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Satellite Orbit Param Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("Epoch: %lf || Inclination: %lf || RAAN: %lf || Eccenctricity: %lf\n", RetVal.Epoch, RetVal.Inclination, RetVal.RAAN, RetVal.Eccentricity);
    OS_printf("AOP: %lf || Mean anomaly: %lf || Mean Motion: %lf || B_starDrag: %lf\n", RetVal.AOP, RetVal.MeanAnomaly, RetVal.MeanMotion, RetVal.B_StarDrag);
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetNodeSelectionConfigCmd(void) {
    // ID 197
    CFE_Status_t               status;
    ADCS_NodeSelectionConfigTlm_Payload_t RetVal = {0,};

    status = ADCS_GetNodeSelectionConfig(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_NODE_SELECTION_CONFIG_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Node Selection Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    
    OS_printf("RWA: ");
    for (int i = 7; i >= 0; i--) {
        if (RetVal.RWLSelectionFlags & (1 << i)) {
            OS_printf("1");
        } else {
            OS_printf("0");
        }
    }
    OS_printf("\n");
    OS_printf("MAG: ");
    for (int i = 7; i >= 0; i--) {
        if (RetVal.MAGSelectionFlags & (1 << i)) {
            OS_printf("1");
        } else {
            OS_printf("0");
        }
    }
    OS_printf("\n");
    OS_printf("FSS: ");
    for (int i = 7; i >= 0; i--) {
        if (RetVal.FSSSelectionFlags & (1 << i)) {
            OS_printf("1");
        } else {
            OS_printf("0");
        }
    }
    OS_printf("\n");
    OS_printf("HSS: ");
    for (int i = 7; i >= 0; i--) {
        if (RetVal.HSSSelectionFlags & (1 << i)) {
            OS_printf("1");
        } else {
            OS_printf("0");
        }
    }
    OS_printf("\n");
    OS_printf("GYR: ");
    for (int i = 7; i >= 0; i--) {
        if (RetVal.GYRSelectionFlags & (1 << i)) {
            OS_printf("1");
        } else {
            OS_printf("0");
        }
    }
    OS_printf("\n");
    OS_printf("STR: ");
    for (int i = 7; i >= 0; i--) {
        if (RetVal.STRSelectionFlags & (1 << i)) {
            OS_printf("1");
        } else {
            OS_printf("0");
        }
    }
    OS_printf("\n");
    OS_printf("GNSS: ");
    for (int i = 7; i >= 0; i--) {
        if (RetVal.GNSSSelectionFlags & (1 << i)) {
            OS_printf("1");
        } else {
            OS_printf("0");
        }
    }
    OS_printf("\n");
    OS_printf("EXT: ");
    for (int i = 7; i >= 0; i--) {
        if (RetVal.ExtSensorSelectionFlags & (1 << i)) {
            OS_printf("1");
        } else {
            OS_printf("0");
        }
    }
    OS_printf("\n");
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetMTQConfigCmd(void) {
    // ID 198
    CFE_Status_t               status;
    ADCS_MTQConfigTlm_Payload_t RetVal = {0,};

    status = ADCS_GetMTQConfig(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_MTQ_CONFIG_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get MTQ Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("[Maximum Dipole Moment (Am2)]\n");
    OS_printf("MTQ0: %f || MTQ1: %f || MTQ2: %f\n", RetVal.MTQ0MaxDipoleMoment, RetVal.MTQ1MaxDipoleMoment, RetVal.MTQ2MaxDipoleMoment);
    OS_printf("[MTQ On Time (ms)]\n");
    OS_printf("Max: %u || Min: %u\n", RetVal.MaxMTQOnTime, RetVal.MinMTQOnTime);
    OS_printf("Control Filter Factor: %f\n", RetVal.MagneticControlFilterFactor);
        
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetEstimationModeCmd(void) {
    // ID 199
    CFE_Status_t               status;
    ADCS_EstimationModeTlm_Payload_t RetVal = {0,};

    status = ADCS_GetEstimationMode(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_ESTIMATION_MODE_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Estimation Mode: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("[Estimator Mode]\n");
    OS_printf("Main / Backup: %u / %u\n", RetVal.MainEstimatorMode, RetVal.BackupEstimatorMode);

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetOperationalStateCmd(void) {
    // ID 200
    CFE_Status_t               status;
    ADCS_OperationalStateTlm_Payload_t RetVal = {0,};

    status = ADCS_GetOperationalState(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_OPERATIONAL_STATE_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Operational State: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("Operational Mode: %u\n", RetVal.OperationalMode);

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetRawCSSSensorCmd(void) {
    // ID 203
    CFE_Status_t               status;
    ADCS_RawCSSSensorTlm_Payload_t RetVal = {0,};

    status = ADCS_GetRawCSSSensor(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_RAW_CSS_SENSOR_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Raw CSS Sensor: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("TimeSec : %u || TimeNanoSec : %u\n", RetVal.TimeSeconds, RetVal.TimeNanoSeconds);
    OS_printf("CSS 0 : %u, 1 : %u, 2 : %u, 3 : %u, 4 : %u, 5 : %u, 6 : %u, 7 : %u, 8 : %u, 9 : %u",
                RetVal.CSS0, RetVal.CSS1, RetVal.CSS2, RetVal.CSS3, RetVal.CSS4, RetVal.CSS5, RetVal.CSS6, RetVal.CSS7, RetVal.CSS8, RetVal.CSS9);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetRawGYRSensorCmd(void) {
    // ID 204
    CFE_Status_t               status;
    ADCS_RawGYRSensorTlm_Paylaod_t RetVal = {0,};

    status = ADCS_GetRawGYRSensor(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_RAW_GYR_SENSOR_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Raw GYR Sensor: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("TimeSec : %u || TimeNanoSec : %u\n", RetVal.TimeSeconds, RetVal.TimeNanoSeconds);
    OS_printf("GYR0 RawRate X: %f || RawRate Y: %f || RawRate Z: %f\n", RetVal.GYR0RawRateX, RetVal.GYR0RawRateY, RetVal.GYR0RawRateZ);
    OS_printf("GYR1 RawRate X: %f || RawRate Y: %f || RawRate Z: %f\n", RetVal.GYR1RawRateX, RetVal.GYR1RawRateY, RetVal.GYR1RawRateZ);
    OS_printf("Valid Flag GYR0 : 0x%02X\n", RetVal.GYR0ValidFlag);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetCalibratedGYRSensorCmd(void) {
    // ID 207
    CFE_Status_t               status;
    ADCS_CalibratedGYRSensorTlm_Payload_t RetVal = {0,};

    status = ADCS_GetCalibratedGYRSensor(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_CALIBRATED_GYR_SENSOR_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Calibrated GYR Sensor: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("TimeSec : %u || TimeNanoSec : %u\n", RetVal.TimeSeconds, RetVal.TimeNanoSeconds);
    OS_printf("GYR0 Cal Rate X: %f || Cal Rate Y: %f || Cal Rate Z: %f\n", RetVal.GYR0CalibratedRateX, RetVal.GYR0CalibratedRateY, RetVal.GYR0CalibratedRateZ);
    OS_printf("GYR1 Cal Rate X: %f || Cal Rate Y: %f || Cal Rate Z: %f\n", RetVal.GYR1CalibratedRateX, RetVal.GYR1CalibratedRateY, RetVal.GYR1CalibratedRateZ);
    OS_printf("Ext GYR0 Cal Rate X: %f || Cal Rate Y: %f || Cal Rate Z: %f\n", RetVal.ExtGYR0CalibratedRateX, RetVal.ExtGYR0CalibratedRateY, RetVal.ExtGYR0CalibratedRateZ);
    OS_printf("Ext GYR1 Cal Rate X: %f || Cal Rate Y: %f || Cal Rate Z: %f\n", RetVal.ExtGYR1CalibratedRateX, RetVal.ExtGYR1CalibratedRateY, RetVal.ExtGYR1CalibratedRateZ);
    OS_printf("Valid flag GYR0 : 0x%02X\n", RetVal.GYR0ValidFlag);    
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetMagSensingElmConfigCmd(void) {
    // ID 221
    CFE_Status_t               status;
    ADCS_MagSensingElmConfigTlm_Payload_t RetVal = {0,};

    status = ADCS_GetMagSensingElmConfig(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_MAG_SENSING_ELM_CONFIG_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Mag Sensing Element Config: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("Mag0SensingElement : %u || Mag0SensingElement : %u\n", RetVal.Mag0SensingElement, RetVal.Mag1SensingElement);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetTlmLogInclMaskCmd(void) {
    // ID 227
    CFE_Status_t               status;
    ADCS_TlmLogInclMaskTlm_Payload_t RetVal = {0,};

    status = ADCS_GetTlmLogInclMask(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_TLM_LOG_INCLMASK_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Telemetry Inclution BitMask: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("[Fast Inclusion Bitmask]\n");
    for (int j = 0; j < 5; j++) {
        OS_printf("%d: ",j);
        for (int i = 7; i >= 0; i--) {
            if (RetVal.FastInclusionBitmask[j] & (1 << i)) {
                OS_printf("1");
            } else {
                OS_printf("0");
            }
        }
        OS_printf("\n");
    }
    OS_printf("\n");
	OS_printf("[Slow Inclusion Bitmask]\n");
    for (int j = 0; j < 5; j++) {
        OS_printf("%d: ",j);
        for (int i = 7; i >= 0; i--) {
            if (RetVal.SlowInclusionBitmask[j] & (1 << i)) {
                OS_printf("1");
            } else {
                OS_printf("0");
            }
        }
        OS_printf("\n");
    }
    OS_printf("\n");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetUnsolicitTlmMsgSetupCmd(void) {
    // ID 228
    CFE_Status_t               status;
    ADCS_UnsolicitTlmMsgSetupTlm_Payload_t RetVal = {0,};

    status = ADCS_GetUnsolicitTlmMsgSetup(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_UNSOLICIT_TLM_MSG_SETUP_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Unsolicit Telemetry Message Setup: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("[CAN Telemetry ID Inclusion Bitmask]\n");
    for (int j = 0; j < 5; j++) {
        OS_printf("%d: ",j);
        for (int i = 7; i >= 0; i--) {
            if (RetVal.CANTlmIDInclusionBitmask[j] & (1 << i)) {
                OS_printf("1");
            } else {
                OS_printf("0");
            }
        }
        OS_printf("\n");
    }
    OS_printf("\n");

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetUnsolicitEventMsgSetupCmd(void) {
    // ID 233
    CFE_Status_t               status;
    ADCS_UnsolicitEventMsgSetupTlm_Payload_t RetVal = {0,};

    status = ADCS_GetUnsolicitEventMsgSetup(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_UNSOLICIT_EVENT_MSG_SETUP_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Unsolicit Event Message Setup: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("[CAN Event ID Inclusion Bitmask]\n");
    OS_printf("Info Events         : %u\n", RetVal.InfoCAN);
    OS_printf("Minor Warning Events: %u\n", RetVal.MinorCAN);
    OS_printf("Major Warning Events: %u\n", RetVal.MajorCAN);
    OS_printf("Critical Evetns     : %u\n", RetVal.CriticalCAN);

    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetEventLogStatusResponseCmd(void) {
    // ID 235
    CFE_Status_t               status;
    ADCS_EventLogStatusResponseTlm_Payload_t RetVal = {0,};

    status = ADCS_GetEventLogStatusResponse(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_EVENT_LOG_STATUS_RESPONSE_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Event Log Status Response: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("Number of queued entries      : %u\n", RetVal.NumQueuedEntry);
    OS_printf("Number of buffered entries    : %u\n", RetVal.NumBufferedEntry);
    OS_printf("Number of entries             : %u\n", RetVal.NumEntry);
    OS_printf("Number of empty entries       : %u\n", RetVal.NumEmptyEntry);
    OS_printf("Oldest entry unix time        : %u\n", RetVal.OldEntryUnixTime);
    OS_printf("Latest entry unix time        : %u\n", RetVal.LastEntryUnixTime);
    OS_printf("Number of critical events     : %u\n", RetVal.NumCriticalEVS);
    OS_printf("Number of major warning events: %u\n", RetVal.NumMajorWarningEVS);
    OS_printf("Number of minor warning events: %u\n", RetVal.NumMinorWarningEVS);
    OS_printf("Number of info events         : %u\n", RetVal.NumInfoEVS);
    OS_printf("Write counter                 : %u\n", RetVal.WriteCnt);
    OS_printf("Read-Queue state              : %u\n", RetVal.ReadQueState);
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_GetPortMapCmd(void) {
    // ID 239
    CFE_Status_t               status;
    ADCS_PortMapTlm_Payload_t RetVal = {0,};

    status = ADCS_GetPortMap(&RetVal);

    ADCS_HandleReport(status, ADCS_GET_PORTMAP_CC, &RetVal, sizeof(RetVal));

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Port Map: 0x%08lx", (unsigned long)status);
        return status;
    }
    
    // Handling Retval
    OS_printf("[Sensors]\n");
    OS_printf("|Sensor #|Node Type|Abstract Node Type|Serial Number Integer|Address (CAN)|\n");
    OS_printf("|1|%u|%u|%u|%u|\n", RetVal.NodeType_Sensor1, RetVal.AbstNodeType_Sensor1, RetVal.SerialNum_Sensor1, RetVal.Address_Sensor1);
    OS_printf("|2|%u|%u|%u|%u|\n", RetVal.NodeType_Sensor2, RetVal.AbstNodeType_Sensor2, RetVal.SerialNum_Sensor2, RetVal.Address_Sensor2);
    OS_printf("|3|%u|%u|%u|%u|\n", RetVal.NodeType_Sensor3, RetVal.AbstNodeType_Sensor3, RetVal.SerialNum_Sensor3, RetVal.Address_Sensor3);
    OS_printf("|4|%u|%u|%u|%u|\n", RetVal.NodeType_Sensor4, RetVal.AbstNodeType_Sensor4, RetVal.SerialNum_Sensor4, RetVal.Address_Sensor4);
    OS_printf("|5|%u|%u|%u|%u|\n", RetVal.NodeType_Sensor5, RetVal.AbstNodeType_Sensor5, RetVal.SerialNum_Sensor5, RetVal.Address_Sensor5);
    OS_printf("|6|%u|%u|%u|%u|\n", RetVal.NodeType_Sensor6, RetVal.AbstNodeType_Sensor6, RetVal.SerialNum_Sensor6, RetVal.Address_Sensor6);
    OS_printf("|7|%u|%u|%u|%u|\n", RetVal.NodeType_Sensor7, RetVal.AbstNodeType_Sensor7, RetVal.SerialNum_Sensor7, RetVal.Address_Sensor7);
    OS_printf("|8|%u|%u|%u|%u|\n", RetVal.NodeType_Sensor8, RetVal.AbstNodeType_Sensor8, RetVal.SerialNum_Sensor8, RetVal.Address_Sensor8);
    OS_printf("\n");
    OS_printf("[Wheels]\n");
    OS_printf("|Wheel #|Node Type|Abstract Node Type|Serial Number Integer|Address (CAN)|\n");
    OS_printf("|1|%u|%u|%u|%u|\n", RetVal.NodeType_Wheel1, RetVal.AbstNodeType_Wheel1, RetVal.SerialNum_Wheel1, RetVal.Address_Wheel1);
    OS_printf("|2|%u|%u|%u|%u|\n", RetVal.NodeType_Wheel2, RetVal.AbstNodeType_Wheel2, RetVal.SerialNum_Wheel2, RetVal.Address_Wheel2);
    OS_printf("|3|%u|%u|%u|%u|\n", RetVal.NodeType_Wheel3, RetVal.AbstNodeType_Wheel3, RetVal.SerialNum_Wheel3, RetVal.Address_Wheel3);
    OS_printf("|4|%u|%u|%u|%u|\n", RetVal.NodeType_Wheel4, RetVal.AbstNodeType_Wheel4, RetVal.SerialNum_Wheel4, RetVal.Address_Wheel4);
    
    
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SetErrorLogClearCmd(const ADCS_ErrorLogClearCmd_t *msg) {
    // ID 5
    CFE_Status_t               status;

	ADCS_UnsolicitTlmMsgSetupCmd_Payload_t SetVal_112 = {0,};
	SetVal_112.CANTlmRetrunInterval = 0;				// Return interval 1s
    SetVal_112.CANTlmEDInclusionBitmask[2] = 0;			// 0b0000 0001
    status = ADCS_SetUnsolicitTlmMsgSetup(&SetVal_112);
    

    status = ADCS_SetErrorLogClear();

    ADCS_HandleReport(status, ADCS_SET_ERROR_LOG_CLEAR_CC, NULL, 0);

    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Error Log Clear: 0x%08lx", (unsigned long)status);
        return status;
    }
    OS_printf("ADCS cmd Success.");

    return CFE_SUCCESS;
}

/* CFE_Status_t ADCS_SequenceCmd_Detumbling(void) */
// Simple Version
CFE_Status_t ADCS_SequenceCmd_Detumbling(void) {	// Detumbling w/o Commissioning
	CFE_Status_t status;

	FILE *fp;
	fp = fopen("./cf/adcs_contmode.txt","w");
	fprintf(fp,"%d", ADCS_SEQ_DTUMB_CC);
	fclose(fp);

    // Power ON: MMT & GYRO
	ADCS_PowerStateTlm_Payload_t RetVal_183 = {0,};
	ADCS_PowerStateCmd_Payload_t SetVal_56 = {0,};

	status = ADCS_GetPowerState(&RetVal_183);
	if (status == CUBEOBC_ERROR_OK) {
		if (sizeof(ADCS_PowerStateCmd_Payload_t) == sizeof(ADCS_PowerStateTlm_Payload_t)) {
			memcpy(&SetVal_56, &RetVal_183, sizeof(ADCS_PowerStateTlm_Payload_t));
			SetVal_56.GYR0 = 1;
			SetVal_56.MAG0 = 1;
		}
		else {
			SetVal_56.GYR0 = 1;
			SetVal_56.MAG0 = 1;
		}
		
	}	
	status = ADCS_SetPowerState(&SetVal_56);
    // ADCS_HandleReport(status, ADCS_SET_POWER_STATE_CC, NULL, 0);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Power State: 0x%08lx", (unsigned long)status);
        return status;
    }

	// Estimation & Control Mode: EstGyro & ConBdot3
	ADCS_ControllerConfig_Payload_t SetVal_62 = {0,};
	ADCS_ControllerConfigTlm_Payload_t RetVal_190 = {0,};
	ADCS_ControlEstimationModeCmd_Payload_t SetVal_42 = {0,};

	status = ADCS_GetControllerConfig(&RetVal_190);
	if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Control Config: 0x%08lx", (unsigned long)status);
        return status;
    }
	memcpy(&SetVal_62, &RetVal_190, sizeof(ADCS_ControllerConfigTlm_Payload_t));
	SetVal_62.DefaultControlMode = 3;
	SetVal_62.flags.EnableSunTrackingInEclipse = 1;
	SetVal_62.flags.EnableSunAvoidance = 0;
	SetVal_42.MainEstimatorMode = 1;
	SetVal_42.ControlMode = 3;
	SetVal_42.ControlTimeout = 0;
	status = ADCS_SetControllerConfig(&SetVal_62);
	OS_TaskDelay(10);
	status = ADCS_SetControlEstimationMode(&SetVal_42);
	OS_TaskDelay(10);
	// ADCS_HandleReport(status, ADCS_SET_CONTROL_ESTIMATION_MODE_CC, NULL, 0);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Operate Detumbling: 0x%08lx", (unsigned long)status);
        return status;
    }

    OS_printf("ADCS cmd Success.");
    return CFE_SUCCESS;
}

// Complex Version
/*
CFE_Status_t ADCS_SequenceCmd_Detumbling(void) {
	CFE_Status_t status;
    status = ADCS_COMM_InitAngRateEst();
	if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to COMMISSIONING: 0x%08lx", (unsigned long)status);
        return status;
    }
    return CFE_SUCCESS;
}
*/


/* CFE_Status_t ADCS_SequenceCmd_Sunpointing(void) */
// Simple Version
CFE_Status_t ADCS_SequenceCmd_Sunpointing(void) {	// Sunpointing w/o Commissioning
	CFE_Status_t status;

	FILE *fp;
	fp = fopen("./cf/adcs_contmode.txt","w");
	fprintf(fp,"%d",ADCS_SEQ_SUNPT_CC);
	fclose(fp);

    // Power ON: Whole H/W
	ADCS_PowerStateCmd_Payload_t SetVal_56 = {0,};

	SetVal_56.GYR0 = 1;
	SetVal_56.MAG0 = 1;
	SetVal_56.FSS0 = 1;
	SetVal_56.HSS0 = 1;
	SetVal_56.RWL0 = 1;
	SetVal_56.RWL1 = 1;
	SetVal_56.RWL2 = 1;
	SetVal_56.RWL3 = 1;
	status = ADCS_SetPowerState(&SetVal_56);
    // ADCS_HandleReport(status, ADCS_SET_POWER_STATE_CC, NULL, 0);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Power State: 0x%08lx", (unsigned long)status);
        return status;
    }

	// Estimation & Control Mode: EstGyroEkf (EstFullEkf) & ConSunTrack
	ADCS_ControllerConfig_Payload_t SetVal_62 = {0,};
	ADCS_ControllerConfigTlm_Payload_t RetVal_190 = {0,};
	ADCS_ControlEstimationModeCmd_Payload_t SetVal_42 = {0,};

	status = ADCS_GetControllerConfig(&RetVal_190);
	if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Control Config: 0x%08lx", (unsigned long)status);
        return status;
    }
	memcpy(&SetVal_62, &RetVal_190, sizeof(ADCS_ControllerConfigTlm_Payload_t));
	SetVal_62.DefaultControlMode = 13;
	SetVal_62.flags.EnableSunTrackingInEclipse = 1;
	SetVal_62.flags.EnableSunAvoidance = 0;
	SetVal_42.MainEstimatorMode = 6;
	SetVal_42.BackupEstimatorMode = 5;
	SetVal_42.ControlMode = 13;
	SetVal_42.ControlTimeout = 0;
	status = ADCS_SetControllerConfig(&SetVal_62);
	OS_TaskDelay(10);
	status = ADCS_SetControlEstimationMode(&SetVal_42);
	OS_TaskDelay(10);
	// ADCS_HandleReport(status, ADCS_SET_CONTROL_ESTIMATION_MODE_CC, NULL, 0);
	
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Operate  Sun Pointing: 0x%08lx", (unsigned long)status);
        return status;
    }

	ADCS_HandleReport(status, ADCS_SEQ_SUNPT_CC, NULL, 0);

    OS_printf("ADCS cmd Success.");
    return CFE_SUCCESS;
}

// Complex Version
/*
CFE_Status_t ADCS_SequenceCmd_Sunpointing(void) {
	CFE_Status_t status;
    status = ADCS_COMM_InitAngRateEst();
	if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to COMMISSIONING: 0x%08lx", (unsigned long)status);
        return status;
    }
    return CFE_SUCCESS;
}
*/

/* Other Pointing Commands */
CFE_Status_t ADCS_SequenceCmd_Vpointing(void) {	// Velocity vector pointing w/o Commissioning
	CFE_Status_t status;

	FILE *fp;
	fp = fopen("./cf/adcs_contmode.txt","w");
	fprintf(fp,"%d",ADCS_SEQ_VELPT_CC);
	fclose(fp);

    // Power ON: Whole H/W
	ADCS_PowerStateCmd_Payload_t SetVal_56 = {0,};

	SetVal_56.GYR0 = 1;
	SetVal_56.MAG0 = 1;
	SetVal_56.FSS0 = 1;
	SetVal_56.HSS0 = 1;
	SetVal_56.RWL0 = 1;
	SetVal_56.RWL1 = 1;
	SetVal_56.RWL2 = 1;
	SetVal_56.RWL3 = 1;
	status = ADCS_SetPowerState(&SetVal_56);
    // ADCS_HandleReport(status, ADCS_SET_POWER_STATE_CC, NULL, 0);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Power State: 0x%08lx", (unsigned long)status);
        return status;
    }

	// Estimation & Control Mode: EstGyroEkf (EstFullEkf) & ConSunTrack
	ADCS_ControllerConfig_Payload_t SetVal_62 = {0,};
	ADCS_ControllerConfigTlm_Payload_t RetVal_190 = {0,};
	ADCS_ControlEstimationModeCmd_Payload_t SetVal_42 = {0,};
	ADCS_ReferenceRPYvaluesCmd_Payload_t SetVal_54 = {0,};

	status = ADCS_GetControllerConfig(&RetVal_190);
	if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Control Config: 0x%08lx", (unsigned long)status);
        return status;
    }
	memcpy(&SetVal_62, &RetVal_190, sizeof(ADCS_ControllerConfigTlm_Payload_t));
	SetVal_62.DefaultControlMode = 13;
	SetVal_62.flags.EnableSunTrackingInEclipse = 1;
	SetVal_62.flags.EnableSunAvoidance = 0;
	SetVal_42.MainEstimatorMode = 6;
	SetVal_42.BackupEstimatorMode = 5;
	SetVal_42.ControlMode = 12;
	// SetVal_42.ControlTimeout = 20;
	SetVal_42.ControlTimeout = 900;

	SetVal_54.Pitch = -90.0;
	SetVal_54.Roll = 0.0;
	SetVal_54.Yaw = 0.0;

	OS_printf("Default Control mode: %u\n", SetVal_62.DefaultControlMode);

	status = ADCS_SetReferenceRPYValues(&SetVal_54);
	OS_TaskDelay(10);
	status = ADCS_SetControllerConfig(&SetVal_62);
	OS_TaskDelay(10);
	status = ADCS_SetControlEstimationMode(&SetVal_42);
	OS_TaskDelay(10);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Operate  Velocity Pointing: 0x%08lx", (unsigned long)status);
        return status;
    }

	ADCS_HandleReport(status, ADCS_SEQ_VELPT_CC, NULL, 0);
	
    OS_printf("ADCS cmd Success.");
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SequenceCmd_KSCpointing(void) {	// KissCAM EARTH pointing w/o Commissioning
	CFE_Status_t status;

	FILE *fp;
	fp = fopen("./cf/adcs_contmode.txt","w");
	fprintf(fp,"%d",ADCS_SEQ_KSCPT_CC);
	fclose(fp);

    // Power ON: Whole H/W
	ADCS_PowerStateCmd_Payload_t SetVal_56 = {0,};

	SetVal_56.GYR0 = 1;
	SetVal_56.MAG0 = 1;
	SetVal_56.FSS0 = 1;
	SetVal_56.HSS0 = 1;
	SetVal_56.RWL0 = 1;
	SetVal_56.RWL1 = 1;
	SetVal_56.RWL2 = 1;
	SetVal_56.RWL3 = 1;
	status = ADCS_SetPowerState(&SetVal_56);
    // ADCS_HandleReport(status, ADCS_SET_POWER_STATE_CC, NULL, 0);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Power State: 0x%08lx", (unsigned long)status);
        return status;
    }

	// Estimation & Control Mode: EstGyroEkf (EstFullEkf) & ConSunTrack
	ADCS_ControllerConfig_Payload_t SetVal_62 = {0,};
	ADCS_ControllerConfigTlm_Payload_t RetVal_190 = {0,};
	ADCS_ControlEstimationModeCmd_Payload_t SetVal_42 = {0,};
	ADCS_ReferenceRPYvaluesCmd_Payload_t SetVal_54 = {0,};

	status = ADCS_GetControllerConfig(&RetVal_190);
	if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Control Config: 0x%08lx", (unsigned long)status);
        return status;
    }
	memcpy(&SetVal_62, &RetVal_190, sizeof(ADCS_ControllerConfigTlm_Payload_t));
	SetVal_62.DefaultControlMode = 13;
	SetVal_62.flags.EnableSunTrackingInEclipse = 1;
	SetVal_62.flags.EnableSunAvoidance = 0;
	SetVal_42.MainEstimatorMode = 6;
	SetVal_42.BackupEstimatorMode = 5;
	SetVal_42.ControlMode = 12;
	// SetVal_42.ControlTimeout = 60;
	SetVal_42.ControlTimeout = 900;

	SetVal_54.Pitch = 90.0;
	SetVal_54.Roll = 0.0;
	SetVal_54.Yaw = 0.0;

	status = ADCS_SetReferenceRPYValues(&SetVal_54);
	OS_TaskDelay(10);
	status = ADCS_SetControllerConfig(&SetVal_62);
	OS_TaskDelay(10);
	status = ADCS_SetControlEstimationMode(&SetVal_42);
	OS_TaskDelay(10);
	// ADCS_HandleReport(status, ADCS_SET_CONTROL_ESTIMATION_MODE_CC, NULL, 0);
	
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Operate  KisCAM Earth Pointing: 0x%08lx", (unsigned long)status);
        return status;
    }

	ADCS_HandleReport(status, ADCS_SEQ_KSCPT_CC, NULL, 0);
	
    OS_printf("ADCS cmd Success.");
    return CFE_SUCCESS;
}

CFE_Status_t ADCS_SequenceCmd_LGCpointing(void) {	// LG CAM EARTH pointing w/o Commissioning
	CFE_Status_t status;

	FILE *fp;
	fp = fopen("./cf/adcs_contmode.txt","w");
	fprintf(fp,"%d",ADCS_SEQ_LGCPT_CC);
	fclose(fp);

    // Power ON: Whole H/W
	ADCS_PowerStateCmd_Payload_t SetVal_56 = {0,};

	SetVal_56.GYR0 = 1;
	SetVal_56.MAG0 = 1;
	SetVal_56.FSS0 = 1;
	SetVal_56.HSS0 = 1;
	SetVal_56.RWL0 = 1;
	SetVal_56.RWL1 = 1;
	SetVal_56.RWL2 = 1;
	SetVal_56.RWL3 = 1;
	status = ADCS_SetPowerState(&SetVal_56);
    // ADCS_HandleReport(status, ADCS_SET_POWER_STATE_CC, NULL, 0);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Power State: 0x%08lx", (unsigned long)status);
        return status;
    }

	// Estimation & Control Mode: EstGyroEkf (EstFullEkf) & ConSunTrack
	ADCS_ControllerConfig_Payload_t SetVal_62 = {0,};
	ADCS_ControllerConfigTlm_Payload_t RetVal_190 = {0,};
	ADCS_ControlEstimationModeCmd_Payload_t SetVal_42 = {0,};
	ADCS_ReferenceRPYvaluesCmd_Payload_t SetVal_54 = {0,};

	status = ADCS_GetControllerConfig(&RetVal_190);
	if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Control Config: 0x%08lx", (unsigned long)status);
        return status;
    }
	memcpy(&SetVal_62, &RetVal_190, sizeof(ADCS_ControllerConfigTlm_Payload_t));
	SetVal_62.DefaultControlMode = 13;
	SetVal_62.flags.EnableSunTrackingInEclipse = 1;
	SetVal_62.flags.EnableSunAvoidance = 0;
	SetVal_42.MainEstimatorMode = 6;
	SetVal_42.BackupEstimatorMode = 5;
	SetVal_42.ControlMode = 12;
	SetVal_42.ControlTimeout = 60;
	// SetVal_42.ControlTimeout = 900;

	SetVal_54.Pitch = 0.0;
	SetVal_54.Roll = 0.0;
	SetVal_54.Yaw = 0.0;

	status = ADCS_SetReferenceRPYValues(&SetVal_54);
	OS_TaskDelay(10);
	status = ADCS_SetControllerConfig(&SetVal_62);
	OS_TaskDelay(10);
	status = ADCS_SetControlEstimationMode(&SetVal_42);
	OS_TaskDelay(10);
	// ADCS_HandleReport(status, ADCS_SET_CONTROL_ESTIMATION_MODE_CC, NULL, 0);
	
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Operate  LG CAM Earth Pointing: 0x%08lx", (unsigned long)status);
        return status;
    }

	ADCS_HandleReport(status, ADCS_SEQ_LGCPT_CC, NULL, 0);
	
    OS_printf("ADCS cmd Success.");
    return CFE_SUCCESS;
}


CFE_Status_t ADCS_SequenceCmd_RPYpointing(const ADCS_SequenceCmdRPYpointingCmd_t *msg) {	// Required RPY pointing based on GS CMD
	CFE_Status_t status;

	FILE *fp;
	fp = fopen("./cf/adcs_contmode.txt","w");
	fprintf(fp,"%d",ADCS_SEQ_RPYPT_CC);
	fclose(fp);

    // Power ON: Whole H/W
	ADCS_PowerStateCmd_Payload_t SetVal_56 = {0,};

	SetVal_56.GYR0 = 1;
	SetVal_56.MAG0 = 1;
	SetVal_56.FSS0 = 1;
	SetVal_56.HSS0 = 1;
	SetVal_56.RWL0 = 1;
	SetVal_56.RWL1 = 1;
	SetVal_56.RWL2 = 1;
	SetVal_56.RWL3 = 1;
	status = ADCS_SetPowerState(&SetVal_56);
    // ADCS_HandleReport(status, ADCS_SET_POWER_STATE_CC, NULL, 0);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Set Power State: 0x%08lx", (unsigned long)status);
        return status;
    }

	// Estimation & Control Mode: EstGyroEkf (EstFullEkf) & ConSunTrack
	ADCS_ControllerConfig_Payload_t SetVal_62 = {0,};
	ADCS_ControllerConfigTlm_Payload_t RetVal_190 = {0,};
	ADCS_ControlEstimationModeCmd_Payload_t SetVal_42 = {0,};

	status = ADCS_GetControllerConfig(&RetVal_190);
	if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Get Control Config: 0x%08lx", (unsigned long)status);
        return status;
    }
	memcpy(&SetVal_62, &RetVal_190, sizeof(ADCS_ControllerConfigTlm_Payload_t));
	SetVal_62.DefaultControlMode = 13;
	SetVal_62.flags.EnableSunTrackingInEclipse = 1;
	SetVal_62.flags.EnableSunAvoidance = 0;
	SetVal_42.MainEstimatorMode = 6;
	SetVal_42.BackupEstimatorMode = 5;
	SetVal_42.ControlMode = 12;
	// SetVal_42.ControlTimeout = 60;
	SetVal_42.ControlTimeout = 900;


	status = ADCS_SetReferenceRPYValues(&msg->Payload);
	OS_TaskDelay(10);
	status = ADCS_SetControllerConfig(&SetVal_62);
	OS_TaskDelay(10);
	status = ADCS_SetControlEstimationMode(&SetVal_42);
	OS_TaskDelay(10);
	// ADCS_HandleReport(status, ADCS_SET_CONTROL_ESTIMATION_MODE_CC, NULL, 0);
	
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Adcs App: Fail to Operate  GS-based RPY Pointing: 0x%08lx", (unsigned long)status);
        return status;
    }

	ADCS_HandleReport(status, ADCS_SEQ_LGCPT_CC, NULL, 0);
	
    OS_printf("ADCS cmd Success.");
    return CFE_SUCCESS;
}


CFE_Status_t ADCS_Loop(void)
{
    int32 status = CFE_SUCCESS;

	int contmode_save;
	uint8 contmode_curr;

	FILE *fp;
	if ((fp = fopen("./cf/adcs_contmode.txt","r"))== NULL) {  // Previous Control mode: if mode = 0; before starting detumbling (EO)
        fp = fopen("./cf/adcs_contmode.txt","w");
        fprintf(fp,"%d",0);
        fclose(fp);
    }
    else {
        fscanf(fp,"%d",&contmode_save);
        fclose(fp);
    }

	ADCS_ControlModeTlm_Payload_t RetVal_185 = {0,};
	ADCS_GetControlMode(&RetVal_185);
	contmode_curr = RetVal_185.ControlMode;

	ADCS_CalibratedGYRSensorTlm_Payload_t RetVal_207 = {0,};
	ADCS_GetCalibratedGYRSensor(&RetVal_207);
	float angvel_norm_sq = 0.0;
	angvel_norm_sq = RetVal_207.GYR0CalibratedRateX * RetVal_207.GYR0CalibratedRateX + RetVal_207.GYR0CalibratedRateY * RetVal_207.GYR0CalibratedRateY + RetVal_207.GYR0CalibratedRateZ * RetVal_207.GYR0CalibratedRateZ;

	if (contmode_curr == 0) {
		if ((contmode_save != 0) && (angvel_norm_sq > 0.5)) { // (After starting detumbling) && (|w| >0.5 [deg/s])
			status = ADCS_SequenceCmd_Detumbling();
			return CFE_SUCCESS;
		}

		// if (contmode_save == ADCS_SEQ_SUNPT_CC) {

		// }

		if (contmode_save == ADCS_SEQ_VELPT_CC) {
			status = ADCS_SequenceCmd_Sunpointing();
			return CFE_SUCCESS;
		}

		if (contmode_save == ADCS_SEQ_KSCPT_CC) {
			status = ADCS_SequenceCmd_Sunpointing();
			return CFE_SUCCESS;
		}

		if (contmode_save == ADCS_SEQ_LGCPT_CC) {
			status = ADCS_SequenceCmd_Sunpointing();
			return CFE_SUCCESS;
		}

		if (contmode_save == ADCS_SEQ_RPYPT_CC) {
			status = ADCS_SequenceCmd_Sunpointing();
			return CFE_SUCCESS;
		}

	}    

    return status;
}