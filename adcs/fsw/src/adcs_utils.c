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
 *   This file contains the source code for the Adcs App utility functions
 */

/*
** Include Files:
*/
#include "adcs_app.h"
#include "adcs_eventids.h"
#include "adcs_tbl.h"
#include "adcs_utils.h"
#include "csp/csp_types.h"

#include "adcs_cube_error_typedefs.h"
#include "adcs_cube_typedefs.h"
#include "adcs_msg.h"

#include <csp/csp.h>

static Handle handle[TYPEDEF__COMMS_ENDPOINT_MAX];
// static uint8 cspDataBuffer[COMMS_BUFFER_SIZE];
static TypeDef_TctlmEndpoint endpoint;

void CUBE_EndpointInit(void)
{
	endpoint.nodeType = TYPES_COMMON_FRAMEWORK_TYPES_1__NODE_TYPE_CUBE_COMPUTER;
	endpoint.type = TYPEDEF__COMMS_ENDPOINT_CAN;
	endpoint.proto = TYPEDEF__COMMS_PROTOCOL_CSP;
	endpoint.addr = CAN_ADDR_CC;
	endpoint.timeout = 500u;
	endpoint.passthrough = false;

	/** 
	 * Maybe this member is useless, if `csp_transaction_w_opt()` is used.
	 * Because this function **doesn't care** the sport (source port)
	 */
	endpoint.cspSrcPort = CSP_SRC_PORT;

	/* No need */
	handle->bufferSize = COMMS_BUFFER_SIZE;
}


static uint8_t *cubeObc_connect_buffer(const TctlmCommsMasterSvc_Endpoint *masterEndpoint)
{
	return handle[masterEndpoint->endpoint.type].buffer;
}

static ErrorCode nack2ErrorCode(uint8 byte)
{
	ErrorCode result;

	switch (byte)
	{
		case (uint8)TCTLM__ERROR_OK:
		{
			result = CUBEOBC_ERROR_OK;
		}
		break;

		case (uint8)TCTLM__ERROR_INVALID_ID:
		{
			result = CUBEOBC_ERROR_TCTLM_INVALID_ID;
		}
		break;

		case (uint8)TCTLM__ERROR_INVALID_LENGTH:
		{
			result = CUBEOBC_ERROR_TCTLM_INVALID_LENGTH;
		}
		break;

		case (uint8)TCTLM__ERROR_INVALID_PARAM:
		{
			result = CUBEOBC_ERROR_TCTLM_INVALID_PARAM;
		}
		break;

		case (uint8)TCTLM__ERROR_CRC:
		{
			result = CUBEOBC_ERROR_TCTLM_CRC;
		}
		break;

		case (uint8)TCTLM__ERROR_NOT_IMPLEMENTED:
		{
			result = CUBEOBC_ERROR_TCTLM_NOT_IMPLEMENTED;
		}
		break;

		case (uint8)TCTLM__ERROR_BUSY:
		{
			result = CUBEOBC_ERROR_TCTLM_BUSY;
		}
		break;

		case (uint8)TCTLM__ERROR_SEQUENCE:
		{
			result = CUBEOBC_ERROR_TCTLM_SEQUENCE;
		}
		break;

		case (uint8)TCTLM__ERROR_INTERNAL:
		{
			result = CUBEOBC_ERROR_TCTLM_INTERNAL;
		}
		break;

		case (uint8)TCTLM__ERROR_PASS_TIMEOUT:
		{
			result = CUBEOBC_ERROR_TCTLM_PASS_TOUT;
		}
		break;

		case (uint8)TCTLM__ERROR_PASS_TARGET:
		{
			result = CUBEOBC_ERROR_TCTLM_PASS_TARGET;
		}
		break;

		default:
		{
			result = CUBEOBC_ERROR_UKN_NACK;
		}
		break;
	}

	return result;
}


/**
 * @brief SendReceive Function by CSP
 * @param masterEndpoint TctlmCommsMasterSvc_Endpoint Structure pointer
 * @param datalen Data length
 * @return CUBEOBC_ERROR_OK on success, otherwise ErrorCode
 */
