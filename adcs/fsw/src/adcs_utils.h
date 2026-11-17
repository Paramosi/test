#ifndef ADCS_APP_UTILS_H
#define ADCS_APP_UTILS_H

#include "adcs_msg.h"
#include <math.h>
#include <string.h>

#include "common_types.h"
#include "adcs_msg.h"
#include "adcs_cube_error_typedefs.h"
#include "adcs_cube_typedefs.h"
/**
 * @brief memset variable to all zero's
 */
#define ZERO_VAR(var)							\
		do										\
		{										\
			(void)memset((void *)&(var), 0, sizeof(var));	\
		} while (false)



#define COMMS_BUFFER_SIZE	((uint32)256)

/**
 * @brief Telemetry Start ID
 */
#define V1_TLM_ID_START	((uint8)128u)		/**< Used for distinguishing the TC/TM */

/**
 * CubeADCS CSP configuration
 */
#define CAN_ADDR_CC				((uint8)4u)		/**< CSP Node address of CubeADCS */
#define CSP_SRC_PORT			((uint8)11u)	/**< Source port used for CSP comms with CubeProduct */

#define CSP_HEADER_SIZE			((uint32)2u)	/**< Size of CubeSpace header within CSP packet */
#define CSP_MSG_TYPE_IDX		((uint32)0u)	/**< Index of message type within CSP packet */
#define CSP_TCTLM_ID_IDX		((uint32)1u)	/**< Index of TCTLM ID within CSP packet */
#define CSP_DATA_IDX			((uint32)2u)	/**< Index of TCTLM data within CSP packet */

#define CSP_PORT_TCTLM			((uint8)8u)		/**< CSP port used for TCTLM */
#define CSP_PORT_PASSTHROUGH	((uint8)48u)	/**< CSP port used for passthrough TCTLM */
#define CSP_PORT_EVENT			((uint8)58)		/**< CSP port used for Events ingestion */
#define CSP_UNKNOWN_LEN			((int32)-1)		/**< CSP parameter value which is used in `csp_transaction_w_opt` */


/*
// CubeADCS Log Frame Max Entry Number
*/
#define CUBESPACE_MAX_ENTRY_NUM		11

typedef enum CubeADCS_EventClass {
	CLASS_INFORMATION,
	CLASS_MINOR_WARNING,
	CLASS_MAJOR_WARNING,
	CLASS_CRITICAL
} CubeADCS_EventClass_t;

typedef struct HandleStruct {
	uint8 buffer[COMMS_BUFFER_SIZE];/**< Buffer for packing and unpacking */
	uint32 bufferSize;				/**< Buffer size */
	uint32 bufferSizeUsed;			/**< Buffer size used */
	uint32 timeout;					/**< Master rx timeout */
	uint32 busyStart;				/**< Millisecond ticks captured at start of transaction (used for timeout) */
} Handle;

typedef enum V1TctlmCanTransport_TypeEnum {
	V1_TCTLM_CAN_TRANSPORT__TYPE_NONE = 0,				/**< Invalid */
	V1_TCTLM_CAN_TRANSPORT__TYPE_TC = 1,				/**< Telecommand */
	V1_TCTLM_CAN_TRANSPORT__TYPE_TC_EXT = 7,			/**< Telecommand Extended*/
	V1_TCTLM_CAN_TRANSPORT__TYPE_TC_RESP = 2,			/**< Telecommand Reply */
	V1_TCTLM_CAN_TRANSPORT__TYPE_TC_NACK = 3,			/**< Telecommand Request Invalid Reply */
	V1_TCTLM_CAN_TRANSPORT__TYPE_TLM = 4,				/**< Telemetry request */
	V1_TCTLM_CAN_TRANSPORT__TYPE_TLM_RESP = 5,			/**< Telemetry reply */
	V1_TCTLM_CAN_TRANSPORT__TYPE_TLM_RESP_EXT = 8,		/**< Telemetry Extended reply */
	V1_TCTLM_CAN_TRANSPORT__TYPE_TLM_NACK = 6,			/**< Telemetry Request Invalid Reply */
	V1_TCTLM_CAN_TRANSPORT__TYPE_EVENT = 9,				/**< Unsolicited event */
	V1_TCTLM_CAN_TRANSPORT__TYPE_USOL_TLM_FIRST = 10,	/**< Unsolicited telemetry first packet */
	V1_TCTLM_CAN_TRANSPORT__TYPE_USOL_TLM_BODY = 11,	/**< Unsolicited telemetry body packet */
	V1_TCTLM_CAN_TRANSPORT__TYPE_USOL_TLM_LAST = 12,	/**< Unsolicited telemetry last packet */
} V1TctlmCanTransport_Type;

