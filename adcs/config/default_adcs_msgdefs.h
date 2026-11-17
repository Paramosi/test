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
 *   Specification for the ADCS command and telemetry
 *   message constant definitions.
 *
 *  For ADCS this is only the function/command code definitions
 */
#ifndef ADCS_MSGDEFS_H
#define ADCS_MSGDEFS_H

#include "common_types.h"
#include "adcs_fcncodes.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                   << ADCS BCN, HK, AOD Structures >>                      */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
typedef struct
{
    /** Combined Power State
     *  | 7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
     *  +--------------------------------------+
     *  |Rsv|RWL0|RWL1|RWL2|MAG0|GYRO|FSS0|HSS0|
     *  +--------------------------------------+
     */
    uint8 PowerState; // ID 183

    uint8 ControlMode; // ID 185

    float GYR0CalibratedRateXComponent;
    float GYR0CalibratedRateYComponent;
    float GYR0CalibratedRateZComponent; // ID 207, 12bytes

} __attribute__((packed)) ADCS_BcnTlm_Payload_t; /* Total 14 bytes */

typedef struct
{
    uint16 MAG0MCUCurrent; // 2 bytes, ID 167

    int16  FSS0MCUTemperature;
    uint16 FSS0MCUCurrent;
    uint16 FSS0MCUVoltage;
    uint8  FSS0CAMSRAMOvercurrent; // combined, 7 bytes, ID 168

    float MTQ1PositiveCurrentAverage;
    float MTQ1NegativeCurrentAverage;
    float MTQ2PositiveCurrentAverage;
    float MTQ2NegativeCurrentAverage;
    float MTQ3PositiveCurrentAverage;
    float MTQ3NegativeCurrentAverage;
    uint8 MTQPolarity; // MTQ 1,2,3 (combined), 25 bytes, ID 169

    int16  HSS0MCUTemperature;
    uint16 HSS0MCUCurrent;
    uint16 HSS0MCUVoltage; // 6 bytes, ID 217

    int16  RWL0MCUTemperature;
    uint16 RWL0MCUCurrent;
    uint16 RWL0BatteryVoltage;
    uint16 RWL0BatteryCurrent;
    int16  RWL1MCUTemperature;
    uint16 RWL1MCUCurrent;
    uint16 RWL1BatteryVoltage;
    uint16 RWL1BatteryCurrent;
    int16  RWL2MCUTemperature;
    uint16 RWL2MCUCurrent;
    uint16 RWL2BatteryVoltage;
    uint16 RWL2BatteryCurrent; // 24 bytes, ID 218

    int16 MTQ0OpenLoopOnTimeCommand;
    int16 MTQ1OpenLoopOnTimeCommand;
    int16 MTQ2OpenLoopOnTimeCommand; // 6bytes, ID 182

    uint8 FSS0CaptureResult;
    uint8 FSS0DetectionResult; // 2bytes, ID 170

    uint8 HSS0CaptureResult;
    uint8 HSS0DetectionResult; // 2bytes, ID 179

    uint32 bytesReceived; // 4bytes, ID 158

    float  mtq0Mmax;  /**< MTQ0 maximum dipole moment  (measurment unit is [A.m^2] */
    float  mtq1Mmax;  /**< MTQ1 maximum dipole moment  (measurment unit is [A.m^2] */
    float  mtq2Mmax;  /**< MTQ2 maximum dipole moment  (measurment unit is [A.m^2] */
    uint16 onTimeMax; /**< Maximum magnetorquer on-time  (measurment unit is [ms]) */
    float  mtqFfac;
        /**< LPF factor for magnetorquer commands. Set to zero for no filtering  (valid range is between 0  and 1 ) */ // 18 bytes, ID 198

    uint8 css0Raw;
    uint8 css1Raw;
    uint8 css2Raw;
    uint8 css3Raw;
    uint8 css4Raw;
    uint8 css5Raw;
    uint8 css6Raw;
    uint8 css7Raw;
    uint8 css8Raw;
    uint8 css9Raw;
    uint8  rawCssIsValid:1; // 11bytes, ID 203

    int16 cssCalVecX;
    int16 cssCalVecY;
    int16 cssCalVecZ;
    uint8  calCssIsValid:1; // 7 bytes, ID 206
} ADCS_HkTlm_Payload_t;  // 107 Bytes

typedef struct
{
    uint32 unixTimeSeconds;
    uint32 unixTimeNanoSeconds; // 8bytes, ID 133

    float RWL0MeasuredSpeed;
    float RWL1MeasuredSpeed;
    float RWL2MeasuredSpeed; // 12 bytes, ID 205

    int16 FSS0AlphaAngle;
    int16 FSS0BetaAngle; // 4bytes, ID 170

    int16  SatelliteGeocentricLatitude;
    int16  SatelliteLongitude;
    uint32 SatelliteAltitude;
    int16  SunORCModelXComponent;
    int16  SunORCModelYComponent;
    int16  SunORCModelZComponent;
    int16  SunBetaAngleWithOrbitPlane; // 16 bytes, ID 174

    int16 hss0CalVecX;
    int16 hss0CalVecY;
    int16 hss0CalVecZ; // 6 bytes, ID 176

    int16 mag0CalVecX;
    int16 mag0CalVecY;
    int16 mag0CalVecZ; // 6 bytes, ID 177

    int16 fss0CalVecX;
    int16 fss0CalVecY;
    int16 fss0CalVecZ; // 6 bytes, ID 178

    int16 hss0RawElevationAngle;
    int16 hss0RawRotationAngle; // 4 bytes, ID 179

    int16 MAG0RawVectorXComponent;
    int16 MAG0RawVectorYComponent;
    int16 MAG0RawVectorZComponent; // 6 bytes, ID 180

    float GYR0RawRateXComponent;
    float GYR0RawRateYComponent;
    float GYR0RawRateZComponent; // 12 bytes, ID 204

    uint32 TimeIntegerSeconds;
    int16  EstimatedRollAngle;
    int16  EstimatedPitchAngle;
    int16  EstimatedYawAngle;
    int16  EstimatedORCQuaternionQ0;
    int16  EstimatedORCQuaternionQ1;
    int16  EstimatedORCQuaternionQ2;
    int16  EstimatedORCQuaternionQ3;
    int16  EstimatedBodyRateORCXComponent;
    int16  EstimatedBodyRateORCYComponent;
    int16  EstimatedBodyRateORCZComponent;
    int16  EstimatedBodyRateIRCXComponent;
    int16  EstimatedBodyRateIRCYComponent;
    int16  EstimatedBodyRateIRCZComponent;
    int16  InnovationVectorXComponent;
    int16  InnovationVectorYComponent;
    int16  InnovationVectorZComponent;
    int16  StdDevOfEstimatedRateXComponent;
    int16  StdDevOfEstimatedRateYComponent;
    int16  StdDevOfEstimatedRateZComponent;
    int16  StdDevOfEstimatedQuaternionQ0Component;
    int16  StdDevOfEstimatedQuaternionQ1Component;
    int16  StdDevOfEstimatedQuaternionQ2Component; // 48 bytes, ID 210

    float ixx;              /**< Moment of inertia Ixx  (measurment unit is [kg.m^2]) */
    float iyy;              /**< Moment of inertia Iyy  (measurment unit is [kg.m^2]) */
    float izz;              /**< Moment of inertia Izz  (measurment unit is [kg.m^2]) */
    float ixy;              /**< Product of inertia Ixy  (measurment unit is [kg.m^2]) */
    float ixz;              /**< Product of inertia Ixz  (measurment unit is [kg.m^2]) */
    float iyz;              /**< Product of inertia Iyz  (measurment unit is [kg.m^2]) */
    int16 sunPointBodyVecX; /**< Sun-pointing body vector X component  */
    int16 sunPointBodyVecY; /**< Sun-pointing body vector Y component  */
    int16 sunPointBodyVecZ; /**< Sun-pointing body vector Z component  */
    int16 tgtTrackBodyVecX; /**< Target-tracking body vector X component  */
    int16 tgtTrackBodyVecY; /**< Target-tracking body vector Y component  */
    int16 tgtTrackBodyVecZ; /**< Target-tracking body vector Z component  */ // 36 bytes, ID 189
} ADCS_AOD_t;                                                                // 164 Bytes

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                     ADCS Set Cmd Payload Structures                       */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* FORMAT OF STRUCT
typedef struct { // ID

}__attribute__((packed)) ADCS_***Cmd_Payload_t;
*/

