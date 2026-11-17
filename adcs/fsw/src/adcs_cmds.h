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
 *   This file contains the prototypes for the Adcs App Ground Command-handling functions
 */

#ifndef ADCS_CMDS_H
#define ADCS_CMDS_H

/*
** Required header files.
*/
#include "cfe_error.h"
#include "adcs_msg.h"

CFE_Status_t ADCS_SendHkCmd(const ADCS_SendHkCmd_t *Msg);
CFE_Status_t ADCS_SendBcnCmd(const ADCS_SendBcnCmd_t *Msg);
CFE_Status_t ADCS_NoopCmd(const ADCS_NoopCmd_t *Msg);
CFE_Status_t ADCS_ResetCountersCmd(const ADCS_ResetCountersCmd_t *Msg);
/*
CFE_Status_t ADCS_EN_HighCmd(void);
CFE_Status_t ADCS_EN_LowCmd(void);
CFE_Status_t ADCS_Boot_HighCmd(void);
CFE_Status_t ADCS_Boot_LowCmd(void);
CFE_Status_t ADCS_ExitBootloader(void);
*/


/*******************************************
 * 
 * Declaration of Invoked Cmd function
 * Upper functions are just references
 * 
 *********************************************/
/* Set function */
CFE_Status_t ADCS_SetReset(void);	// 1
CFE_Status_t ADCS_SetCurrentUnixTimeCmd(const ADCS_CurrentUnixTimeCmd_t *msg);	// 2
CFE_Status_t ADCS_SetErrorLogSettingCmd(const ADCS_ErrorLogSettingCmd_t *msg);	// 6
CFE_Status_t ADCS_SetPersistConfigCmd(const ADCS_PersistConfigCmd_t *msg);	// 7
CFE_Status_t ADCS_SetControlEstimationModeCmd(const ADCS_ControlEstimationModeCmd_t *msg);	// 42
CFE_Status_t ADCS_SetDisableMagRwlMntMngCmd(const ADCS_DisableMagRwlMntMngCmd_t *msg);	// 43
CFE_Status_t ADCS_SetReferenceIRCVectorCmd(const ADCS_ReferenceIRCVectorCmd_t *msg);	// 47
CFE_Status_t ADCS_SetReferenceLLHTargetCmd(const ADCS_ReferenceLLHTargetCmd_t *msg);		// 48
CFE_Status_t ADCS_SetOrbitModeCmd(const ADCS_OrbitModeCmd_t *msg);	// 51
CFE_Status_t ADCS_SetMagDeployCmd(const ADCS_MagDeployCmd_t *msg);	// 52
CFE_Status_t ADCS_SetReferenceRPYValuesCmd(const ADCS_ReferenceRPYvaluesCmd_t *msg);	// 54
CFE_Status_t ADCS_SetOpenLoopCmdMTQCmd(const ADCS_OpenLoopCmdMTQCmd_t *msg);	// 55
CFE_Status_t ADCS_SetPowerStateCmd(const ADCS_PowerStateCmd_t *msg);	// 56
CFE_Status_t ADCS_SetRunModeCmd(const ADCS_RunModeCmd_t *msg);	// 57
CFE_Status_t ADCS_SetControlModeCmd(const ADCS_ControlModeCmd_t *msg);	// 58
CFE_Status_t ADCS_SetWhlConfigCmd(const ADCS_WhlConfigCmd_t *msg);	// 59
CFE_Status_t ADCS_SetSatelliteConfigCmd(const ADCS_SatConfigCmd_t *msg);	// 61
CFE_Status_t ADCS_SetControllerConfigCmd(const ADCS_ControllerConfig_t *msg);	// 62
CFE_Status_t ADCS_SetMag0MMTCalibConfigCmd(const ADCS_Mag0MMTCalibConfigCmd_t *msg);	// 63
CFE_Status_t ADCS_SetDefaultModeConfigCmd(const ADCS_DefaultModeConfigCmd_t *msg);	// 64
CFE_Status_t ADCS_SetMountingConfigCmd(const ADCS_MountingConfigCmd_t *msg);	// 65
CFE_Status_t ADCS_SetMag1MMTCalibConfigCmd(const ADCS_Mag1MMTCalibConfigCmd_t *msg);	// 66
CFE_Status_t ADCS_SetEstimatorConfigCmd(const ADCS_EstimatorConfigCmd_t *msg);	// 67
CFE_Status_t ADCS_SetSatOrbitParamConfigCmd(const ADCS_SatOrbitParamConfigCmd_t *msg);	// 68
CFE_Status_t ADCS_SetNodeSelectionConfigCmd(const ADCS_NodeSelectionConfigCmd_t *msg);	// 69
CFE_Status_t ADCS_SetMTQConfigCmd(const ADCS_MTQConfigCmd_t *msg);	// 70
CFE_Status_t ADCS_SetEstimationModeCmd(const ADCS_EstimationModeCmd_t *msg);	// 71
CFE_Status_t ADCS_SetOperationalStateCmd(const ADCS_OperationalStateCmd_t *msg);	// 72
CFE_Status_t ADCS_SetMagSensingElmConfigCmd(const ADCS_MagSensingElmConfigCmd_t *msg);	// 77
CFE_Status_t ADCS_SetUnsolicitTlmMsgSetupCmd(const ADCS_UnsolicitTlmMsgSetupCmd_t *msg);	// 112
CFE_Status_t ADCS_SetUnsolicitEventMsgSetupCmd(const ADCS_UnsolicitEventMsgSetupCmd_t *msg);	// 116
CFE_Status_t ADCS_SetInitiateEventLogTransferCmd(const ADCS_InitiateEventLogTransferCmd_t *msg);	// 120