typedef struct CubeADCS_TlmLogEntryMetadaStruct {
	uint32 counter;
	uint32 uptime;
	uint32 unixtime_sec;
	uint16 unixtime_ms;
} __attribute__((packed)) CubeADCS_TlmLogEntryMetada_t;

typedef struct CubeADCS_TlmLogEntryStruct_Test {
	CubeADCS_TlmLogEntryMetada_t MetaData;
	
	ADCS_RawGYRSensorTlm_Paylaod_t TLM0;	// ID 204
	// ADCS_XXXXXTlm_Payload_t TLMX; // ID XXX

} CubeADCS_TlmLogEntry_Test_t;

typedef struct CubeADCS_TlmLogFrameStruct_Test {
	uint8 inclusionMask[5];
	CubeADCS_TlmLogEntry_Test_t Entry[CUBESPACE_MAX_ENTRY_NUM];
} CubeADCS_TlmLogFrame_Test_t;

/**
 * @brief Set Endpoint parameter
 */
void CUBE_EndpointInit(void);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                     	 < Telecommand(TC) Functions >                 	     */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * */
/**
 * @brief ID 1, Reset command : Perform a soft reset
 * @return DEVICE SUCCESS on success, otherwise DEVICE_ERR_WRITE
 */
int32 ADCS_Reset(void);


/********************************************************
 * 
 * COSMIC Actual Set Command Function
 * 
 ********************************************************/