typedef struct
{                                  // ID 2
    uint32 CurrentUnixseconds;     // Current Unix time s. (Unit of measure is [s])
    uint32 CurrentUnixNanoseconds; // Current Unix time ns. (Unit of measure is [ns])
} __attribute__((packed)) ADCS_CurrentUnixTimeCmd_Payload_t;

typedef struct
{ // ID 6
    uint8   ActiveState : 1;
    uint8   BufferFullAction : 1;
    uint8_t Reserved : 6; // Padding
} __attribute__((packed)) ADCS_ErrorLogSettingCmd_Payload_t;

// typedef struct { // ID 7 - Noarg

// }

typedef struct
{ // ID 42
    uint8  ControlMode;
    uint8  MainEstimatorMode;
    uint8  BackupEstimatorMode;
    uint16 ControlTimeout;
} __attribute__((packed)) ADCS_ControlEstimationModeCmd_Payload_t;

typedef struct
{ // ID 43
    uint16 Duration;
} __attribute__((packed)) ADCS_DisableMagRwlMntMngCmd_Payload_t;

typedef struct
{ // ID 47
    float ECIPointingVectorX;
    float ECIPointingVectorY;
    float ECIPointingVectorZ;
} __attribute__((packed)) ADCS_ReferenceIRCVectorCmd_Payload_t;

typedef struct
{ // ID 48
    float TargetLatitude;
    float TargetLongiTude;
    float TargetAltitude;
} __attribute__((packed)) ADCS_ReferenceLLHTargetCmd_Payload_t;

typedef struct
{ // ID 51
    uint8 OrbitMode;
} __attribute__((packed)) ADCS_OrbitModeCmd_Payload_t;

typedef struct
{ // ID 52
    uint8 DeployMAG0 : 1;
    uint8 DeployMAG1 : 1;

    uint8 Spare : 6; // Explicit declaration
} __attribute__((packed)) ADCS_MagDeployCmd_Payload_t;

typedef struct
{ // ID 54
    float Roll;
    float Pitch;
    float Yaw;
} __attribute__((packed)) ADCS_ReferenceRPYvaluesCmd_Payload_t;

typedef struct
{ // ID 55
    int16 MTQ0_OpenLoopCmd;
    int16 MTQ1_OpenLoopCmd;
    int16 MTQ2_OpenLoopCmd;
} __attribute__((packed)) ADCS_OpenLoopCmdMTQCmd_Payload_t;

typedef struct
{ // ID 56
    uint8 RWL0;
    uint8 RWL1;
    uint8 RWL2;
    uint8 RWL3;
    uint8 MAG0;
    uint8 MAG1;
    uint8 GYR0;
    uint8 GYR1;
    uint8 FSS0;
    uint8 FSS1;
    uint8 FSS2;
    uint8 FSS3;
    uint8 HSS0;
    uint8 HSS1;
    uint8 STR0;
    uint8 STR1;
    uint8 ExtSensor0;
    uint8 ExtSensor1;
    uint8 ExtGYR0;
    uint8 ExtGYR1;
} __attribute__((packed)) ADCS_PowerStateCmd_Payload_t;

typedef struct
{ // ID 57
    uint8 RunMode;
} __attribute__((packed)) ADCS_RunModeCmd_Payload_t;

typedef struct
{ // ID 58
    uint8  ControlMode;
    uint16 Controltimeout;
} __attribute__((packed)) ADCS_ControlModeCmd_Payload_t;

typedef struct
{ // ID 59
    float Rwl0Inertia;
    float Rwl0MaxMomentum;
    float Rwl0MaxToque;
    float Rwl1Inertia;
    float Rwl1MaxMomentum;
    float Rwl1MaxToque;
    float Rwl2Inertia;
    float Rwl2MaxMomentum;
    float Rwl2MaxToque;
    float Rwl3Inertia;
    float Rwl3MaxMomentum;
    float Rwl3MaxToque;
    float WheelRampTorque;

    uint8 WheelScheme;
    uint8 FailedWheelID;

    float PyramidNominalMomentum;
    float PyramidTiltAngle;
} __attribute__((packed)) ADCS_WhlConfigCmd_Payload_t;

typedef struct
{ // ID 61
    float Ixx;
    float Iyy;
    float Izz;
    float Ixy;
    float Ixz;
    float Iyz;

    int16 SunPointingBodyVectorX;
    int16 SunPointingBodyVectorY;
    int16 SunPointingBodyVectorZ;

    int16 TargetTrackingBodyVectorX;
    int16 TargetTrackingBodyVectorY;
    int16 TargetTrackingBodyVectorZ;

    int16 SatTrackingBodyVectorX;
    int16 SatTrackingBodyVectorY;
    int16 SatTrackingBodyVectorZ;
} __attribute__((packed)) ADCS_SatConfigCmd_Payload_t;

