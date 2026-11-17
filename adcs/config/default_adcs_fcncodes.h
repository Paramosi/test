/************************************************************************
 * NASA Docket No. GSC-18719-1 and identified as “core Flight System: Bootes”
 *
 * Copyright (c) 2020 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/**
 * @file
 *   Specification for the ADCS command function codes
 *
 * @note
 *   This file should be strictly limited to the command/function code (CC)
 *   macro definitions.  Other definitions such as enums typedefs or other
 *   macros should be placed in the msgdefs.h or msg.h files.
 */
#ifndef ADCS_FCNCODES_H
#define ADCS_FCNCODES_H

/************************************************************************
 * Macro Definitions
 ************************************************************************/

/*
** Adcs App command codes
*/
#define ADCS_NOOP_CC           0
#define ADCS_RESET_COUNTERS_CC 1

#define ADCS_RESET_APP_CMD_COUNTERS_CC          2
#define ADCS_RESET_DEVICE_CMD_COUNTERS_CC       3
#define ADCS_SET_COMMUNICATION_MODE_AS_CAN_CC   4
#define ADCS_GPIO_ENABLE_HIGH_CC 5
#define ADCS_GPIO_ENABLE_LOW_CC 6
#define ADCS_GPIO_BOOT_HIGH_CC 8
#define ADCS_GPIO_BOOT_LOW_CC 9
#define ADCS_EXIT_BOOTLOADER_CC 10

/* < ADCS Module Command Code (TC, TM) > */
/* Telecommand */
#define ADCS_SET_RESET_CC						11 // 1
#define ADCS_SET_CURRENT_UNIX_TIME_CC			12 // 2
#define ADCS_SET_ERROR_LOG_SETTING_CC			13 // 6
#define ADCS_SET_PERSIST_CONFIG_CC				14 // 7
#define ADCS_SET_CONTROL_ESTIMATION_MODE_CC		15 // 42
#define ADCS_SET_DISABLE_MAG_RWL_MNT_MNG_CC		16 // 43
#define ADCS_SET_REFERENCE_IRC_VECTOR_CC		17 // 47
#define ADCS_SET_REFERENCE_LLH_TARGET_CC		18 // 48
#define ADCS_SET_ORBIT_MODE_CC					19 // 51
#define ADCS_SET_MAG_DEPLOY_CMD_CC				20 // 52
#define ADCS_SET_REFERENCE_RPY_VALUES_CC		21 // 54
#define ADCS_SET_OPENLOOPCMD_MTQ_CC				22 // 55
#define ADCS_SET_POWER_STATE_CC					23 // 56
#define ADCS_SET_RUN_MODE_CC					24 // 57
#define ADCS_SET_CONTROL_MODE_CC				25 // 58
#define ADCS_SET_WHL_CONFIG_CC					26 // 59
#define ADCS_SET_SATELLITE_CONFIG_CC			27 // 61
#define ADCS_SET_CONTROLLER_CONFIG_CC			28 // 62
#define ADCS_SET_MAG0_MMT_CALIB_CONFIG_CC		29 // 63
#define ADCS_SET_DEFAULT_MODE_CONFIG_CC			30 // 64
#define ADCS_SET_MOUNTING_CONFIG_CC				31 // 65
#define ADCS_SET_MAG1_MMT_CALIB_CONFIG_CC		32 // 66
#define ADCS_SET_ESTIMATOR_CONFIG_CC			33 // 67
#define ADCS_SET_SAT_ORBIT_PARAMS_CONFIG_CC		34 // 68
#define ADCS_SET_NODE_SELECTION_CONFIG_CC		35 // 69
#define ADCS_SET_MTQ_CONFIG_CC					36 // 70
#define ADCS_SET_ESTIMATION_MODE_CC				37 // 71
#define ADCS_SET_OPERATIONAL_STATE_CC			38 // 72
#define ADCS_SET_MAG_SENSING_ELM_CONFIG_CC		39 // 77
#define ADCS_SET_UNSOLICIT_TLM_MSG_SETUP_CC		40 // 112
#define ADCS_SET_UNSOLICIT_EVENT_MSG_SETUP_CC	41 // 116
#define ADCS_SET_INITIATE_EVENT_LOG_TRANSFER_CC	42 // 120

