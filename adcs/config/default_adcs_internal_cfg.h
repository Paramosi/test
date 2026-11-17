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
 *   ADCS Application Private Config Definitions
 *
 * This provides default values for configurable items that are internal
 * to this module and do NOT affect the interface(s) of this module.  Changes
 * to items in this file only affect the local module and will be transparent
 * to external entities that are using the public interface(s).
 *
 * @note This file may be overridden/superceded by mission-provided defintions
 * either by overriding this header or by generating definitions from a command/data
 * dictionary tool.
 */
#ifndef ADCS_INTERNAL_CFG_H
#define ADCS_INTERNAL_CFG_H

/***********************************************************************/
#define ADCS_PIPE_DEPTH 32 /* Depth of the Command Pipe for Application */

#define ADCS_EVS_TASK_STACK_SIZE        2048
#define ADCS_EVS_TASK_STACK_PRIORITY    200

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                              EndPoint ID                                  */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * */
/* Set fucntion : ID 1 ~ 122 */
#define	ADCS_ID_SET_RESET		                    1
#define	ADCS_ID_SET_CURRENT_UNIX_TIME               2
#define ADCS_ID_SET_ERROR_LOG_SETTING				6
#define ADCS_ID_SET_PERSIST_CONFIG                  7
#define ADCS_ID_SET_CONTROL_ESTIMATION_MODE         42
#define ADCS_ID_SET_DISABLE_MAG_RWL_MNT_MNG			43
#define ADCS_ID_SET_REFERENCE_IRC_VECTOR			47
#define ADCS_ID_SET_REFERENCE_LLH_TARGET            48
#define ADCS_ID_SET_ORBIT_MODE                      51
#define ADCS_ID_SET_MAG_DEPLOY_CMD					52
#define ADCS_ID_SET_REFERENCE_RPY_VALUES            54
#define ADCS_ID_SET_OPENLOOPCMD_MTQ					55
#define ADCS_ID_SET_POWER_STATE                     56
#define ADCS_ID_SET_RUN_MODE                        57
#define ADCS_ID_SET_CONTROL_MODE					58
#define ADCS_ID_SET_WHL_CONFIG						59
#define ADCS_ID_SET_SATELLITE_CONFIG                61
#define ADCS_ID_SET_CONTROLLER_CONFIG               62
#define ADCS_ID_SET_MAG0_MMT_CALIB_CONFIG			63
#define ADCS_ID_SET_DEFAULT_MODE_CONFIG             64
#define ADCS_ID_SET_MOUNTING_CONFIG                 65
#define ADCS_ID_SET_MAG1_MMT_CALIB_CONFIG			66
#define ADCS_ID_SET_ESTIMATOR_CONFIG				67
#define ADCS_ID_SET_SAT_ORBIT_PARAM_CONFIG          68
#define ADCS_ID_SET_NODE_SELECTION_CONFIG			69
#define ADCS_ID_SET_MTQ_CONFIG						70
#define ADCS_ID_SET_ESTIMATION_MODE					71
#define ADCS_ID_SET_OPERATIONAL_STATE				72
#define ADCS_ID_SET_MAG_SENSING_ELM_CONFIG			77
#define ADCS_ID_SET_UNSOLICIT_TLM_MSG_SETUP			112
#define ADCS_ID_SET_UNSOLICIT_EVENT_MSG_SETUP       116
#define ADCS_ID_SET_INITIATE_EVENT_LOG_TRANSGER     120

/* Get fucntion : ID 128 ~ 244 */
#define	ADCS_ID_GET_ERROR_LOG_SETTING				132
#define	ADCS_ID_GET_CURRENT_UNIX_TIME               133
#define ADCS_ID_GET_PERSIST_CONFIG_DIAGNOSTIC       134
#define ADCS_ID_GET_COMMUNICATION_STATUS            135
#define ADCS_ID_GET_CONTROL_ESTIMATION_MODE         150
#define	ADCS_ID_GET_REFERENCE_IRC_VECTOR			156
#define ADCS_ID_GET_REFERENCE_LLH_TARGET            157
#define ADCS_ID_GET_ORBIT_MODE                      162
#define	ADCS_ID_GET_HEALTH_TLM_MMT					167
#define ADCS_ID_GET_RAW_CUBESENSE_SUN               170
#define	ADCS_ID_GET_REFERENCE_RPY_VALUES			181
#define	ADCS_ID_GET_OPENLOOPCMD_MTQ					182
#define ADCS_ID_GET_POWERSTATE                      183
#define ADCS_ID_GET_RUN_MODE                        184
#define ADCS_ID_GET_CONTROL_MODE                    185
#define	ADCS_ID_GET_WHL_CONFIG						186
#define ADCS_ID_GET_SATELLITE_CONFIG                189
#define ADCS_ID_GET_CONTROLLER_CONFIG               190
#define	ADCS_ID_GET_MAG0_MMT_CALIB_CONFIG			191
#define ADCS_ID_GET_DEFAULT_MODE_CONFIG             192
#define ADCS_ID_GET_MOUNTING_CONFIG                 193
#define	ADCS_ID_GET_MAG1_MMT_CALIB_CONFIG			194
#define	ADCS_ID_GET_ESTIMATOR_CONFIG				195
#define ADCS_ID_GET_SAT_ORBIT_PARAM_CONFIG          196
#define	ADCS_ID_GET_NODE_SELECTION_CONFIG			197
#define	ADCS_ID_GET_MTQ_CONFIG						198
#define	ADCS_ID_GET_ESTIMATION_MODE					199
#define ADCS_ID_GET_OPERATIONAL_STATE               200
#define ADCS_ID_GET_RAW_CSS_SENSOR                  203
#define ADCS_ID_GET_RAW_GYR_SENSOR                  204
#define ADCS_ID_GET_CALIBRATED_GYR_SENSOR           207
#define	ADCS_ID_GET_MAG_SENSING_ELM_CONFIG			221
#define	ADCS_ID_GET_UNSOLICIT_TLM_MSG_SETUP			228
#define ADCS_ID_GET_UNSOLICIT_EVENT_MSG_SETUP       233
#define ADCS_ID_GET_EVENT_LOG_STATUS_RESPONSE       235
#define ADCS_ID_GET_PORTMAP                         239
/* End of EndPoint ID */

#endif