typedef struct
{                                           // ID 62 (GS should sent properly)
    uint8_t DefaultControlMode;             // ENUM: Default control mode
    float   DetumblingDampingGain;          // Kd
    float   SunSpinGainSunlit;              // KDSun
    float   SunSpinGainEclipse;             // KDecel
    float   DetumblingSpinGain;             // Ks
    float   FastBDotGain;                   // Kdf
    float   YMomentumNutationDampingGain;   // Kn
    float   YMomentumQuatGain;              // Kq
    float   XAxisGGQuatGain;                // Kqx
    float   YAxisGGQuatGain;                // Kqy
    float   ZAxisGGQuatGain;                // Kqz
    float   WheelDesaturationGain;          // Kh
    float   YMomentumProportionalGain;      // Kp1
    float   YMomentumDerivativeGain;        // Kd1
    float   RWheelProportionalGain;         // Kp2
    float   RWheelDerivativeGain;           // Kd2
    float   TrackingProportionalGain;       // Kp3
    float   TrackingDerivativeGain;         // Kd3
    float   TrackingIntegralGain;           // Ki3
    float   ReferenceSpinRate;              // wy-ref [degps]
    float   ReferenceWheelMomentum;         // H-ref [Nms]
    float   YWheelBiasMomentum;             // Hy-bias [Nms]
    float   ReferenceSpinRateRWspinControl; // [degps]
    float   SunKeepOutAngle;                // [deg]
    float   RollLimitAngle;                 // [deg]

    /* 3 one-bit flags + 5-bit reserved packed into 1 byte */
    struct __attribute__((packed))
    {
        uint8_t YawCompensationForEarthRotation : 1; // BOOL
        uint8_t EnableSunTrackingInEclipse : 1;      // BOOL
        uint8_t EnableSunAvoidance : 1;              // BOOL
        uint8_t Reserved : 5;                        // Padding to match spec
    } flags;
} __attribute__((packed)) ADCS_ControllerConfig_Payload_t;

typedef struct
{ // ID 63
    int16 MMT_Ch1Offset;
    int16 MMT_Ch2Offset;
    int16 MMT_Ch3Offset;

    int16 MMT_SensitivityMAT_S11;
    int16 MMT_SensitivityMAT_S22;
    int16 MMT_SensitivityMAT_S33;
    int16 MMT_SensitivityMAT_S12;
    int16 MMT_SensitivityMAT_S13;
    int16 MMT_SensitivityMAT_S21;
    int16 MMT_SensitivityMAT_S23;
    int16 MMT_SensitivityMAT_S31;
    int16 MMT_SensitivityMAT_S32;
} __attribute__((packed)) ADCS_Mag0MMTCalibConfigCmd_Payload_t;

typedef struct
{ // ID 64
    uint8_t DefaultRunMode;
    uint8_t DefaultOperationalState;
    uint8_t DefaultControlModeInOpStateSafe;
    uint8_t DefaultControlModeInOpStateAuto;
} __attribute__((packed)) ADCS_DefaultModeConfigCmd_Payload_t;

typedef struct
{ // ID 65
    /* Stack & Actuators (ENUM, 1B each) */
    uint8 StackX_mounting; // StackX mounting (Table 43)
    uint8 StackY_mounting; // StackY mounting (Table 43)
    uint8 StackZ_mounting; // StackZ mounting (Table 43)
    uint8 MTQ0_mounting;   // MTQ0 mounting (Table 43)
    uint8 MTQ1_mounting;   // MTQ1 mounting (Table 43)
    uint8 MTQ2_mounting;   // MTQ2 mounting (Table 43)
    uint8 Wheel0_mounting; // Wheel0 mounting (Table 43)
    uint8 Wheel1_mounting; // Wheel1 mounting (Table 43)
    uint8 Wheel2_mounting; // Wheel2 mounting (Table 43)
    uint8 Wheel3_mounting; // Wheel3 mounting (Table 43)

    /* Pyramid RWL angles (INT16; deg = raw/100.0) */
    int16 PyramidRWL_alpha; // alpha angle
    int16 PyramidRWL_beta;  // beta angle
    int16 PyramidRWL_gamma; // gamma angle

    /* CSS mounting (ENUM, 1B each) */
    uint8 CSS0_mounting;
    uint8 CSS1_mounting;
    uint8 CSS2_mounting;
    uint8 CSS3_mounting;
    uint8 CSS4_mounting;
    uint8 CSS5_mounting;
    uint8 CSS6_mounting;
    uint8 CSS7_mounting;
    uint8 CSS8_mounting;
    uint8 CSS9_mounting;

    /* FSS0..3 angles (INT16; deg = raw/100.0) */
    int16 FSS0_alpha;
    int16 FSS0_beta;
    int16 FSS0_gamma;
    int16 FSS1_alpha;
    int16 FSS1_beta;
    int16 FSS1_gamma;
    int16 FSS2_alpha;
    int16 FSS2_beta;
    int16 FSS2_gamma;
    int16 FSS3_alpha;
    int16 FSS3_beta;
    int16 FSS3_gamma;

    /* HSS0..1 angles (INT16; deg = raw/100.0) */
    int16 HSS0_alpha;
    int16 HSS0_beta;
    int16 HSS0_gamma;
    int16 HSS1_alpha;
    int16 HSS1_beta;
    int16 HSS1_gamma;

    /* MAG0..1 angles (INT16; deg = raw/100.0) */
    int16 MAG0_alpha;
    int16 MAG0_beta;
    int16 MAG0_gamma;
    int16 MAG1_alpha;
    int16 MAG1_beta;
    int16 MAG1_gamma;

    /* STR0..1 angles (INT16; deg = raw/100.0) */
    int16 STR0_alpha;
    int16 STR0_beta;
    int16 STR0_gamma;
    int16 STR1_alpha;
    int16 STR1_beta;
    int16 STR1_gamma;

    /* External sensor 0/1 angles (INT16; deg = raw/100.0) */
    int16 ExtSensor0_alpha;
    int16 ExtSensor0_beta;
    int16 ExtSensor0_gamma;
    int16 ExtSensor1_alpha;
    int16 ExtSensor1_beta;
    int16 ExtSensor1_gamma;

    /* External gyro axis mounting (ENUM, 1B each; Table 43) */
    uint8 ExtGyro0_axis1_mounting;
    uint8 ExtGyro0_axis2_mounting;
    uint8 ExtGyro0_axis3_mounting;
    uint8 ExtGyro1_axis1_mounting;
    uint8 ExtGyro1_axis2_mounting;
    uint8 ExtGyro1_axis3_mounting;
} __attribute__((packed)) ADCS_MountingConfigCmd_Payload_t;

typedef struct
{ // ID 66
    int16 MMT_Ch1Offset;
    int16 MMT_Ch2Offset;
    int16 MMT_Ch3Offset;

    int16 MMT_SensitivityMAT_S11;
    int16 MMT_SensitivityMAT_S22;
    int16 MMT_SensitivityMAT_S33;
    int16 MMT_SensitivityMAT_S12;
    int16 MMT_SensitivityMAT_S13;
    int16 MMT_SensitivityMAT_S21;
    int16 MMT_SensitivityMAT_S23;
    int16 MMT_SensitivityMAT_S31;
    int16 MMT_SensitivityMAT_S32;
} __attribute__((packed)) ADCS_Mag1MMTCalibConfigCmd_Payload_t;