static ErrorCode cubeObc_sendReceive(TctlmCommsMasterSvc_Endpoint *masterEndpoint, uint16 datalen) {
	ErrorCode result = CUBEOBC_ERROR_OK;
	TypeDef_TctlmEndpoint *endpoint = &masterEndpoint->endpoint; // Get the generic endpoint

	uint8 msgType;
	uint8 dstPort;

	if (masterEndpoint->id < V1_TLM_ID_START)
	{
		// Telecommand
		msgType = V1_TCTLM_CAN_TRANSPORT__TYPE_TC;
	}
	else
	{
		// Telemetry
		msgType = V1_TCTLM_CAN_TRANSPORT__TYPE_TLM;
	}

	if (endpoint->passthrough == false)
	{
		dstPort = CSP_PORT_TCTLM;
	}
	else
	{
		dstPort = CSP_PORT_PASSTHROUGH;
	}

	uint8_t txCspDataBuffer[COMMS_BUFFER_SIZE];
	uint8_t rxCspDataBuffer[COMMS_BUFFER_SIZE];

	// Add header data to CSP packet
	txCspDataBuffer[CSP_MSG_TYPE_IDX] = msgType;
	txCspDataBuffer[CSP_TCTLM_ID_IDX] = masterEndpoint->id;

	/**
	 * Copy the data buffer
	 */
	memcpy(txCspDataBuffer + CSP_HEADER_SIZE, handle[endpoint->type].buffer, datalen);
	// OS_printf("TxData: ");
	// for (uint8_t i=0; i< datalen + CSP_HEADER_SIZE; i++) {
	// 	OS_printf("0x%02X\t", txCspDataBuffer[i]);
	// }
	// OS_printf("\n");

	int32 res;
	if(msgType == V1_TCTLM_CAN_TRANSPORT__TYPE_TC) {
		/**
		 * If the TC is Reset, there is not ack (i.e. No reply)
		 */
		if(masterEndpoint->id == ADCS_ID_SET_RESET) {
			res = CFE_SRL_ApiTransactionCSP(endpoint->addr, dstPort,
										txCspDataBuffer, datalen + CSP_HEADER_SIZE,
										NULL, 0);
			if (res == 1) result = CUBEOBC_ERROR_OK;
			goto cleanup;
		}
		else {
		/**
		 * In this case, return length shoud be `3`(ACK) or `4`(NACK)
		 */
		res = CFE_SRL_ApiTransactionCSP(endpoint->addr, dstPort,
									txCspDataBuffer, datalen + CSP_HEADER_SIZE,
									rxCspDataBuffer, CSP_UNKNOWN_LEN);
		}
		
	}
	else { // msgType == V1_TCTLM_CAN_TRANSPORT__TYPE_TLM
		res = CFE_SRL_ApiTransactionCSP(endpoint->addr, dstPort,
									txCspDataBuffer, CSP_HEADER_SIZE,
									rxCspDataBuffer, datalen + CSP_HEADER_SIZE);
	}
									
	if (res) {
		result = CUBEOBC_ERROR_OK;
	}
	else
		return result;
	// Check that the response is for the expected TCTLM ID
	if (result == CUBEOBC_ERROR_OK)
	{
		uint8 tctlmId = rxCspDataBuffer[CSP_TCTLM_ID_IDX];

		if (tctlmId != masterEndpoint->id)
		{
			result = CUBEOBC_ERROR_TCTLM_ID;
		}
	}
	// Check the response type
	if (result == CUBEOBC_ERROR_OK)
	{
		V1TctlmCanTransport_Type rxMsgType;

		rxMsgType = (V1TctlmCanTransport_Type)rxCspDataBuffer[CSP_MSG_TYPE_IDX];

		/* Check for Nack */
		if ((rxMsgType == V1_TCTLM_CAN_TRANSPORT__TYPE_TC_NACK) ||
			(rxMsgType == V1_TCTLM_CAN_TRANSPORT__TYPE_TLM_NACK))
		{
			result = nack2ErrorCode(rxCspDataBuffer[CSP_HEADER_SIZE]);
		}
	}

	// Extract the data
	if (result == CUBEOBC_ERROR_OK)
	{
		// Extract TCTLM data from CSP response data
		memcpy(handle[endpoint->type].buffer, &rxCspDataBuffer[CSP_DATA_IDX], datalen);
	}
cleanup:
	return result;
}



/********************************************************
 * 
 * COSMIC Actual Set Command Function
 * 
 ********************************************************/