int32 ADCS_SetCurrentUnixTime(const ADCS_CurrentUnixTimeCmd_Payload_t *setVal);		// 2
int32 ADCS_SetErrorLogClear(void);	// 5
int32 ADCS_SetErrorLogSetting(const ADCS_ErrorLogSettingCmd_Payload_t *msg);	// 6
int32 ADCS_SetPersistConfig(void);	// 7
int32 ADCS_SetControlEstimationMode(const ADCS_ControlEstimationModeCmd_Payload_t *setVal);	// 42
int32 ADCS_SetDisableMagRwlMntMng(const ADCS_DisableMagRwlMntMngCmd_Payload_t *setVal);	// 43
int32 ADCS_SetReferenceIRCVector(const ADCS_ReferenceIRCVectorCmd_Payload_t *setVal);	// 47
int32 ADCS_SetReferenceLLHTarget(const ADCS_ReferenceLLHTargetCmd_Payload_t *setVal);	// 48
int32 ADCS_SetOrbitMode(const ADCS_OrbitModeCmd_Payload_t *setVal);	// 51
int32 ADCS_SetMagDeploy(const ADCS_MagDeployCmd_Payload_t *setVal);	// 52
int32 ADCS_SetReferenceRPYValues(const ADCS_ReferenceRPYvaluesCmd_Payload_t *setVal);	// 54
int32 ADCS_SetOpenLoopCmdMTQ(const ADCS_OpenLoopCmdMTQCmd_Payload_t *setVal);	// 55
int32 ADCS_SetPowerState(const ADCS_PowerStateCmd_Payload_t *setVal);	// 56
int32 ADCS_SetRunMode(const ADCS_RunModeCmd_Payload_t *setVal);	// 57
int32 ADCS_SetControlMode(const ADCS_ControlModeCmd_Payload_t *setVal);	// 58
int32 ADCS_SetWhlConfig(const ADCS_WhlConfigCmd_Payload_t *setVal);	// 59
int32 ADCS_SetSatelliteConfig(const ADCS_SatConfigCmd_Payload_t *setVal);	// 61
int32 ADCS_SetControllerConfig(const ADCS_ControllerConfig_Payload_t *setVal);	// 62
int32 ADCS_SetMag0MMTCalibConfig(const ADCS_Mag0MMTCalibConfigCmd_Payload_t *setVal);	// 63
int32 ADCS_SetDefaultModeConfig(const ADCS_DefaultModeConfigCmd_Payload_t *setVal);	// 64
int32 ADCS_SetMountingConfig(const ADCS_MountingConfigCmd_Payload_t *setVal);	// 65
int32 ADCS_SetMag1MMTCalibConfig(const ADCS_Mag1MMTCalibConfigCmd_Payload_t *setVal);	// 66
int32 ADCS_SetEstimatorConfig(const ADCS_EstimatorConfigCmd_Payload_t *setVal);	// 67
int32 ADCS_SetSatOrbitParamConfig(const ADCS_SatOrbitParamConfigCmd_Payload_t *setVal);	// 68
int32 ADCS_SetNodeSelectionConfig(const ADCS_NodeSelectionConfigCmd_Payload_t *setVal);	// 69
int32 ADCS_SetMTQConfig(const ADCS_MTQConfigCmd_Payload_t *setVal);	// 70
int32 ADCS_SetEstimationMode(const ADCS_EstimationModeCmd_Payload_t *setVal);	// 71
int32 ADCS_SetOperationalState(const ADCS_OperationalStateCmd_Payload_t *setVal);	// 72
int32 ADCS_SetMagSensingElmConfig(const ADCS_MagSensingElmConfigCmd_Payload_t *setVal);	// 77
int32 ADCS_SetTransferFrame(const ADCS_TransferFrameCmd_Payload_t *setVal);	// 79
int32 ADCS_SetUnsolicitTlmMsgSetup(const ADCS_UnsolicitTlmMsgSetupCmd_Payload_t *setVal);	// 112
int32 ADCS_SetUnsolicitEventMsgSetup(const ADCS_UnsolicitEventMsgSetupCmd_InternalPayload_t *setVal);	// 116
int32 ADCS_SetReqTlmLogTransferSetup(const ADCS_RequestTlmLogTransferSetupCmd_Payload_t *setVal);	// 117
int32 ADCS_SetInitiateEventLogTransfer(const ADCS_InitiateEventLogTransferCmd_Payload_t *setVal);	// 120

/********************************************************
 * 
 * COSMIC Actual Get Command Function (Get tlm)
 * 
 ********************************************************/