typedef struct
{ // ID 67
    uint8 DefaultMainEstimatorMode;
    uint8 DefaultBackupEstimatorMode;
    float MAGMeasurementNoise;
    float CSSMeasurementNoise;
    float FSSMeasurementNoise;
    float HSSMeasurementNoise;
    float STRMeasurementNoise;
    float MMTRKFSystemNoise;
    float EKFSystemNoise;
    float NutationEpsilonCorrection;
    float NutationPsiCorrection;

    uint8 UseFSSinEKF : 1;
    uint8 UseCSSinEKF : 1;
    uint8 UseHSSinEKF : 1;
    uint8 UseSTRinEKF : 1;
    uint8 TriadVector1 : 4;
    uint8 TriadVector2 : 4;

    uint8 Spare : 4; // Explicit declaration

} __attribute__((packed)) ADCS_EstimatorConfigCmd_Payload_t;

typedef struct
{ // ID 68
    double Epoch;
    double Inclination;
    double RAAN;
    double Eccentricity;
    double AOP;
    double MeanAnomaly;
    double MeanMotion;
    double B_StarDrag;
} __attribute__((packed)) ADCS_SatOrbitParamConfigCmd_Payload_t;

typedef struct
{ // ID 69
    uint8 RSWSelectionFlags;
    uint8 MAGSelectionFlags;
    uint8 FSSSelectionFlags;
    uint8 HSSSelectionFlags;
    uint8 GYRSelectionFlags;
    uint8 STRSelectionFlags;
    uint8 GNSSSelectionFlags;
    uint8 ExtSensorSelectionFlags;
} __attribute__((packed)) ADCS_NodeSelectionConfigCmd_Payload_t;

typedef struct
{ // ID 70
    float  MTQ0MaxDipoleMoment;
    float  MTQ1MaxDipoleMoment;
    float  MTQ2MaxDipoleMoment;
    uint16 MaxMTQOnTime;
    uint16 MinMTQOnTime;
    float  MagneticControlFilterFactor;
} __attribute__((packed)) ADCS_MTQConfigCmd_Payload_t;

typedef struct
{ // ID 71
    uint8 MainEstimatorMode;
    uint8 BackupEstimatorMode;
} __attribute__((packed)) ADCS_EstimationModeCmd_Payload_t;

typedef struct
{ // ID 72
    uint8 OperationalState;
} __attribute__((packed)) ADCS_OperationalStateCmd_Payload_t;

typedef struct
{ // ID 77
    uint8 Mag0SensingElement : 1;
    uint8 Mag1SensingElement : 1;

    uint8 Spare : 6; // Explicit declaration
} __attribute__((packed)) ADCS_MagSensingElmConfigCmd_Payload_t;

typedef struct
{ // ID 79
    uint16 NextFrameNumber;
} __attribute__((packed)) ADCS_TransferFrameCmd_Payload_t;

typedef struct
{ // ID 112
    uint8 UARTTlmReturnInterval : 4;
    uint8 UART2TlmReturnInterval : 4;
    uint8 CANTlmRetrunInterval : 4;
    uint8 Reserved : 4;

    uint8 UARTTlmEDInclusionBitmask[5];
    uint8 UART2TlmEDInclusionBitmask[5];
    uint8 CANTlmEDInclusionBitmask[5];
} __attribute__((packed)) ADCS_UnsolicitTlmMsgSetupCmd_Payload_t;

typedef struct
{ // ID 116
    uint8 Flag;
} ADCS_UnsolicitEventMsgSetupCmd_ExternalPayload_t;

typedef struct
{ // ID 116
    uint8_t InfoUART : 1;
    uint8_t MinorUART : 1;
    uint8_t MajorUART : 1;
    uint8_t CriticalUART : 1;

    uint8_t InfoUART2 : 1;
    uint8_t MinorUART2 : 1;
    uint8_t MajorUART2 : 1;
    uint8_t CriticalUART2 : 1;

    uint8_t InfoCAN : 1;
    uint8_t MinorCAN : 1;
    uint8_t MajorCAN : 1;
    uint8_t CriticalCAN : 1;

    uint8_t Spare : 4; // Explicit declaration
} __attribute__((packed)) ADCS_UnsolicitEventMsgSetupCmd_InternalPayload_t;

typedef struct
{ // ID 117
    uint8  FilterType;
    uint32 UnixStartTime;
    uint32 UnixEndTime;
    uint32 NumberOfEntries;
    uint32 WriteCounter;
    uint8  TlmLogReturnInterval : 4;
    uint8  Reserved : 4;
    uint8  LogIDbitmask[5];
} __attribute__((packed)) ADCS_RequestTlmLogTransferSetupCmd_Payload_t;

typedef struct
{ // ID 120
    uint8_t  FilterType;
    uint32_t UnixStartTime;
    uint32_t UnixEndTime;
    uint32_t NumberOfEntry;
    uint32_t WriteCounter;

    struct __attribute__((packed))
    {
        uint8_t IncludeCriticalEVS : 1;
        uint8_t IncludeMajorWarningEVS : 1;
        uint8_t IncludeMinorWarningEVS : 1;
        uint8_t IncludeInfoEVS : 1;
        uint8_t IncludeCubeCom : 1;
        uint8_t IncludeRWL0 : 1;
        uint8_t IncludeRWL1 : 1;
        uint8_t IncludeRWL2 : 1;
        uint8_t IncludeRWL3 : 1;
        uint8_t IncludeFSS0 : 1;
        uint8_t IncludeFSS1 : 1;
        uint8_t IncludeFSS2 : 1;
        uint8_t IncludeFSS3 : 1;
        uint8_t IncludeHSS0 : 1;
        uint8_t IncludeHSS1 : 1;
        uint8_t IncludeSTR0 : 1;
        uint8_t IncludeSTR1 : 1;
        uint8_t IncludeMAG0 : 1;
        uint8_t IncludeMAG1 : 1;
        uint8_t IncludeExt0 : 1;
        uint8_t IncludeExt1 : 1;
        uint8_t Padding : 3;
    } flags;
} __attribute__((packed)) ADCS_InitiateEventLogTransferCmd_Payload_t;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                      ADCS Get Cmd Payload Structures                      */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* FORMAT OF STRUCT
typedef struct { // ID

}__attribute__((packed)) ADCS_***Tlm_Payload_t;
*/

typedef struct
{ // ID 132
    uint8   ActiveState : 1;
    uint8   BufferFullAction : 1;
    uint8_t Reserved : 6; // Padding
} __attribute__((packed)) ADCS_ErrorLogSettingTlm_Payload_t;

typedef struct
{                                  // ID 133
    uint32 CurrentUnixseconds;     // Current Unix time s. (Unit of measure is [s])
    uint32 CurrentUnixNanoseconds; // Current Unix time ns. (Unit of measure is [ns])
} __attribute__((packed)) ADCS_CurrentUnixTimeTlm_Payload_t;

typedef struct
{ // ID 134
    uint8_t  State;
    uint8_t  LastResult;
    uint32_t Timestamp;
} __attribute__((packed)) ADCS_PersistConfigDiagnosticTlm_Payload_t;