/* Telemetry */
#define	ADCS_GET_ERROR_LOG_SETTING_CC			51 // 132
#define ADCS_GET_CURRENT_UNIX_TIME_CC			52 // 133
#define ADCS_GET_PERSIST_CONFIG_DIAGNOSTIC_CC	53 // 134
#define ADCS_GET_COMMUNICATION_STATUS_CC		54 // 135
#define ADCS_GET_CONTROL_ESTIMATION_MODE_CC		55 // 150
#define	ADCS_GET_REFERENCE_IRC_VECTOR_CC		56 // 156
#define ADCS_GET_REFERENCE_LLH_TARGET_CC		57 // 157
#define ADCS_GET_ORBIT_MODE_CC					58 // 162
#define	ADCS_GET_HEALTH_TLM_MMT_CC				59 // 167
#define ADCS_GET_RAW_CUBESENSE_SUN_CC			60 // 170
#define	ADCS_GET_REFERENCE_RPY_VALUES_CC		61 // 181
#define	ADCS_GET_OPENLOOPCMD_MTQ_CC				62 // 182
#define ADCS_GET_POWER_STATE_CC					63 // 183
#define ADCS_GET_RUN_MODE_CC					64 // 184
#define ADCS_GET_CONTROL_MODE_CC                65 // 185
#define	ADCS_GET_WHL_CONFIG_CC					66 // 186
#define ADCS_GET_SATELLITE_CONFIG_CC			67 // 189
#define ADCS_GET_CONTROLLER_CONFIG_CC			68 // 190
#define	ADCS_GET_MAG0_MMT_CALIB_CONFIG_CC		69 // 191
#define ADCS_GET_DEFAULT_MODE_CONFIG_CC			70 // 192
#define ADCS_GET_MOUNTING_CONFIG_CC				71 // 193
#define	ADCS_GET_MAG1_MMT_CALIB_CONFIG_CC		72 // 194
#define	ADCS_GET_ESTIMATOR_CONFIG_CC			73 // 195
#define ADCS_GET_SAT_ORBIT_PARAM_CONFIG_CC		74 // 196
#define	ADCS_GET_NODE_SELECTION_CONFIG_CC		75 // 197
#define	ADCS_GET_MTQ_CONFIG_CC					76 // 198
#define	ADCS_GET_ESTIMATION_MODE_CC				77 // 199
#define ADCS_GET_OPERATIONAL_STATE_CC			78 // 200
#define ADCS_GET_RAW_CSS_SENSOR_CC				79 // 203
#define ADCS_GET_RAW_GYR_SENSOR_CC				80 // 204
#define ADCS_GET_CALIBRATED_GYR_SENSOR_CC		81 // 207
#define	ADCS_GET_MAG_SENSING_ELM_CONFIG_CC		82 // 221
#define	ADCS_GET_TLM_LOG_INCLMASK_CC			83 // 227
#define	ADCS_GET_UNSOLICIT_TLM_MSG_SETUP_CC		84 // 228
#define ADCS_GET_UNSOLICIT_EVENT_MSG_SETUP_CC	85 // 233
#define ADCS_GET_EVENT_LOG_STATUS_RESPONSE_CC	86 // 235
#define ADCS_GET_PORTMAP_CC	                    87 // 239

#define ADCS_SEQ_DTUMB_CC						100
#define ADCS_SEQ_SUNPT_CC						101
#define ADCS_SEQ_VELPT_CC						102
#define ADCS_SEQ_KSCPT_CC						103
#define ADCS_SEQ_LGCPT_CC						104
#define ADCS_SEQ_RPYPT_CC						105

// #define ADCS_LOOPTEST_CC		106

#define ADCS_SET_ERROR_LOG_CLEAR_CC				110 // 5

#define ADCS_GET_CURRENT_UNIX_TIME_INTERNAL_CC  114 // 133

#endif