int32 ADCS_GetErrorLogSetting(ADCS_ErrorLogSettingTlm_Payload_t *returnVal);	// 132
int32 ADCS_GetCurrentUnixTime(ADCS_CurrentUnixTimeTlm_Payload_t *returnVal);	// 133
int32 ADCS_GetPersistConfigDiagnostic(ADCS_PersistConfigDiagnosticTlm_Payload_t *returnVal);	// 134
int32 ADCS_GetCommunicationStatus(ADCS_CommunicationStatusTlm_Payload_t *returnVal);	// 135
int32 ADCS_GetControlEstimationMode(ADCS_ControlEstimationModeTlm_Payload_t *returnVal);	// 150
int32 ADCS_GetReferenceIRCVector(ADCS_ReferenceIRCVectorTlm_Payload_t *returnVal);	// 156
int32 ADCS_GetReferenceLLHTarget(ADCS_ReferenceLLHTargetTlm_Payload_t *returnVal);	// 157
int32 ADCS_GetOrbitMode(ADCS_OrbitModeTlm_Payload_t *returnVal);	// 162
int32 ADCS_GetHealthTlmMMT(ADCS_HealthTlmMMTTlm_Payload_t *returnVal);	// 167
int32 ADCS_GetRawCubeSenseSun(ADCS_RawCubeSenseSunTlm_Payload_t *returnVal);	// 170
int32 ADCS_GetReferenceRPYvalues(ADCS_ReferenceRPYvaluesTlm_Payload_t *returnVal);	// 181
int32 ADCS_GetOpenLoopCmdMTQ(ADCS_OpenLoopCmdMTQTlm_Payload_t *returnVal);	// 182
int32 ADCS_GetPowerState(ADCS_PowerStateTlm_Payload_t *returnVal);	// 183
int32 ADCS_GetRunMode(ADCS_RunModeTlm_Payload_t *returnVal);	// 184
int32 ADCS_GetControlMode(ADCS_ControlModeTlm_Payload_t *returnVal);	// 185
int32 ADCS_GetWhlConfig(ADCS_WhlConfigTlm_Payload_t *returnVal);	// 186
int32 ADCS_GetSatelliteConfig(ADCS_SatelliteConfigTlm_Payload_t *returnVal);	// 189
int32 ADCS_GetControllerConfig(ADCS_ControllerConfigTlm_Payload_t *returnVal);	// 190
int32 ADCS_GetMag0MMTCalibConfig(ADCS_Mag0MMTCalibConfigTlm_Payload_t *returnVal);	// 191
int32 ADCS_GetDefaultModeConfig(ADCS_DefaultModeConfigTlm_Payload_t *returnVal);	// 192
int32 ADCS_GetMountingConfig(ADCS_MountingConfigTlm_Payload_t *returnVal);	// 193
int32 ADCS_GetMag1MMTCalibConfig(ADCS_Mag1MMTCalibConfigTlm_Payload_t *returnVal);	// 194
int32 ADCS_GetEstimatorConfig(ADCS_EstimatorConfigTlm_Payload_t *returnVal);	// 195
int32 ADCS_GetSatOrbitParamConfig(ADCS_SatOrbitParamConfigTlm_Payload_t *returnVal);	// 196
int32 ADCS_GetNodeSelectionConfig(ADCS_NodeSelectionConfigTlm_Payload_t *returnVal);	// 197
int32 ADCS_GetMTQConfig(ADCS_MTQConfigTlm_Payload_t *returnVal);	// 198
int32 ADCS_GetEstimationMode(ADCS_EstimationModeTlm_Payload_t *returnVal);	// 199
int32 ADCS_GetOperationalState(ADCS_OperationalStateTlm_Payload_t *returnVal);	// 200
int32 ADCS_GetRawCSSSensor(ADCS_RawCSSSensorTlm_Payload_t *returnVal);	// 203
int32 ADCS_GetRawGYRSensor(ADCS_RawGYRSensorTlm_Paylaod_t *returnVal);	// 204
int32 ADCS_GetCalibratedGYRSensor(ADCS_CalibratedGYRSensorTlm_Payload_t *returnVal);	// 207
int32 ADCS_GetDataFrame(ADCS_DataFrameTlm_Payload_t *returnVal);	// 219
int32 ADCS_GetInfoFramInMemory(ADCS_InfoFrameInMemoryTlm_Payload_t *returnVal);	// 220
int32 ADCS_GetMagSensingElmConfig(ADCS_MagSensingElmConfigTlm_Payload_t *returnVal);	// 221
int32 ADCS_GetTlmLogInclMask(ADCS_TlmLogInclMaskTlm_Payload_t *returnVal);	// 227
int32 ADCS_GetUnsolicitTlmMsgSetup(ADCS_UnsolicitTlmMsgSetupTlm_Payload_t *returnVal);	// 228
int32 ADCS_GetUnsolicitEventMsgSetup(ADCS_UnsolicitEventMsgSetupTlm_Payload_t *returnVal);	// 233
int32 ADCS_GetTlmLogStatusResponse(ADCS_TlmLogStatusResponseTlm_Payload_t *returnVal);	// 234
int32 ADCS_GetEventLogStatusResponse(ADCS_EventLogStatusResponseTlm_Payload_t *returnVal);	// 235
int32 ADCS_GetPortMap(ADCS_PortMapTlm_Payload_t *returnVal);	// 239


/**
 * Report function
 */
void ADCS_HandleReport(int32 Status, uint8_t CC, void *ReadData, uint16_t ReadSize);

/**
 * Event Function
 */
void ADCS_HandleEvent(const ADCS_EventEntry_t *Event);
void ADCS_ListenEventTask(void);


/**
 * Additional Function
 */

int32 ADCS_COMM_InitAngRateEst(void);

#endif