typedef struct
{ // ID 135
    uint16_t UART_TcCnt;
    uint16_t UART_TlmCnt;
    uint16_t UART_ErrSW;
    uint16_t UART_ErrHW;

    uint16_t UART2_TcCnt;
    uint16_t UART2_TlmCnt;
    uint16_t UART2_ErrSW;
    uint16_t UART2_ErrHW;

    uint16_t CAN_TcCnt;
    uint16_t CAN_TlmCnt;
    uint16_t CAN_ErrSW;
    uint16_t CAN_ErrHW;

    uint16_t I2C_TcCnt;
    uint16_t I2C_TlmCnt;
    uint16_t I2C_ErrSW;
    uint16_t I2C_ErrHW;
} __attribute__((packed)) ADCS_CommunicationStatusTlm_Payload_t;

typedef struct
{ // ID 150
    uint8  ControlMode;
    uint8  MainEstimatorMode;
    uint8  BackupEstimatorMode;
    uint16 ControlTimeout;
} __attribute__((packed)) ADCS_ControlEstimationModeTlm_Payload_t;

typedef struct
{ // ID 156
    float ECIPointingVectorX;
    float ECIPointingVectorY;
    float ECIPointingVectorZ;
} __attribute__((packed)) ADCS_ReferenceIRCVectorTlm_Payload_t;

typedef struct
{ // ID 157
    float Latitude;
    float Longitude;
    float Altitude;
} __attribute__((packed)) ADCS_ReferenceLLHTargetTlm_Payload_t;

typedef struct
{ // ID 162
    uint8 OrbitMode;
} __attribute__((packed)) ADCS_OrbitModeTlm_Payload_t;

typedef struct
{ // ID 167
    int16  Mag0MCUTemperature;
    uint16 Mag0MCUCurrent;
    uint16 Mag0MCUVoltage;
    int16  Mag0PrimaryTemperature;
    int16  Mag0RedundantTemperature;
    uint32 Mag0BurnCurrent;
    uint8  Mag0DeployPinState : 1;
    uint8  Mag0BurnPinState : 1;
    uint8  Mag0BurnUnderCurrent : 1;
    uint8  Mag0BurnOverCurrent : 1;
    uint8  Mag0DeployTimeout : 1;
    uint8  Padding1 : 3;

    int16  Mag1MCUTemperature;
    uint16 Mag1MCUCurrent;
    uint16 Mag1MCUVoltage;
    int16  Mag1PrimaryTemperature;
    int16  Mag1RedundantTemperature;
    uint32 Mag1BurnCurrent;
    uint8  Mag1DeployPinState : 1;
    uint8  Mag1BurnPinState : 1;
    uint8  Mag1BurnUnderCurrent : 1;
    uint8  Mag1BurnOverCurrent : 1;
    uint8  Mag1DeployTimeout : 1;
    uint8  Padding2 : 3; // Explicit declaration
} __attribute__((packed)) ADCS_HealthTlmMMTTlm_Payload_t;

typedef struct
{ // ID 170
    uint32 TimeSecond;
    uint32 TimeNanoSecond;
    int16  FSS0AlphaAngle;
    int16  FSS0BetaAngle;
    uint8  FSS0CaptureResult;
    uint8  FSS0DetectionResult;
    int16  FSS1AlphaAngle;
    int16  FSS1BetaAngle;
    uint8  FSS1CaptureResult;
    uint8  FSS1DetectionResult;
    int16  FSS2AlphaAngle;
    int16  FSS2BetaAngle;
    uint8  FSS2CaptureResult;
    uint8  FSS2DetectionResult;
    int16  FSS3AlphaAngle;
    int16  FSS3BetaAngle;
    uint8  FSS3CaptureResult;
    uint8  FSS3DetectionResult;
    uint8   ValidResult:1; // FSS0, 1, 2, 3
} __attribute__((packed)) ADCS_RawCubeSenseSunTlm_Payload_t;

typedef struct
{ // ID 173
    uint32 TimeSecond;
    uint32 TimeNanoSecond;
    int16  EstRoll;
    int16  EstPitch;
    int16  EstYaw;
    int16  EstORCQ0;
    int16  EstORCQ1;
    int16  EstORCQ2;
    int16  EstORCQ3;
    int16  EstGYRbiasX;
    int16  EstGYRbiasY;
    int16  EstGYRbiasZ;
    int16  EstORCBodyRateX;
    int16  EstORCBodyRateY;
    int16  EstORCBodyRateZ;
    int16  EstIRCBodyRateX;
    int16  EstIRCBodyRateY;
    int16  EstIRCBodyRateZ;
    float  EstGyroscTrqX;
    float  EstGyroscTrqY;
    float  EstGyroscTrqZ;
    int16  InnvVecX;
    int16  InnvVecY;
    int16  InnvVecZ;
    int16  StdEstRateX;
    int16  StdEstRateY;
    int16  StdEstRateZ;
    int16  StdEstQ0;
    int16  StdEstQ1;
    int16  StdEstQ2;
    uint8  ActiveEstMode;
} __attribute__((packed)) ADCS_BackupEstimatorTlm_Payload_t;

typedef struct
{ // ID 181
    float Roll;
    float Pitch;
    float Yaw;
} __attribute__((packed)) ADCS_ReferenceRPYvaluesTlm_Payload_t;

typedef struct
{ // ID 182
    int16 MTQ0_OpenLoopCmd;
    int16 MTQ1_OpenLoopCmd;
    int16 MTQ2_OpenLoopCmd;
} __attribute__((packed)) ADCS_OpenLoopCmdMTQTlm_Payload_t;

typedef struct
{ // ID 183
    uint8 RWL0;
    uint8 RWL1;
    uint8 RWL2;
    uint8 RWL3;

    uint8 MAG0;
    uint8 MAG1;

    uint8 GYR0;
    uint8 GYR1;

    uint8 FSS0;
    uint8 FSS1;
    uint8 FSS2;
    uint8 FSS3;

    uint8 HSS0;
    uint8 HSS1;

    uint8 STR0;
    uint8 STR1;

    uint8 ExtSensor0;
    uint8 ExtSensor1;

    uint8 ExtGYR0;
    uint8 ExtGYR1;
} __attribute__((packed)) ADCS_PowerStateTlm_Payload_t;

typedef struct
{ // ID 184
    uint8_t RunMode;
} __attribute__((packed)) ADCS_RunModeTlm_Payload_t;

typedef struct
{ // ID 185
    uint8  ControlMode;
    uint16 ControlTimeout;
} __attribute__((packed)) ADCS_ControlModeTlm_Payload_t;

typedef struct
{ // ID 186
    float Rwl0Inertia;
    float Rwl0MaxMomentum;
    float Rwl0MaxToque;
    float Rwl1Inertia;
    float Rwl1MaxMomentum;
    float Rwl1MaxToque;
    float Rwl2Inertia;
    float Rwl2MaxMomentum;
    float Rwl2MaxToque;
    float Rwl3Inertia;
    float Rwl3MaxMomentum;
    float Rwl3MaxToque;
    float WheelRampTorque;

    uint8 WheelScheme;
    uint8 FailedWheelID;

    float PyramidNominalMomentum;
    float PyramidTiltAngle;
} __attribute__((packed)) ADCS_WhlConfigTlm_Payload_t;

