/*
36개 이상의 에러 코드 정의
(NULL 포인터
, 타임아웃
, CAN 에러
, CRC 실패
, TCTLM NACK 에러 등)
*/
#ifndef ADCS_CUBE_ERR_H
#define ADCS_CUBE_ERR_H

#include "common_types.h"

#define CUBEOBC_ERROR_OK						((int32)0)	/**< No error */

#define CUBEOBC_ERROR_NULLPTR					((int32)1)	/**< NULL pointer */
#define CUBEOBC_ERROR_SIZE						((int32)2)	/**< Size incorrect */
#define CUBEOBC_ERROR_SIZEL						((int32)3)	/**< Size too low */
#define CUBEOBC_ERROR_SIZEH						((int32)4)	/**< Size too high */
#define CUBEOBC_ERROR_OVRRUN					((int32)5)	/**< Overrun */
#define CUBEOBC_ERROR_PARAM						((int32)6)	/**< Parameter error (out of range) */
#define CUBEOBC_ERROR_TOUT						((int32)7)	/**< Timeout */
#define CUBEOBC_ERROR_NACK						((int32)8)	/**< TCTLM comms Nack */
#define CUBEOBC_ERROR_BUSY						((int32)9)	/**< Busy */
#define CUBEOBC_ERROR_FRAME						((int32)10)	/**< Frame */
#define CUBEOBC_ERROR_CRC						((int32)11)	/**< CRC */
#define CUBEOBC_ERROR_READ						((int32)12)	/**< Read */
#define CUBEOBC_ERROR_WRITE						((int32)13)	/**< Write */
#define CUBEOBC_ERROR_CAN_ID					((int32)14)	/**< CAN ID type error */
#define CUBEOBC_ERROR_CAN_ERR					((int32)15)	/**< CAN frame error */
#define CUBEOBC_ERROR_UKN_NACK					((int32)16)	/**< Unknown NACK */
#define CUBEOBC_ERROR_NODE_TYPE					((int32)17)	/**< Invalid node type */
#define CUBEOBC_ERROR_FTP						((int32)18)	/**< CubeSpace file upload internal error */
#define CUBEOBC_ERROR_USAGE						((int32)19)	/**< Usage error */
#define CUBEOBC_ERROR_AUTOD						((int32)20)	/**< Auto-Discovery error */
#define CUBEOBC_ERROR_IMG						((int32)21)	/**< Image error */
#define CUBEOBC_ERROR_EXIST						((int32)22)	/**< Does not exist error */
#define CUBEOBC_ERROR_USER_DATA					((int32)23)	/**< User data error */
#define CUBEOBC_ERROR_COMMIT					((int32)24)	/**< Commit error */
#define CUBEOBC_ERROR_TCTLM_PROTOCOL			((int32)25)	/**< TCTLM protocol error */
#define CUBEOBC_ERROR_UNKNOWN					((int32)26)	/**< General unexpected/unknown error */
#define CUBEOBC_ERROR_TLM_SIZE					((int32)27)	/**< Telemetry response size error */
#define CUBEOBC_ERROR_TCTLM_ID					((int32)28)	/**< TCTLM response ID does not match the request */

#define CUBEOBC_ERROR_TCTLM_INVALID_ID			((int32)50)	/**< TCTLM Nack - invalid ID */
#define CUBEOBC_ERROR_TCTLM_INVALID_LENGTH		((int32)51)	/**< TCTLM Nack - invalid length */
#define CUBEOBC_ERROR_TCTLM_INVALID_PARAM		((int32)52)	/**< TCTLM Nack - invalid parameter data */
#define CUBEOBC_ERROR_TCTLM_CRC					((int32)53)	/**< TCTLM Nack - CRC failed */
#define CUBEOBC_ERROR_TCTLM_NOT_IMPLEMENTED		((int32)54)	/**< TCTLM Nack - not implemented */
#define CUBEOBC_ERROR_TCTLM_BUSY				((int32)55)	/**< TCTLM Nack - busy */
#define CUBEOBC_ERROR_TCTLM_SEQUENCE			((int32)56)	/**< TCTLM Nack - sequence */
#define CUBEOBC_ERROR_TCTLM_INTERNAL			((int32)57)	/**< TCTLM Nack - internal */
#define CUBEOBC_ERROR_TCTLM_PASS_TOUT			((int32)58)	/**< TCTLM Nack - pass-through timeout */
#define CUBEOBC_ERROR_TCTLM_PASS_TARGET			((int32)59)	/**< TCTLM Nack - pass-through target */

#define CUBEOBC_ERROR_CSP_RECV_TIMEOUT			((int32)70)	/**< CSP - receive timeout */
#define CUBEOBC_ERROR_CSP_BUFFER_NONE			((int32)71)	/**< CSP - failed to acquire a buffer */

#define CUBEOBC_ERROR_TODO						((int32)65535)	/**< Not implemented / TODO */


/**
 * @brief Error Code - see cubeObc_errorDef.h
 */
typedef int32_t ErrorCode;


/**
 * @brief Comms Error (Nack) Definitions
 */
typedef enum Tctlm_ErrorEnum {
	TCTLM__ERROR_OK = 0,				/*!< All Good */
	TCTLM__ERROR_INVALID_ID = 1,		/*!< Invalid TCTLM ID */
	TCTLM__ERROR_INVALID_LENGTH = 2,	/*!< Invalid Parameter Length */
	TCTLM__ERROR_INVALID_PARAM = 3,		/*!< Invalid Parameter Data */
	TCTLM__ERROR_CRC = 4,				/*!< CRC failed */
	TCTLM__ERROR_NOT_IMPLEMENTED = 5,	/*!< Request not supported for this firmware */
	TCTLM__ERROR_BUSY = 6,				/*!< Firmware cannot accept another command right now */
	TCTLM__ERROR_SEQUENCE = 7,			/*!< Command not possible in current firmware state */
	TCTLM__ERROR_INTERNAL = 8,			/*!< Internal Request Failure */
	TCTLM__ERROR_PASS_TIMEOUT = 9,		/*!< Pass through Request Timeout */
	TCTLM__ERROR_PASS_TARGET = 10,		/*!< Pass through target is invalid (pass through is disabled) */
} Tctlm_Error;

#endif