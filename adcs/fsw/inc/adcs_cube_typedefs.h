/*
CubeADCS 노드 타입(CubeComputer, CubeSense, CubeWheel 등)
, CAN 통신 엔드포인트
, TCTLM 프로토콜 구조체 정의
*/

#ifndef ADCS_CUBE_DEFS_H
#define ADCS_CUBE_DEFS_H

#include "common_types.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                      EndPoint enum & struct Definition                    */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * */
/**
 * @brief Enumeration for TypesCommonFrameworkTypes1_NodeType
*/
typedef enum TypesCommonFrameworkTypes1_NodeTypeEnum {
    TYPES_COMMON_FRAMEWORK_TYPES_1__NODE_TYPE_INVALID = 0,                       /**< Invalid Node Type */
    TYPES_COMMON_FRAMEWORK_TYPES_1__NODE_TYPE_CUBE_COMPUTER = 1,                 /**< CubeComputer Type - version independant */
    TYPES_COMMON_FRAMEWORK_TYPES_1__NODE_TYPE_CUBE_SENSE = 2,                    /**< CubeSense Type - version independant */
    TYPES_COMMON_FRAMEWORK_TYPES_1__NODE_TYPE_CUBE_WHEEL = 3,                    /**< CubeWheel Type - version independant */
    TYPES_COMMON_FRAMEWORK_TYPES_1__NODE_TYPE_CUBE_IR = 4,                       /**< CubeIR Type - version independant */
    TYPES_COMMON_FRAMEWORK_TYPES_1__NODE_TYPE_CUBE_MAG_DEPLOY = 5,               /**< CubeMag Deploy Type - version independant */
    TYPES_COMMON_FRAMEWORK_TYPES_1__NODE_TYPE_CUBE_MAG_COMPACT = 6,              /**< CubeMag Compact Type - version independant */
    TYPES_COMMON_FRAMEWORK_TYPES_1__NODE_TYPE_CUBE_STAR = 7,                     /**< CubeStar Type - version independant */
    TYPES_COMMON_FRAMEWORK_TYPES_1__NODE_TYPE_CUBE_AURIGA = 8,                   /**< CubeAuriga Type - version independant */
    TYPES_COMMON_FRAMEWORK_TYPES_1__NODE_TYPE_CUBE_NODE = 9,                     /**< CubeNode General Type - version independant */
    TYPES_COMMON_FRAMEWORK_TYPES_1__NODE_TYPE_CUBE_NODE_SLT = 10,                /**< CubeNode SLT Type - version independant */
    TYPES_COMMON_FRAMEWORK_TYPES_1__NODE_TYPE_CUBE_NODE_PST3S = 11,              /**< CubeNode PST3S Type - version independant */
    TYPES_COMMON_FRAMEWORK_TYPES_1__NODE_TYPE_CUBE_NODE_NSSRWL = 12,             /**< CubeNode NSSRWL Type - version independant */
    TYPES_COMMON_FRAMEWORK_TYPES_1__NODE_TYPE_CUBE_NODE_QUAD = 16,               /**< CubeNodeQuad General Type - version independant */
    TYPES_COMMON_FRAMEWORK_TYPES_1__NODE_TYPE_CUBE_NODE_QUAD_PST3S = 17,         /**< CubeNodeQuad PST3S Type - version independant */
    TYPES_COMMON_FRAMEWORK_TYPES_1__NODE_TYPE_CUBE_NODE_QUAD_NSSRWL = 18,        /**< CubeNodeQuad NSSRWL Type - version independant */
    TYPES_COMMON_FRAMEWORK_TYPES_1__NODE_TYPE_CUBE_NODE_QUAD_LITEFUFORS = 19,    /**< CubeNodeQuad LITEFUFORS Type - version independant */
} TypesCommonFrameworkTypes1_NodeType;

/**
 * @brief Type of transport to use for reaching node
 */
typedef enum TypeDef_CommsEndpointTypeEnum {
	TYPEDEF__COMMS_ENDPOINT_CAN = 0,	/**< Use CAN Slave Bus */
	// TYPEDEF__COMMS_ENDPOINT_I2C = 1,	/**< Use I2C Slave Bus */
	// TYPEDEF__COMMS_ENDPOINT_UART = 2,	/**< Use UART Slave Bus */

	TYPEDEF__COMMS_ENDPOINT_MAX			/**< Max endpoints */
} TypeDef_CommsEndpointType;

/**
 * @brief Type of protocol to use over transport
 */
typedef enum TypeDef_CommsProtocolEnum {
	TYPEDEF__COMMS_PROTOCOL_CUBESPACE = 0,	/**< CubeSpace protocol */
	TYPEDEF__COMMS_PROTOCOL_CSP,			/**< Cubesat-Space-Protocol (CSP) - CAN endpoint only */
} TypeDef_CommsProtocol;

/**
 * @brief Endpoint type and address
 */
typedef struct TypeDef_TctlmEndpointStruct {
	TypesCommonFrameworkTypes1_NodeType nodeType;	/**< Node Type for endpoint */
	TypeDef_CommsEndpointType type;					/**< Endpoint type */
	TypeDef_CommsProtocol proto;					/**< Endpoint protocol */
    uint32 addr;									/**< Endpoint address */
    uint32 addrPass;								/**< Endpoint address for passthrough (CAN only) */
    uint8 cspSrcPort;								/**< Source port to use if using CSP protocol */
    uint32 timeout;									/**< Transaction timeout */
    bool passthrough;							    /**< Signal passthrough transaction */
} TypeDef_TctlmEndpoint;

/**
 * @brief Endpoint type and address
 */
typedef struct TctlmCommsMasterSvc_EndpointStruct {
	TypeDef_TctlmEndpoint endpoint;		/**< Generic endpoint */
	uint8 id;								/**< TCTLM ID */
} TctlmCommsMasterSvc_Endpoint;

/* End of EndPoint enum & struct Definition */



#endif