typedef struct
{ // ID 189
    float Ixx;
    float Iyy;
    float Izz;
    float Ixy;
    float Ixz;
    float Iyz;

    int16 SunPointingBodyVectorX;
    int16 SunPointingBodyVectorY;
    int16 SunPointingBodyVectorZ;

    int16 TargetTrackingBodyVectorX;
    int16 TargetTrackingBodyVectorY;
    int16 TargetTrackingBodyVectorZ;

    int16 SatTrackingBodyVectorX;
    int16 SatTrackingBodyVectorY;
    int16 SatTrackingBodyVectorZ;
} __attribute__((packed)) ADCS_SatelliteConfigTlm_Payload_t;

typedef struct
{                               // ID 190
    uint8_t DefaultControlMode; // ENUM (Table 14)

    float DetumblingDampingGain;       // Kd
    float SunSpinGain_Sunlit;          // KDSun
    float SunSpinGain_Eclipse;         // KDecel
    float DetumblingSpinGain;          // Ks
    float FastBDotGain;                // Kdf
    float YMomNutationDampingGain;     // Kn
    float YMomNutationDampingQuatGain; // Kq
    float XGGQuatGain;                 // Kqx
    float YGGQuatGain;                 // Kqy
    float ZGGQuatGain;                 // Kqz
    float WheelDesatControlGain;       // Kh
    float YMomProportionalGain;        // Kp1
    float YMomDerivativeGain;          // Kd1
    float RWheelProportionalGain;      // Kp2
    float RWheelDerivativeGain;        // Kd2
    float TrackingProportionalGain;    // Kp3
    float TrackingDerivativeGain;      // Kd3
    float TrackingIntegralGain;        // Ki3
    float ReferenceSpinRate_degps;     // wy-ref [degps]
    float ReferenceWheelMomentum_Nms;  // H-ref [Nms], must be < 0
    float YWheelBiasMomentum_Nms;      // Hy-bias [Nms]
    float RefSpinRate_RW_degps;        // for ConSunYawSpin RW control [degps]
    float SunKeepOutAngle_deg;         // [deg]
    float RollLimitAngle_deg;          // [deg]

    /* 3 one-bit flags + 5-bit reserved packed into 1 byte */
    struct __attribute__((packed))
    {
        uint8_t YawCompensationForEarthRotation : 1; // BOOL
        uint8_t EnableSunTrackingInEclipse : 1;      // BOOL
        uint8_t EnableSunAvoidance : 1;              // BOOL
        uint8_t Reserved : 5;                        // Padding to match spec
    } flags;
} __attribute__((packed)) ADCS_ControllerConfigTlm_Payload_t;

typedef struct
{ // ID 191
    int16 MMT_Ch1Offset;
    int16 MMT_Ch2Offset;
    int16 MMT_Ch3Offset;

    int16 MMT_SensitivityMAT_S11;
    int16 MMT_SensitivityMAT_S22;
    int16 MMT_SensitivityMAT_S33;
    int16 MMT_SensitivityMAT_S12;
    int16 MMT_SensitivityMAT_S13;
    int16 MMT_SensitivityMAT_S21;
    int16 MMT_SensitivityMAT_S23;
    int16 MMT_SensitivityMAT_S31;
    int16 MMT_SensitivityMAT_S32;
} __attribute__((packed)) ADCS_Mag0MMTCalibConfigTlm_Payload_t;

typedef struct
{ // ID 192
    uint8_t DefaultRunMode;
    uint8_t DefaultOperationalState;
    uint8_t DefaultControlModeInOpStateSafe;
    uint8_t DefaultControlModeInOpStateAuto;
} __attribute__((packed)) ADCS_DefaultModeConfigTlm_Payload_t;

typedef struct
{ // ID 193
    /* Stack & Actuators (ENUM, 1B each) */
    uint8_t StackX_mounting; // StackX mounting (Table 43)
    uint8_t StackY_mounting; // StackY mounting (Table 43)
    uint8_t StackZ_mounting; // StackZ mounting (Table 43)
    uint8_t MTQ0_mounting;   // MTQ0 mounting (Table 43)
    uint8_t MTQ1_mounting;   // MTQ1 mounting (Table 43)
    uint8_t MTQ2_mounting;   // MTQ2 mounting (Table 43)
    uint8_t Wheel0_mounting; // Wheel0 mounting (Table 43)
    uint8_t Wheel1_mounting; // Wheel1 mounting (Table 43)
    uint8_t Wheel2_mounting; // Wheel2 mounting (Table 43)
    uint8_t Wheel3_mounting; // Wheel3 mounting (Table 43)

    /* Pyramid RWL angles (INT16; deg = raw/100.0) */
    int16_t PyramidRWL_alpha; // alpha angle
    int16_t PyramidRWL_beta;  // beta angle
    int16_t PyramidRWL_gamma; // gamma angle

    /* CSS mounting (ENUM, 1B each) */
    uint8_t CSS0_mounting;
    uint8_t CSS1_mounting;
    uint8_t CSS2_mounting;
    uint8_t CSS3_mounting;
    uint8_t CSS4_mounting;
    uint8_t CSS5_mounting;
    uint8_t CSS6_mounting;
    uint8_t CSS7_mounting;
    uint8_t CSS8_mounting;
    uint8_t CSS9_mounting;

    /* FSS0..3 angles (INT16; deg = raw/100.0) */
    int16_t FSS0_alpha;
    int16_t FSS0_beta;
    int16_t FSS0_gamma;
    int16_t FSS1_alpha;
    int16_t FSS1_beta;
    int16_t FSS1_gamma;
    int16_t FSS2_alpha;
    int16_t FSS2_beta;
    int16_t FSS2_gamma;
    int16_t FSS3_alpha;
    int16_t FSS3_beta;
    int16_t FSS3_gamma;

    /* HSS0..1 angles (INT16; deg = raw/100.0) */
    int16_t HSS0_alpha;
    int16_t HSS0_beta;
    int16_t HSS0_gamma;
    int16_t HSS1_alpha;
    int16_t HSS1_beta;
    int16_t HSS1_gamma;

    /* MAG0..1 angles (INT16; deg = raw/100.0) */
    int16_t MAG0_alpha;
    int16_t MAG0_beta;
    int16_t MAG0_gamma;
    int16_t MAG1_alpha;
    int16_t MAG1_beta;
    int16_t MAG1_gamma;

    /* STR0..1 angles (INT16; deg = raw/100.0) */
    int16_t STR0_alpha;
    int16_t STR0_beta;
    int16_t STR0_gamma;
    int16_t STR1_alpha;
    int16_t STR1_beta;
    int16_t STR1_gamma;

    /* External sensor 0/1 angles (INT16; deg = raw/100.0) */
    int16_t ExtSensor0_alpha;
    int16_t ExtSensor0_beta;
    int16_t ExtSensor0_gamma;
    int16_t ExtSensor1_alpha;
    int16_t ExtSensor1_beta;
    int16_t ExtSensor1_gamma;

    /* External gyro axis mounting (ENUM, 1B each; Table 43) */
    uint8_t ExtGyro0_axis1_mounting;
    uint8_t ExtGyro0_axis2_mounting;
    uint8_t ExtGyro0_axis3_mounting;
    uint8_t ExtGyro1_axis1_mounting;
    uint8_t ExtGyro1_axis2_mounting;
    uint8_t ExtGyro1_axis3_mounting;
} __attribute__((packed)) ADCS_MountingConfigTlm_Payload_t;