/* TC Functions, ID: 0 ~ 127 */
int32 ADCS_Reset(void)
{
	// ID 1
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed = 1;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_RESET;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	uint8 resetType = 66;		/**< Performs reset immediately - hard reset*/
	memcpy(tx_buffer, &resetType, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetCurrentUnixTime(const ADCS_CurrentUnixTimeCmd_Payload_t *setVal)
{	// ID 2
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_CURRENT_UNIX_TIME;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	//setVal->unixTimeSeconds = 	1731686180; // 예시 : 24-11-15-15-56-38 // CNDH에서 주는 값 그대로 input으로 받아야 함
	//memcpy(&rx_buffer[0], &setVal->unixTimeSeconds, sizeof(uint32_t)); 

	bufferSizeUsed = sizeof(ADCS_CurrentUnixTimeCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetErrorLogSetting(const ADCS_ErrorLogSettingCmd_Payload_t *setVal)
{	// ID 6
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_ERROR_LOG_SETTING;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_ErrorLogSettingCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetPersistConfig(void)
{	// ID 7
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_PERSIST_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	bufferSizeUsed = 0;

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetControlEstimationMode(const ADCS_ControlEstimationModeCmd_Payload_t *setVal)
{	// ID 42
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_CONTROL_ESTIMATION_MODE;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_ControlEstimationModeCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetDisableMagRwlMntMng(const ADCS_DisableMagRwlMntMngCmd_Payload_t *setVal)
{	// ID 43
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_DISABLE_MAG_RWL_MNT_MNG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_DisableMagRwlMntMngCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetReferenceIRCVector(const ADCS_ReferenceIRCVectorCmd_Payload_t *setVal)
{	// ID 47
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_REFERENCE_IRC_VECTOR;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_ReferenceIRCVectorCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetReferenceLLHTarget(const ADCS_ReferenceLLHTargetCmd_Payload_t *setVal)
{	// ID 48
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_REFERENCE_LLH_TARGET;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_ReferenceLLHTargetCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetOrbitMode(const ADCS_OrbitModeCmd_Payload_t *setVal)
{	// ID 51
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_ORBIT_MODE;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_OrbitModeCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetMagDeploy(const ADCS_MagDeployCmd_Payload_t *setVal)
{	// ID 52
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_MAG_DEPLOY_CMD;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_MagDeployCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetReferenceRPYValues(const ADCS_ReferenceRPYvaluesCmd_Payload_t *setVal)
{	// ID 54
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_REFERENCE_RPY_VALUES;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_ReferenceRPYvaluesCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetOpenLoopCmdMTQ(const ADCS_OpenLoopCmdMTQCmd_Payload_t *setVal)
{	// ID 55
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_OPENLOOPCMD_MTQ;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_OpenLoopCmdMTQCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetPowerState(const ADCS_PowerStateCmd_Payload_t *setVal)
{	// ID 56
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_POWER_STATE;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_PowerStateCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetRunMode(const ADCS_RunModeCmd_Payload_t *setVal)
{	// ID 57
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_RUN_MODE;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_RunModeCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetControlMode(const ADCS_ControlModeCmd_Payload_t *setVal)
{	// ID 58
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_CONTROL_MODE;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_ControlModeCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetWhlConfig(const ADCS_WhlConfigCmd_Payload_t *setVal)
{	// ID 59
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_WHL_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_WhlConfigCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetSatelliteConfig(const ADCS_SatConfigCmd_Payload_t *setVal)
{	// ID 61
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_SATELLITE_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_SatConfigCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetControllerConfig(const ADCS_ControllerConfig_Payload_t *setVal)
{	// ID 62
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_CONTROLLER_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_ControllerConfig_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetMag0MMTCalibConfig(const ADCS_Mag0MMTCalibConfigCmd_Payload_t *setVal)
{	// ID 63
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_MAG0_MMT_CALIB_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_Mag0MMTCalibConfigCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetDefaultModeConfig(const ADCS_DefaultModeConfigCmd_Payload_t *setVal)
{	// ID 64
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_DEFAULT_MODE_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_DefaultModeConfigCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetMountingConfig(const ADCS_MountingConfigCmd_Payload_t *setVal)
{	// ID 65
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_MOUNTING_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_MountingConfigCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetMag1MMTCalibConfig(const ADCS_Mag1MMTCalibConfigCmd_Payload_t *setVal)
{	// ID 66
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_MAG1_MMT_CALIB_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_Mag1MMTCalibConfigCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetEstimatorConfig(const ADCS_EstimatorConfigCmd_Payload_t *setVal)
{	// ID 67
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_ESTIMATOR_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_EstimatorConfigCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetSatOrbitParamConfig(const ADCS_SatOrbitParamConfigCmd_Payload_t *setVal)
{	// ID 68
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_SAT_ORBIT_PARAM_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_SatOrbitParamConfigCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetNodeSelectionConfig(const ADCS_NodeSelectionConfigCmd_Payload_t *setVal)
{	// ID 69
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_NODE_SELECTION_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_NodeSelectionConfigCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetMTQConfig(const ADCS_MTQConfigCmd_Payload_t *setVal)
{	// ID 70
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_MTQ_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_MTQConfigCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetEstimationMode(const ADCS_EstimationModeCmd_Payload_t *setVal)
{	// ID 71
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_ESTIMATION_MODE;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_EstimationModeCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetOperationalState(const ADCS_OperationalStateCmd_Payload_t *setVal)
{	// ID 72
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_OPERATIONAL_STATE;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_OperationalStateCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetMagSensingElmConfig(const ADCS_MagSensingElmConfigCmd_Payload_t *setVal)
{	// ID 77
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_MAG_SENSING_ELM_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_MagSensingElmConfigCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetTransferFrame(const ADCS_TransferFrameCmd_Payload_t *setVal)
{	// ID 79
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_TRANSFER_FRAME;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_TransferFrameCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetUnsolicitTlmMsgSetup(const ADCS_UnsolicitTlmMsgSetupCmd_Payload_t *setVal)
{	// ID 112
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_UNSOLICIT_TLM_MSG_SETUP;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_UnsolicitTlmMsgSetupCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetUnsolicitEventMsgSetup(const ADCS_UnsolicitEventMsgSetupCmd_InternalPayload_t *setVal)
{	// ID 116
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_UNSOLICIT_EVENT_MSG_SETUP;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_UnsolicitEventMsgSetupCmd_InternalPayload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetReqTlmLogTransferSetup(const ADCS_RequestTlmLogTransferSetupCmd_Payload_t *setVal)
{	// ID 117
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_REQ_TLM_LOG_TRANSFER_SETUP;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_RequestTlmLogTransferSetupCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}

int32 ADCS_SetInitiateEventLogTransfer(const ADCS_InitiateEventLogTransferCmd_Payload_t *setVal)
{	// ID 120

	int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8_t *tx_buffer;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_INITIATE_EVENT_LOG_TRANSFER;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	tx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_InitiateEventLogTransferCmd_Payload_t);
	memcpy(tx_buffer, setVal, bufferSizeUsed);

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}


/********************************************************
 * 
 * COSMIC Actual Get Command Function (Get tlm)
 * 
 ********************************************************/
int32 ADCS_GetErrorLogSetting(ADCS_ErrorLogSettingTlm_Payload_t *returnVal)
{	// ID 132

    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_ERROR_LOG_SETTING;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_ErrorLogSettingTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

 int32 ADCS_GetCurrentUnixTime(ADCS_CurrentUnixTimeTlm_Payload_t *returnVal)
{	// ID 133

    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_CURRENT_UNIX_TIME;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_CurrentUnixTimeTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetPersistConfigDiagnostic(ADCS_PersistConfigDiagnosticTlm_Payload_t *returnVal)
{	// ID 134
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_PERSIST_CONFIG_DIAGNOSTIC;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_PersistConfigDiagnosticTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);
	
	return CFE_SUCCESS;
}

int32 ADCS_GetCommunicationStatus(ADCS_CommunicationStatusTlm_Payload_t *returnVal)
{	// ID 135
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_COMMUNICATION_STATUS;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_CommunicationStatusTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetControlEstimationMode(ADCS_ControlEstimationModeTlm_Payload_t *returnVal)
{	// ID 150
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_CONTROL_ESTIMATION_MODE;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_ControlEstimationModeTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetReferenceIRCVector(ADCS_ReferenceIRCVectorTlm_Payload_t *returnVal)
{	// ID 156
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_REFERENCE_IRC_VECTOR;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_ReferenceIRCVectorTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetReferenceLLHTarget(ADCS_ReferenceLLHTargetTlm_Payload_t *returnVal)
{	// ID 157
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_REFERENCE_LLH_TARGET;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_ReferenceLLHTargetTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetOrbitMode(ADCS_OrbitModeTlm_Payload_t *returnVal)
{	// ID 162
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_ORBIT_MODE;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_OrbitModeTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetHealthTlmMMT(ADCS_HealthTlmMMTTlm_Payload_t *returnVal)
{	// ID 167
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_HEALTH_TLM_MMT;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_HealthTlmMMTTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetRawCubeSenseSun(ADCS_RawCubeSenseSunTlm_Payload_t *returnVal)
{	// ID 170
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_RAW_CUBESENSE_SUN;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_RawCubeSenseSunTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetReferenceRPYvalues(ADCS_ReferenceRPYvaluesTlm_Payload_t *returnVal)
{	// ID 181
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_REFERENCE_RPY_VALUES;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_ReferenceRPYvaluesTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetOpenLoopCmdMTQ(ADCS_OpenLoopCmdMTQTlm_Payload_t *returnVal)
{	// ID 182
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_OPENLOOPCMD_MTQ;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_OpenLoopCmdMTQTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetPowerState(ADCS_PowerStateTlm_Payload_t *returnVal)
{	// ID 183
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_POWERSTATE;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_PowerStateTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetRunMode(ADCS_RunModeTlm_Payload_t *returnVal)
{	// ID 184
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_RUN_MODE;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_RunModeTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetControlMode(ADCS_ControlModeTlm_Payload_t *returnVal)
{	// ID 185
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_CONTROL_MODE;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_ControlModeTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetWhlConfig(ADCS_WhlConfigTlm_Payload_t *returnVal)
{	// ID 186
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_WHL_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_WhlConfigTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetSatelliteConfig(ADCS_SatelliteConfigTlm_Payload_t *returnVal)
{	// ID 189
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_SATELLITE_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_SatelliteConfigTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetControllerConfig(ADCS_ControllerConfigTlm_Payload_t *returnVal)
{	// ID 190
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_CONTROLLER_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_ControllerConfigTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetMag0MMTCalibConfig(ADCS_Mag0MMTCalibConfigTlm_Payload_t *returnVal)
{	// ID 191
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_MAG0_MMT_CALIB_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_Mag0MMTCalibConfigTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetDefaultModeConfig(ADCS_DefaultModeConfigTlm_Payload_t *returnVal)
{	// ID 192
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_DEFAULT_MODE_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_DefaultModeConfigTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetMountingConfig(ADCS_MountingConfigTlm_Payload_t *returnVal)
{	// ID 193
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_MOUNTING_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_MountingConfigTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetMag1MMTCalibConfig(ADCS_Mag1MMTCalibConfigTlm_Payload_t *returnVal)
{	// ID 194
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_MAG1_MMT_CALIB_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_Mag1MMTCalibConfigTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetEstimatorConfig(ADCS_EstimatorConfigTlm_Payload_t *returnVal)
{	// ID 195
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_ESTIMATOR_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_EstimatorConfigTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetSatOrbitParamConfig(ADCS_SatOrbitParamConfigTlm_Payload_t *returnVal)
{	// ID 196
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_SAT_ORBIT_PARAM_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_SatOrbitParamConfigTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetNodeSelectionConfig(ADCS_NodeSelectionConfigTlm_Payload_t *returnVal)
{	// ID 197
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_NODE_SELECTION_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_NodeSelectionConfigTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetMTQConfig(ADCS_MTQConfigTlm_Payload_t *returnVal)
{	// ID 198
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_MTQ_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_MTQConfigTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetEstimationMode(ADCS_EstimationModeTlm_Payload_t *returnVal)
{	// ID 199
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_ESTIMATION_MODE;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_EstimationModeTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetOperationalState(ADCS_OperationalStateTlm_Payload_t *returnVal)
{	// ID 200
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_OPERATIONAL_STATE;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_OperationalStateTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetRawCSSSensor(ADCS_RawCSSSensorTlm_Payload_t *returnVal)
{	// ID 203
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_RAW_CSS_SENSOR;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_RawCSSSensorTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}


int32 ADCS_GetRawGYRSensor(ADCS_RawGYRSensorTlm_Paylaod_t *returnVal)
{	// ID 204
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_RAW_GYR_SENSOR;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_RawGYRSensorTlm_Paylaod_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetCalibratedGYRSensor(ADCS_CalibratedGYRSensorTlm_Payload_t *returnVal)
{	// ID 207
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_CALIBRATED_GYR_SENSOR;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_CalibratedGYRSensorTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetDataFrame(ADCS_DataFrameTlm_Payload_t *returnVal)
{	// ID 219
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_DATA_FRAME;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_DataFrameTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetInfoFramInMemory(ADCS_InfoFrameInMemoryTlm_Payload_t *returnVal)
{	// ID 220
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_INFO_FRAME_IN_MOMERY;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_InfoFrameInMemoryTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetMagSensingElmConfig(ADCS_MagSensingElmConfigTlm_Payload_t *returnVal)
{	// ID 221
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_MAG_SENSING_ELM_CONFIG;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_MagSensingElmConfigTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetTlmLogInclMask(ADCS_TlmLogInclMaskTlm_Payload_t *returnVal)
{	// ID 227
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_TLM_LOG_INCLMASK;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_TlmLogInclMaskTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetUnsolicitTlmMsgSetup(ADCS_UnsolicitTlmMsgSetupTlm_Payload_t *returnVal)
{	// ID 228
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_UNSOLICIT_TLM_MSG_SETUP;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_UnsolicitTlmMsgSetupTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetUnsolicitEventMsgSetup(ADCS_UnsolicitEventMsgSetupTlm_Payload_t *returnVal)
{	// ID 233
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_UNSOLICIT_EVENT_MSG_SETUP;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_UnsolicitEventMsgSetupTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetTlmLogStatusResponse(ADCS_TlmLogStatusResponseTlm_Payload_t *returnVal)
{	// ID 234
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_TLM_LOG_STATUS_RESPONSE;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_TlmLogStatusResponseTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetEventLogStatusResponse(ADCS_EventLogStatusResponseTlm_Payload_t *returnVal)
{	// ID 235
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_EVENT_LOG_STATUS_RESPONSE;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_EventLogStatusResponseTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_GetPortMap(ADCS_PortMapTlm_Payload_t *returnVal)
{	// ID 239
	
    int32 status;
	TctlmCommsMasterSvc_Endpoint target;
	uint8 *rx_buffer;
	uint16 bufferSizeUsed;

	ZERO_VAR(target);
	
	target.id = ADCS_ID_GET_PORTMAP;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	rx_buffer = cubeObc_connect_buffer(&target);

	bufferSizeUsed = sizeof(ADCS_PortMapTlm_Payload_t);
	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS CAN Read Error (Error code : %d, ID: %d)\n", status, target.id);
		return status;
	}
	memcpy(returnVal, rx_buffer, bufferSizeUsed);

	return CFE_SUCCESS;
}

int32 ADCS_SetErrorLogClear(void)
{	// ID 5
	
    int32_t status;
	TctlmCommsMasterSvc_Endpoint target;
	uint16_t bufferSizeUsed;
	
	ZERO_VAR(target);
	
	target.id = ADCS_ID_SET_ERROR_LOG_CLEAR;
	memcpy((uint8_t *) &target.endpoint, (uint8_t *) &endpoint, sizeof(TypeDef_TctlmEndpoint));

	bufferSizeUsed = 0;

	if((status = cubeObc_sendReceive(&target, bufferSizeUsed)) != CUBEOBC_ERROR_OK)
	{
		OS_printf("ADCS CAN Write Error (Error code : %d, ID: %d)\n",status, target.id);
		return status;
	}

	return CFE_SUCCESS;
}


/***********************************************
 * 
 * Report util function
 * 
 **********************************************/
void ADCS_HandleReport(int32 Status, uint8_t CC, void *ReadData, uint16_t ReadSize) {
	CFE_SB_Buffer_t *BufPtr = CFE_SB_AllocateMessageBuffer(sizeof(ADCS_ReportTlm_t));
	if (BufPtr == NULL) return;

	ADCS_ReportTlm_t *Report = (ADCS_ReportTlm_t *)BufPtr;
	if (CFE_MSG_Init(CFE_MSG_PTR(Report->TelemetryHeader), CFE_SB_ValueToMsgId(ADCS_REPORT_TLM_MID), sizeof(ADCS_ReportTlm_t)) != CFE_SUCCESS) {
		CFE_SB_ReleaseMessageBuffer(BufPtr);
		return;
	}

	Report->Report.MsgID = ADCS_CMD_MID;
	Report->Report.CommandCode = CC;
	Report->Report.ReturnType = (Status == CFE_SUCCESS) ? RPT_RETTYPE_SUCCESS : RPT_RETTYPE_HW;
	Report->Report.ReturnCode = Status; // `adcs_cube_error_typedefs.h`
	Report->Report.ReturnDataSize = ReadSize;
	if (ReadSize && ReadData) {
		memcpy(Report->Report.ReturnValue, ReadData,
				(ReadSize > RPT_RET_VALUE_BUF_SIZE) ? RPT_RET_VALUE_BUF_SIZE : ReadSize);
	}

	CFE_SB_TimeStampMsg((CFE_MSG_PTR(Report->TelemetryHeader)));
	if (CFE_SB_TransmitBuffer(BufPtr, true) != CFE_SUCCESS) {
		CFE_SB_ReleaseMessageBuffer(BufPtr);
		return;
	}
	return;
}


/**************************************
 * CubeADCS EVS listen Task function
 **************************************/
void ADCS_HandleEvent(const ADCS_EventEntry_t *Event) {
	switch (Event->Identifier.EventClass)
	{
	case CLASS_CRITICAL:
		OS_printf("CRRITICAL EventType : %u || EventSource : %u || EventClass : %u\n",
				Event->Identifier.EventType, Event->Identifier.EventSource,
				Event->Identifier.EventClass);
		break;
	case CLASS_MAJOR_WARNING:
		OS_printf("MAJOR EventType : %u || EventSource : %u || EventClass : %u\n",
				Event->Identifier.EventType, Event->Identifier.EventSource,
				Event->Identifier.EventClass);
		break;
	case CLASS_MINOR_WARNING:
		OS_printf("MINOR EventType : %u || EventSource : %u || EventClass : %u\n",
				Event->Identifier.EventType, Event->Identifier.EventSource,
				Event->Identifier.EventClass);
		break;
	case CLASS_INFORMATION:
		OS_printf("INFO EventType : %u || EventSource : %u || EventClass : %u\n",
				Event->Identifier.EventType, Event->Identifier.EventSource,
				Event->Identifier.EventClass);
		
		switch (Event->Identifier.EventType)
		{
		case 142: // Eclipse/sunlight transition occurred
			OS_printf("Event Data : ");
			for (uint8_t i=0; i < 8; i++) {
				OS_printf("0x%02X\t", Event->EventData[i]);
			}
			OS_printf("\n");
			ADCS_AppData.BcnTlm.IsSunlight = Event->EventData[0] ? true : false;
			break;
		case 139:
			
		default:
			break;
		}

		break;
	/* End of CLASS_INFORMATION */

	default:
		break;
	}
}

void ADCS_ListenEventTask(void) {
	int Status;
	csp_socket_t *Sock;
	csp_conn_t *Conn = NULL;
	csp_packet_t *Packet = NULL;

	Sock = csp_socket(CSP_O_NONE);
	if (Sock == NULL) {
        CFE_ES_WriteToSysLog("%s: csp_socket failed! NO RC\n", __func__);
        return;  // Revise to `csp_socket failed`
    }

	Status = csp_bind(Sock, CSP_PORT_EVENT);
	if (Status != CSP_ERR_NONE) {
        CFE_ES_WriteToSysLog("%s: csp_bind failed at Port: %d RC=%d\n", __func__, 58, Status);
        return;
    }

	Status = csp_listen(Sock, 5);
	if (Status != CSP_ERR_NONE) {
        CFE_ES_WriteToSysLog("%s: csp_listen failed! RC=%d\n", __func__, Status);
        return;
    }

	for (;;) {
		Conn = csp_accept(Sock, 10000);
		if (Conn == NULL) {
			continue;
		}
		while ((Packet = csp_read(Conn, 1000)) != NULL) {
			int Port = csp_conn_dport(Conn);
			switch (Port)
			{
			case CSP_PORT_EVENT:
				OS_printf("ADCS Event Comming.\n");
				ADCS_HandleEvent((const ADCS_EventEntry_t *)Packet);

				/* Free buffer & Remove dangled pointer */
				csp_buffer_free(Packet);
				Packet = NULL;
				break;
			
			default:
				break;
			}
		}
		csp_close(Conn);
	}
	
}


/**************************************
 * CubeADCS Additional Functions
 **************************************/
// Commissioning Functions
 int32 ADCS_COMM_InitAngRateEst(void) {
	CFE_Status_t			status;
	CFE_TIME_SysTime_t		tnow, tsend;

    // Struct for Setting Values
	ADCS_PowerStateCmd_Payload_t					SetVal_56 = {0,};
	// ADCS_UnsolicitTlmMsgSetupCmd_Payload_t 			SetVal_112 = {0,};
    ADCS_RequestTlmLogTransferSetupCmd_Payload_t 	SetVal_117 = {0,};
    ADCS_TransferFrameCmd_Payload_t 				SetVal_79 = {0,};

    // Struct for Return Values
    ADCS_CurrentUnixTimeTlm_Payload_t 				RetVal_133 = {0,};    
    ADCS_TlmLogStatusResponseTlm_Payload_t 			RetVal_234 = {0,};
    ADCS_InfoFrameInMemoryTlm_Payload_t 			RetVal_220 = {0,};
	ADCS_DataFrameTlm_Payload_t						RetVal_219 = {0,};


    // uint32 UnixSeconds_start = 0;
	uint8 TotalNumberOfEntries = 20;
	uint8 SumNumberOfEntries = 0;

	SetVal_56.GYR0 = 1;
	SetVal_56.GYR1 = 1;
	status = ADCS_SetPowerState(&SetVal_56);
	OS_TaskDelay(10);

	// SetVal_112.CANTlmRetrunInterval = 6;				// Return interval 1s
    // SetVal_112.CANTlmEDInclusionBitmask[2] = 1;			// 0b0000 0001
    // status = ADCS_SetUnsolicitTlmMsgSetup(&SetVal_112);
    // SetVal_112.CANTlmEDInclusionBitmask[2] = 0;

	// status = ADCS_SetUnsolicitTlmMsgSetup(&SetVal_112);

	// if (status != CUBEOBC_ERROR_OK) {
	// 	OS_printf("ADCS Error on ADCS_SubSeqTlmSet_InitAngRateEst()\n");
	// 	return status;
	// }
	
    status = ADCS_GetCurrentUnixTime(&RetVal_133);

    SetVal_117.FilterType = 4;
	SetVal_117.NumberOfEntries = TotalNumberOfEntries;
	SetVal_117.UnixStartTime = RetVal_133.CurrentUnixseconds;
	SetVal_117.UnixEndTime = RetVal_133.CurrentUnixseconds;
	SetVal_117.TlmLogReturnInterval = 0;	// 1 = 1 s, ... 5 = 5 s, 6 = 10 s, ...
    SetVal_117.LogIDbitmask[2] = 1;
	
	// SET TLM LOG Transfer Setup
    status = ADCS_SetReqTlmLogTransferSetup(&SetVal_117);
	
	OS_TaskDelay(10);
	// OS_TaskDelay(22000);

	uint16 timeout_234 = 10000;
	uint16 backoff = 100;
	uint16 backoffTotal = 0;

	// TLM Poll
    status = ADCS_GetTlmLogStatusResponse(&RetVal_234);
	uint8 flagReadQStatus = (RetVal_234.ReadQState == 1);
	while ((flagReadQStatus == false) && (status == CUBEOBC_ERROR_OK)) {
		if (flagReadQStatus == false) {
			if (backoffTotal >= timeout_234) {
				return CUBEOBC_ERROR_TOUT;
			}
			else {
				OS_TaskDelay(backoff);
				backoffTotal += backoff;
			}

		}
		status = ADCS_GetTlmLogStatusResponse(&RetVal_234);
		flagReadQStatus = (RetVal_234.ReadQState == 1);
	}
	OS_TaskDelay(10);


	// Data Transfer - Download
	ErrorCode result = CUBEOBC_ERROR_OK;
	ErrorCode lastResult = CUBEOBC_ERROR_OK;
	uint16 LocalFrameNumber = 0;
	uint8 flagFrameSet = false;
	uint8 flagFrameError = false;
	uint8 flagExit = false;
	uint8 flagFrameNumMatch = false;
	uint8 flagLastFrame = false;

	uint8 frameCrc = 0xFF;

	CubeADCS_TlmLogFrame_Test_t Frame_test = {0};

	tsend = CFE_TIME_GetTime();

	while ((flagLastFrame == false) && (result == CUBEOBC_ERROR_OK)) {
		flagFrameSet = false;
		flagExit = false;

		ZERO_VAR(Frame_test);

		// Set Frame Number
		while ((flagFrameSet == false) && (flagExit == false)) {
			SetVal_79.NextFrameNumber = LocalFrameNumber;
			OS_printf("START: ADCS_SetTransferFrame! - Required Frame #: %u\n",LocalFrameNumber);
			result = ADCS_SetTransferFrame(&SetVal_79);
			OS_printf("ADCS_SetTransferFrame Result: %d\n",result);
			if (result == CUBEOBC_ERROR_OK) {
				tsend = CFE_TIME_GetTime();
				flagFrameSet = true;
				
			}
			else {
				tnow = CFE_TIME_GetTime();
				if (result == CUBEOBC_ERROR_TOUT) {
					if ((tnow.Seconds - tsend.Seconds) > 5) {
						flagExit = true;
					}
				}
				else if (result == CUBEOBC_ERROR_TCTLM_BUSY) {
					if (lastResult == CUBEOBC_ERROR_TOUT) {
						flagFrameSet = true;
					}
					else if ((tnow.Seconds - tsend.Seconds) > 5) {
						flagExit = true;
					}
					else {
						OS_TaskDelay(5);
					}				
				}
				else if (result == CUBEOBC_ERROR_TCTLM_INVALID_PARAM) {
					if (lastResult == CUBEOBC_ERROR_TOUT) {
						flagFrameSet = true;
					}
					else if ((tnow.Seconds - tsend.Seconds) > 5) {
						flagExit = true;
					}
				}
				else {
					flagExit = true;
				}
			}
			lastResult = result;
			OS_printf("ERROR CODE: %d\n",result);
		}

		if (flagFrameSet == true) status = CUBEOBC_ERROR_OK;
		status = result;
		tsend = CFE_TIME_GetTime();

		if (status == CUBEOBC_ERROR_OK) {
			OS_printf("Set Frame Number is complete!!\n");
		}

		// Poll Frame Number
		if (status == CUBEOBC_ERROR_OK) {
			while ((flagFrameNumMatch == false) && (flagFrameError == false) && (flagExit == false)) {
				OS_TaskDelay(10);
				result = ADCS_GetInfoFramInMemory(&RetVal_220);
				if (result == CUBEOBC_ERROR_OK) {
					flagFrameNumMatch = (RetVal_220.FrameNumber == LocalFrameNumber);
					flagLastFrame = RetVal_220.LastFrame;
					flagFrameError = RetVal_220.FrameError;
				}
				if (flagFrameNumMatch == false) {
					tnow = CFE_TIME_GetTime();
					if ((tnow.Seconds - tsend.Seconds) > 5) {
						flagExit = true;
						if (result == CUBEOBC_ERROR_OK) result = CUBEOBC_ERROR_TOUT;
					}
				}

			}
		}

		status = result;
		if (flagLastFrame) {
			OS_printf("This is the Last Frame!: %u\n",LocalFrameNumber);
		}



		if ((status == CUBEOBC_ERROR_OK) && (flagFrameError == true)) status = CUBEOBC_ERROR_FRAME;
		
		// Get Frame
		frameCrc = 0xFF;
		if (status == CUBEOBC_ERROR_OK) {
			flagExit = false;
			
			while (flagExit == false) {	
				status = ADCS_GetDataFrame(&RetVal_219);
				OS_printf("Download Compelete! (FrameSize = %u)\n",RetVal_219.FrameSize);
				
				if (result == CUBEOBC_ERROR_OK) {
					flagExit =true;
				}
				else {
					tnow = CFE_TIME_GetTime();
					if ((tnow.Seconds - tsend.Seconds) > 5) {
						flagExit = true;
					}
				}
			}
			
			if (result == CUBEOBC_ERROR_OK) {
				if ((RetVal_219.FrameSize == 0) && (flagLastFrame == false)) result = CUBEOBC_ERROR_UNKNOWN;
			}
			
			if ((result == CUBEOBC_ERROR_OK) && (RetVal_219.FrameSize > 0)) {
				
				// Check CRC
				for (int i = 0; i < RetVal_219.FrameSize; i++) {
					frameCrc ^= RetVal_219.FrameByte[i];
				}

				OS_printf("Frame #%u CRC from FrameInfo: %u\n",LocalFrameNumber,RetVal_220.Checksum);
				OS_printf("Frame #%u CRC from Calculate: %u\n",LocalFrameNumber,frameCrc);

				if (RetVal_220.Checksum != frameCrc) {
					OS_printf("Frame #%u CRC is NOT MATCHED!\n",LocalFrameNumber);
					result = CUBEOBC_ERROR_CRC;
					flagExit = true;
				}
				else {
					OS_printf("Frame #%u CRC is MATCHED!\n",LocalFrameNumber);
				}

				// Handling Data in the current Frame
				memcpy(&Frame_test,&RetVal_219.FrameByte,RetVal_219.FrameSize);


				LocalFrameNumber++;

				status = result;
			}
		}

		uint16 NumberofEntry = 0;
		NumberofEntry = (RetVal_219.FrameSize - 5) / (14 + sizeof(ADCS_RawGYRSensorTlm_Paylaod_t));
		OS_printf("[Frame %u]\n",LocalFrameNumber-1);
		OS_printf("Size of Frame         : %u\n", RetVal_219.FrameSize);
		OS_printf("Size of Frame w/o mask: %u\n", RetVal_219.FrameSize-5);
		OS_printf("Size of TLM + metadata: %u\n", (uint32) (14 + sizeof(ADCS_RawGYRSensorTlm_Paylaod_t)));
		OS_printf("Size of TLM           : %u\n", (uint32) sizeof(ADCS_RawGYRSensorTlm_Paylaod_t));

		OS_printf("[TLM LOG Transfer Reseult]\n");
		OS_printf("Inclusion Mask: |%u|%u|%u|%u|%u|\n",Frame_test.inclusionMask[0],Frame_test.inclusionMask[1],Frame_test.inclusionMask[2],Frame_test.inclusionMask[3],Frame_test.inclusionMask[4]);
		OS_printf("Number of Entries: %u\n", NumberofEntry);
		OS_printf("------------------------------\n");
		for (int i = 0; i<NumberofEntry; i++) {
			OS_printf("<Entry %d>\n", i);
			OS_printf("Counter: %u | Uptime: %u\n", Frame_test.Entry[i].MetaData.counter, Frame_test.Entry[i].MetaData.uptime);
			OS_printf("Unix Time: %u [sec] + %u [ms]\n", Frame_test.Entry[i].MetaData.unixtime_sec, Frame_test.Entry[i].MetaData.unixtime_ms);
			OS_printf("[TLM ID 204]\n");
			OS_printf("Time : %u [sec] + %u [ns]\n", Frame_test.Entry[i].TLM0.TimeSeconds, Frame_test.Entry[i].TLM0.TimeNanoSeconds);
			OS_printf("GYR0 X|Y|Z: %f | %f | %f \n", Frame_test.Entry[i].TLM0.GYR0RawRateX, Frame_test.Entry[i].TLM0.GYR0RawRateY, Frame_test.Entry[i].TLM0.GYR0RawRateZ);
			OS_printf("GYR1 X|Y|Z: %f | %f | %f \n", Frame_test.Entry[i].TLM0.GYR1RawRateX, Frame_test.Entry[i].TLM0.GYR1RawRateY, Frame_test.Entry[i].TLM0.GYR1RawRateZ);
			OS_printf("Valid Flag GYR0 & 1: %u & %u\n", Frame_test.Entry[i].TLM0.GYR0ValidFlag, Frame_test.Entry[i].TLM0.GYR1ValidFlag);
			OS_printf("------------------------------\n");
		}

		SumNumberOfEntries += NumberofEntry;

		if (SumNumberOfEntries == TotalNumberOfEntries) flagLastFrame = true;

	}

	
	return status;
 }

/*
int32 ADCS_SubSeqTlmSet_InitAngRateEst() {
    CFE_Status_t               status;
	
	// Telemetry Logging
    ADCS_UnsolicitTlmMsgSetupCmd_Payload_t SetVal_112 = {0,};

    // // ID 211 Main estimator high-resolution telemetry
    // SetVal_112.CANTlmRetrunInterval = 6;				// Return interval 10s
    // SetVal_112.CANTlmEDInclusionBitmask[2] = 128;		// 0b1000 0000
    // status = ADCS_SetUnsolicitTlmMsgSetup(&SetVal_112);
    // SetVal_112.CANTlmEDInclusionBitmask[2] = 0;
    
    // // ID 173 Backup estimator telemetry
    // SetVal_112.CANTlmRetrunInterval = 6;				// Return interval 10s
    // SetVal_112.CANTlmEDInclusionBitmask[0] = 128;		// 0b1000 0000
    // status = ADCS_SetUnsolicitTlmMsgSetup(&SetVal_112);
    // SetVal_112.CANTlmEDInclusionBitmask[0] = 0;

    // ID 204 Raw GYR sensor telemetry
    // SetVal_112.CANTlmRetrunInterval = 6;				// Return interval 10s
	// SetVal_112.CANTlmRetrunInterval = 5;				// Return interval 5s
	SetVal_112.CANTlmRetrunInterval = 1;				// Return interval 1s
    SetVal_112.CANTlmEDInclusionBitmask[2] = 1;			// 0b0000 0001
    status = ADCS_SetUnsolicitTlmMsgSetup(&SetVal_112);
    SetVal_112.CANTlmEDInclusionBitmask[2] = 0;

	if (status != CUBEOBC_ERROR_OK) {
		OS_printf("ADCS Error on ADCS_SubSeqTlmSet_InitAngRateEst()\n");
		return status;
	}
    
    // // ID 180 Raw MAG sensor telemetry
    // SetVal_112.CANTlmRetrunInterval = 6;				// Return interval 10s
    // SetVal_112.CANTlmEDInclusionBitmask[1] = 64;		// 0b0100 0000
    // status = ADCS_SetUnsolicitTlmMsgSetup(&SetVal_112);
    // SetVal_112.CANTlmEDInclusionBitmask[1] = 0;
	
	return CFE_SUCCESS;
}
*/