/* Get function */
CFE_Status_t ADCS_GetErrorLogSettingCmd(void);	// 132
CFE_Status_t ADCS_GetCurrentUnixTimeCmd(void);	// 133
CFE_Status_t ADCS_GetCurrentUnixTimeInternalCmd(void); // 133
CFE_Status_t ADCS_GetPersistConfigDiagnosticCmd(void);	// 134
CFE_Status_t ADCS_GetCommunicationStatusCmd(void);	// 135
CFE_Status_t ADCS_GetControlEstimationModeCmd(void);	// 150
CFE_Status_t ADCS_GetReferenceIRCVectorCmd(void);	// 156
CFE_Status_t ADCS_GetReferenceLLHTargetCmd(void);	// 157
CFE_Status_t ADCS_GetOrbitModeCmd(void);	// 162
CFE_Status_t ADCS_GetHealthTlmMMTCmd(void);	// 167
CFE_Status_t ADCS_GetRawCubeSenseSunCmd(void);	// 170
CFE_Status_t ADCS_GetReferenceRPYvaluesCmd(void);	// 181
CFE_Status_t ADCS_GetOpenLoopCmdMTQCmd(void);	// 182
CFE_Status_t ADCS_GetPowerStateCmd(void);	// 183
CFE_Status_t ADCS_GetRunModeCmd(void);	// 184
CFE_Status_t ADCS_GetControlModeCmd(void);	// 185
CFE_Status_t ADCS_GetWhlConfigCmd(void);	// 186
CFE_Status_t ADCS_GetSatelliteConfigCmd(void);	// 189
CFE_Status_t ADCS_GetControllerConfigCmd(void);	// 190
CFE_Status_t ADCS_GetMag0MMTCalibConfigCmd(void);	// 191
CFE_Status_t ADCS_GetDefaultModeConfigCmd(void);	// 192
CFE_Status_t ADCS_GetMountingConfigCmd(void);	// 193
CFE_Status_t ADCS_GetMag1MMTCalibConfigCmd(void);	// 194
CFE_Status_t ADCS_GetEstimatorConfigCmd(void);	// 195
CFE_Status_t ADCS_GetSatOrbitParamConfigCmd(void);	// 196
CFE_Status_t ADCS_GetNodeSelectionConfigCmd(void);	// 197
CFE_Status_t ADCS_GetMTQConfigCmd(void);	// 198
CFE_Status_t ADCS_GetEstimationModeCmd(void);	// 199
CFE_Status_t ADCS_GetOperationalStateCmd(void);	// 200
CFE_Status_t ADCS_GetRawCSSSensorCmd(void);	// 203
CFE_Status_t ADCS_GetRawGYRSensorCmd(void);	// 204
CFE_Status_t ADCS_GetCalibratedGYRSensorCmd(void);	// 207
CFE_Status_t ADCS_GetMagSensingElmConfigCmd(void);	// 221
CFE_Status_t ADCS_GetTlmLogInclMaskCmd(void); // 227
CFE_Status_t ADCS_GetUnsolicitTlmMsgSetupCmd(void);	// 228
CFE_Status_t ADCS_GetUnsolicitEventMsgSetupCmd(void);	// 233
CFE_Status_t ADCS_GetEventLogStatusResponseCmd(void);	// 235
CFE_Status_t ADCS_GetPortMapCmd(void);  // 239

CFE_Status_t ADCS_SetErrorLogClearCmd(const ADCS_ErrorLogClearCmd_t *msg);
CFE_Status_t ADCS_SequenceCmd_Detumbling(void);
CFE_Status_t ADCS_SequenceCmd_Sunpointing(void);
CFE_Status_t ADCS_SequenceCmd_Vpointing(void);
CFE_Status_t ADCS_SequenceCmd_KSCpointing(void);
CFE_Status_t ADCS_SequenceCmd_LGCpointing(void);
CFE_Status_t ADCS_SequenceCmd_RPYpointing(const ADCS_SequenceCmdRPYpointingCmd_t *msg);

CFE_Status_t ADCS_Loop(void);

#endif /* ADCS_CMDS_H */