typedef struct
{ // ID 194
    int16 MMT_Ch1Offset;
    int16 MMT_Ch2Offset;
    int16 MMT_Ch3Offset;

    int16 MMT_SensitivityMAT_S11;
    int16 MMT_SensitivityMAT_S22;
    int16 MMT_SensitivityMAT_S33;
    int16 MMT_SensitivityMAT_S12;
    int16 MMT_SensitivityMAT_S13;
    int16 MMT_SensitivityMAT_S21;
    int16 MMT_SensitivityMAT_S23;
    int16 MMT_SensitivityMAT_S31;
    int16 MMT_SensitivityMAT_S32;
} __attribute__((packed)) ADCS_Mag1MMTCalibConfigTlm_Payload_t;

typedef struct
{ // ID 195
    uint8 DefaultMainEstimatorMode;
    uint8 DefaultBackupEstimatorMode;
    float MAGMeasurementNoise;
    float CSSMeasurementNoise;
    float FSSMeasurementNoise;
    float HSSMeasurementNoise;
    float STRMeasurementNoise;
    float MMTRKFSystemNoise;
    float EKFSystemNoise;
    float NutationEpsilonCorrection;
    float NutationPsiCorrection;

    uint8 UseFSSinEKF : 1;
    uint8 UseCSSinEKF : 1;
    uint8 UseHSSinEKF : 1;
    uint8 UseSTRinEKF : 1;
    uint8 TriadVector1 : 4;
    uint8 TriadVector2 : 4;

    uint8 Spare : 4; // Explicit declaration

} __attribute__((packed)) ADCS_EstimatorConfigTlm_Payload_t;

typedef struct
{ // ID 196
    double Epoch;
    double Inclination;
    double RAAN;
    double Eccentricity;
    double AOP;
    double MeanAnomaly;
    double MeanMotion;
    double B_StarDrag;
} __attribute__((packed)) ADCS_SatOrbitParamConfigTlm_Payload_t;

typedef struct
{ // ID 197
    uint8 RWLSelectionFlags;
    uint8 MAGSelectionFlags;
    uint8 FSSSelectionFlags;
    uint8 HSSSelectionFlags;
    uint8 GYRSelectionFlags;
    uint8 STRSelectionFlags;
    uint8 GNSSSelectionFlags;
    uint8 ExtSensorSelectionFlags;
} __attribute__((packed)) ADCS_NodeSelectionConfigTlm_Payload_t;

typedef struct
{ // ID 198
    float  MTQ0MaxDipoleMoment;
    float  MTQ1MaxDipoleMoment;
    float  MTQ2MaxDipoleMoment;
    uint16 MaxMTQOnTime;
    uint16 MinMTQOnTime;
    float  MagneticControlFilterFactor;
} __attribute__((packed)) ADCS_MTQConfigTlm_Payload_t;

typedef struct
{ // ID 199
    uint8 MainEstimatorMode;
    uint8 BackupEstimatorMode;
} __attribute__((packed)) ADCS_EstimationModeTlm_Payload_t;

typedef struct
{ // ID 200
    uint8_t OperationalMode;
} __attribute__((packed)) ADCS_OperationalStateTlm_Payload_t;

typedef struct
{ // ID 203
    uint32 TimeSeconds;
    uint32 TimeNanoSeconds;
    uint8  CSS0;
    uint8  CSS1;
    uint8  CSS2;
    uint8  CSS3;
    uint8  CSS4;
    uint8  CSS5;
    uint8  CSS6;
    uint8  CSS7;
    uint8  CSS8;
    uint8  CSS9;
    uint8   CSSValidFlag:1;
} __attribute__((packed)) ADCS_RawCSSSensorTlm_Payload_t;

typedef struct
{ // ID 204
    uint32 TimeSeconds;
    uint32 TimeNanoSeconds;
    float  GYR0RawRateX;
    float  GYR0RawRateY;
    float  GYR0RawRateZ;
    float  GYR1RawRateX;
    float  GYR1RawRateY;
    float  GYR1RawRateZ;
    uint8   GYR0ValidFlag:1; // GYR0, 1
    uint8   GYR1ValidFlag:1; // GYR1, 0
} __attribute__((packed)) ADCS_RawGYRSensorTlm_Paylaod_t;

typedef struct
{ // ID 207
    uint32 TimeSeconds;
    uint32 TimeNanoSeconds;
    float  GYR0CalibratedRateX;
    float  GYR0CalibratedRateY;
    float  GYR0CalibratedRateZ;
    float  GYR1CalibratedRateX;
    float  GYR1CalibratedRateY;
    float  GYR1CalibratedRateZ;
    float  ExtGYR0CalibratedRateX;
    float  ExtGYR0CalibratedRateY;
    float  ExtGYR0CalibratedRateZ;
    float  ExtGYR1CalibratedRateX;
    float  ExtGYR1CalibratedRateY;
    float  ExtGYR1CalibratedRateZ;
    uint8   GYR0ValidFlag:1; // GYR0, 1
    uint8   GYR1ValidFlag:1; // GYR1, 0
    uint8   EXTGYR0ValidFlag:1; // GYR0, 0
    uint8   EXTGYR1ValidFlag:1; // GYR1, 0
} __attribute__((packed)) ADCS_CalibratedGYRSensorTlm_Payload_t;

typedef struct
{ // ID 210
    uint32 TimeSecond;
    uint32 TimeNanoSecond;
    int16  EstRoll;
    int16  EstPitch;
    int16  EstYaw;
    int16  EstORCQ0;
    int16  EstORCQ1;
    int16  EstORCQ2;
    int16  EstORCQ3;
    int16  EstGYRbiasX;
    int16  EstGYRbiasY;
    int16  EstGYRbiasZ;
    int16  EstORCBodyRateX;
    int16  EstORCBodyRateY;
    int16  EstORCBodyRateZ;
    int16  EstIRCBodyRateX;
    int16  EstIRCBodyRateY;
    int16  EstIRCBodyRateZ;
    float  EstGyroscTrqX;
    float  EstGyroscTrqY;
    float  EstGyroscTrqZ;
    int16  InnvVecX;
    int16  InnvVecY;
    int16  InnvVecZ;
    int16  StdEstRateX;
    int16  StdEstRateY;
    int16  StdEstRateZ;
    int16  StdEstQ0;
    int16  StdEstQ1;
    int16  StdEstQ2;
    uint8  ActiveEstMode;
} __attribute__((packed)) ADCS_MainEstimatorTlm_Payload_t;

typedef struct
{ // ID 211
    uint32 TimeSecond;
    uint32 TimeNanoSecond;
    float  EstORCQ0;
    float  EstORCQ1;
    float  EstORCQ2;
    float  EstORCQ3;
    float  EstORCBodyRateX;
    float  EstORCBodyRateY;
    float  EstORCBodyRateZ;
} __attribute__((packed)) ADCS_MainEstimatorHighResTlm_Payload_t;

typedef struct
{ // ID 219
    uint16 FrameSize;
    uint8  FrameByte[256];

} __attribute__((packed)) ADCS_DataFrameTlm_Payload_t;

typedef struct
{ // ID 220
    uint16 FrameNumber;
    uint8  Checksum;
    uint8  LastFrame : 1;
    uint8  FrameError : 1;

    uint8 Spare : 6; // Explicit declaration
} __attribute__((packed)) ADCS_InfoFrameInMemoryTlm_Payload_t;

typedef struct
{ // ID 221
    uint8 Mag0SensingElement : 1;
    uint8 Mag1SensingElement : 1;

    uint8 Spare : 6; // Explicit declaration
} __attribute__((packed)) ADCS_MagSensingElmConfigTlm_Payload_t;

typedef struct
{ // ID 227
    uint8 FastInclusionBitmask[5];
    uint8 SlowInclusionBitmask[5];
} __attribute__((packed)) ADCS_TlmLogInclMaskTlm_Payload_t;

typedef struct
{ // ID 228
    uint8 UARTTlmReturnInterval : 4;
    uint8 UART2TlmReturnInterval : 4;
    uint8 CANTlmRetrunInterval : 4;
    uint8 Reserved : 4;

    uint8 UARTTlmIDInclusionBitmask[5];
    uint8 UART2TlmIDInclusionBitmask[5];
    uint8 CANTlmIDInclusionBitmask[5];
} __attribute__((packed)) ADCS_UnsolicitTlmMsgSetupTlm_Payload_t;

typedef struct
{ // ID 233
    uint8_t InfoUART : 1;
    uint8_t MinorUART : 1;
    uint8_t MajorUART : 1;
    uint8_t CriticalUART : 1;

    uint8_t InfoUART2 : 1;
    uint8_t MinorUART2 : 1;
    uint8_t MajorUART2 : 1;
    uint8_t CriticalUART2 : 1;

    uint8_t InfoCAN : 1;
    uint8_t MinorCAN : 1;
    uint8_t MajorCAN : 1;
    uint8_t CriticalCAN : 1;

    uint8_t Spare : 4; // Explicit declaration
} __attribute__((packed)) ADCS_UnsolicitEventMsgSetupTlm_Payload_t;

typedef struct
{ // ID 234
    uint8  NumberOfQEntries;
    uint8  NumberOfRQIterations;
    uint32 NumberOfEntries;
    uint32 OldestEntryUnixTime;
    uint32 LatestEntryUnixTime;
    uint32 WriteCounter;
    uint8  ReadQState : 2;

    uint8 Spare : 6; // Explicit declaration
} __attribute__((packed)) ADCS_TlmLogStatusResponseTlm_Payload_t;

typedef struct
{ // ID 235
    uint16_t NumQueuedEntry;
    uint16_t NumBufferedEntry;
    uint32_t NumEntry;
    uint32_t NumEmptyEntry;
    uint32_t OldEntryUnixTime;
    uint32_t LastEntryUnixTime;
    uint32_t NumCriticalEVS;
    uint32_t NumMajorWarningEVS;
    uint32_t NumMinorWarningEVS;
    uint32_t NumInfoEVS;
    uint32_t WriteCnt;
    uint8_t  ReadQueState;
} __attribute__((packed)) ADCS_EventLogStatusResponseTlm_Payload_t;

typedef struct
{ // ID 239
    uint8  NodeType_Sensor1;
    uint8  AbstNodeType_Sensor1;
    uint32 SerialNum_Sensor1;
    uint32 Address_Sensor1;

    uint8  NodeType_Sensor2;
    uint8  AbstNodeType_Sensor2;
    uint32 SerialNum_Sensor2;
    uint32 Address_Sensor2;

    uint8  NodeType_Sensor3;
    uint8  AbstNodeType_Sensor3;
    uint32 SerialNum_Sensor3;
    uint32 Address_Sensor3;

    uint8  NodeType_Sensor4;
    uint8  AbstNodeType_Sensor4;
    uint32 SerialNum_Sensor4;
    uint32 Address_Sensor4;

    uint8  NodeType_Sensor5;
    uint8  AbstNodeType_Sensor5;
    uint32 SerialNum_Sensor5;
    uint32 Address_Sensor5;

    uint8  NodeType_Sensor6;
    uint8  AbstNodeType_Sensor6;
    uint32 SerialNum_Sensor6;
    uint32 Address_Sensor6;

    uint8  NodeType_Sensor7;
    uint8  AbstNodeType_Sensor7;
    uint32 SerialNum_Sensor7;
    uint32 Address_Sensor7;

    uint8  NodeType_Sensor8;
    uint8  AbstNodeType_Sensor8;
    uint32 SerialNum_Sensor8;
    uint32 Address_Sensor8;

    uint8  NodeType_Wheel1;
    uint8  AbstNodeType_Wheel1;
    uint32 SerialNum_Wheel1;
    uint32 Address_Wheel1;

    uint8  NodeType_Wheel2;
    uint8  AbstNodeType_Wheel2;
    uint32 SerialNum_Wheel2;
    uint32 Address_Wheel2;

    uint8  NodeType_Wheel3;
    uint8  AbstNodeType_Wheel3;
    uint32 SerialNum_Wheel3;
    uint32 Address_Wheel3;

    uint8  NodeType_Wheel4;
    uint8  AbstNodeType_Wheel4;
    uint32 SerialNum_Wheel4;
    uint32 Address_Wheel4;

} __attribute__((packed)) ADCS_PortMapTlm_Payload_t;

/*************************************
 * CubeADCS Event Entry
 *************************************/
typedef struct
{
    uint32_t Counter;
    uint32_t UpTime;
    uint32_t UnixTime;
    uint16_t MilliSec;
    struct __attribute__((packed))
    {
        uint16_t EventType : 9;
        uint8_t  EventSource : 5;
        uint8_t  EventClass : 2;
    } Identifier;
    uint8_t EventData[8];
} __attribute__((packed)) ADCS_EventEntry_t;

/*************************************
 * CubeADCS Frame Something
 *************************************/
typedef struct
{
    uint32_t Counter;
    uint32_t UpTime;
    uint32_t UnixTime;
    uint16_t MilliSec;
    struct __attribute__((packed))
    {
        uint16_t EventType : 9;
        uint8_t  EventSource : 5;
        uint8_t  EventClass : 2;
    } Identifier;
    uint8_t EventData[8];
} __attribute__((packed)) ADCS_Frame_t;

#endif