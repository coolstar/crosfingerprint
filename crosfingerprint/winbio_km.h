#ifndef FILE_DEVICE_BIOMETRIC
#define FILE_DEVICE_BIOMETRIC   0x00000044
#else
#if 0x00000044 != FILE_DEVICE_BIOMETRIC
#error "Incorrect Biometric Device Definition"
#endif
#endif

//
// {E2B5183A-99EA-4cc3-AD6B-80CA8D715B80}
//
DEFINE_GUID(GUID_DEVINTERFACE_BIOMETRIC_READER,
    0xe2b5183a, 0x99ea, 0x4cc3, 0xad, 0x6b, 0x80, 0xca, 0x8d, 0x71, 0x5b, 0x80);


//
///////////////////////////////////////////////////////////////////////////////
//
//  Biometric Device IOCTLs
//

#define BIO_CTL_CODE(code)                      CTL_CODE(FILE_DEVICE_BIOMETRIC, \
                                                (code), \
                                                METHOD_BUFFERED, \
                                                FILE_ANY_ACCESS)

//
// Mandatory IOCTLs
//
#define IOCTL_BIOMETRIC_GET_ATTRIBUTES                  BIO_CTL_CODE(0x001)
#define IOCTL_BIOMETRIC_RESET                           BIO_CTL_CODE(0x002)
#define IOCTL_BIOMETRIC_CALIBRATE                       BIO_CTL_CODE(0x003)
#define IOCTL_BIOMETRIC_GET_SENSOR_STATUS               BIO_CTL_CODE(0x004)
#define IOCTL_BIOMETRIC_CAPTURE_DATA                    BIO_CTL_CODE(0x005)

//
// Optional IOCTL for updating the firmware
//
#define IOCTL_BIOMETRIC_UPDATE_FIRMWARE                 BIO_CTL_CODE(0x006)

//
// Optional IOCTL for retrieving supported hash algorthims
// Capability:  WINBIO_CAPABILITY_PROCESSING, WINBIO_CAPABILITY_ENCRYPTION
//
#define IOCTL_BIOMETRIC_GET_SUPPORTED_ALGORITHMS        BIO_CTL_CODE(0x007)

//
// Optional IOCTLs to set indicator status
// Capability:  WINBIO_CAPABILITY_INDICATOR
//
#define IOCTL_BIOMETRIC_GET_INDICATOR                   BIO_CTL_CODE(0x008)
#define IOCTL_BIOMETRIC_SET_INDICATOR                   BIO_CTL_CODE(0x009)

#if (NTDDI_VERSION >= NTDDI_WIN10_RS3)

//
// Optional IOCTL to retrieve sensor-type information
// in a vendor-specific format
//
#define IOCTL_BIOMETRIC_GET_PRIVATE_SENSOR_TYPE         BIO_CTL_CODE(0x00A)

#endif // (NTDDI_VERSION >= NTDDI_WIN10_RS3)

#if (NTDDI_VERSION >= NTDDI_WIN10_RS4)

//
// Optional IOCTLs for support of the Secure Connection Protocol (SCP) V1
// Capability: WINBIO_CAPABILITY_SCP_V1
//
#define IOCTL_BIOMETRIC_CONNECT_SECURE                  BIO_CTL_CODE(0x00B)
#define IOCTL_BIOMETRIC_CAPTURE_ENCRYPTED_DATA          BIO_CTL_CODE(0x00C)

//
// Optional IOCTL for wake-on-touch support.
// Capability: WINBIO_CAPABILITY_WAKE
// The WBDI driver must hold this IOCTL in non-power-managed IO queue.
// The Windows Biometric Framework will send this IOCTL on transition to a low
// power state. When the sensor wishes to wake the host, it should complete this
// IOCTL.
//
#define IOCTL_BIOMETRIC_NOTIFY_WAKE                    BIO_CTL_CODE(0x00D)

#endif // (NTDDI_VERSION >= NTDDI_WIN10_RS4)

//
// Vendor control codes are specified at 0x800:
//
//
#define IOCTL_BIOMETRIC_VENDOR                          CTL_CODE(FILE_DEVICE_BIOMETRIC, 0x800, 0, 0)


//
///////////////////////////////////////////////////////////////////////////////
//
// IOCTL payload and type definitions
//

//
// WINBIO version
//

#define WINBIO_WBDI_MAJOR_VERSION 0x1
#define WINBIO_WBDI_MINOR_VERSION 0x0

//
// Values representing the operating status of a sensor
//
typedef ULONG WINBIO_SENSOR_STATUS, * PWINBIO_SENSOR_STATUS;

#define WINBIO_SENSOR_STATUS_UNKNOWN        ((WINBIO_SENSOR_STATUS)0)
#define WINBIO_SENSOR_ACCEPT                ((WINBIO_SENSOR_STATUS)1)
#define WINBIO_SENSOR_REJECT                ((WINBIO_SENSOR_STATUS)2)
#define WINBIO_SENSOR_READY                 ((WINBIO_SENSOR_STATUS)3)
#define WINBIO_SENSOR_BUSY                  ((WINBIO_SENSOR_STATUS)4)
#define WINBIO_SENSOR_NOT_CALIBRATED        ((WINBIO_SENSOR_STATUS)5)
#define WINBIO_SENSOR_FAILURE               ((WINBIO_SENSOR_STATUS)6)

//
// A GUID
//
typedef GUID WINBIO_UUID, * PWINBIO_UUID;

//
// Represents a NULL-terminated Unicode character
// string inside a fixed-length buffer.
//
#define WINBIO_MAX_STRING_LEN 256
typedef WCHAR WINBIO_STRING[WINBIO_MAX_STRING_LEN];
typedef WINBIO_STRING* PWINBIO_STRING;

//
// Version
//
typedef struct _WINBIO_VERSION {
    DWORD MajorVersion;
    DWORD MinorVersion;
} WINBIO_VERSION, * PWINBIO_VERSION;


///////////////////////////////////////////////////////////////////////////////
//
// Bitmask describing the supported set of biometric types (factors).
//
typedef ULONG32 WINBIO_BIOMETRIC_TYPE, * PWINBIO_BIOMETRIC_TYPE;

#ifdef MIDL_PASS

const WINBIO_BIOMETRIC_TYPE WINBIO_STANDARD_TYPE_MASK = (WINBIO_BIOMETRIC_TYPE)0x00FFFFFF;

const WINBIO_BIOMETRIC_TYPE WINBIO_NO_TYPE_AVAILABLE = (WINBIO_BIOMETRIC_TYPE)0x00000000;

//
// Standard biometric types (from NISTIR 6529-A)
//
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_MULTIPLE = (WINBIO_BIOMETRIC_TYPE)0x00000001;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_FACIAL_FEATURES = (WINBIO_BIOMETRIC_TYPE)0x00000002;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_VOICE = (WINBIO_BIOMETRIC_TYPE)0x00000004;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_FINGERPRINT = (WINBIO_BIOMETRIC_TYPE)0x00000008;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_IRIS = (WINBIO_BIOMETRIC_TYPE)0x00000010;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_RETINA = (WINBIO_BIOMETRIC_TYPE)0x00000020;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_HAND_GEOMETRY = (WINBIO_BIOMETRIC_TYPE)0x00000040;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_SIGNATURE_DYNAMICS = (WINBIO_BIOMETRIC_TYPE)0x00000080;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_KEYSTROKE_DYNAMICS = (WINBIO_BIOMETRIC_TYPE)0x00000100;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_LIP_MOVEMENT = (WINBIO_BIOMETRIC_TYPE)0x00000200;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_THERMAL_FACE_IMAGE = (WINBIO_BIOMETRIC_TYPE)0x00000400;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_THERMAL_HAND_IMAGE = (WINBIO_BIOMETRIC_TYPE)0x00000800;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_GAIT = (WINBIO_BIOMETRIC_TYPE)0x00001000;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_SCENT = (WINBIO_BIOMETRIC_TYPE)0x00002000;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_DNA = (WINBIO_BIOMETRIC_TYPE)0x00004000;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_EAR_SHAPE = (WINBIO_BIOMETRIC_TYPE)0x00008000;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_FINGER_GEOMETRY = (WINBIO_BIOMETRIC_TYPE)0x00010000;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_PALM_PRINT = (WINBIO_BIOMETRIC_TYPE)0x00020000;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_VEIN_PATTERN = (WINBIO_BIOMETRIC_TYPE)0x00040000;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_FOOT_PRINT = (WINBIO_BIOMETRIC_TYPE)0x00080000;
//
// WinBio extended types
//
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_OTHER = (WINBIO_BIOMETRIC_TYPE)0x40000000;
const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_PASSWORD = (WINBIO_BIOMETRIC_TYPE)0x80000000;

const WINBIO_BIOMETRIC_TYPE WINBIO_TYPE_ANY = (WINBIO_BIOMETRIC_TYPE)(WINBIO_STANDARD_TYPE_MASK | WINBIO_TYPE_OTHER | WINBIO_TYPE_PASSWORD);

#else   // MIDL_PASS

#define WINBIO_STANDARD_TYPE_MASK           ((WINBIO_BIOMETRIC_TYPE)0x00FFFFFF)

#define WINBIO_NO_TYPE_AVAILABLE            ((WINBIO_BIOMETRIC_TYPE)0x00000000)
//
// Standard biometric types (from NISTIR 6529-A)
//
#define WINBIO_TYPE_MULTIPLE                ((WINBIO_BIOMETRIC_TYPE)0x00000001)
#define WINBIO_TYPE_FACIAL_FEATURES         ((WINBIO_BIOMETRIC_TYPE)0x00000002)
#define WINBIO_TYPE_VOICE                   ((WINBIO_BIOMETRIC_TYPE)0x00000004)
#define WINBIO_TYPE_FINGERPRINT             ((WINBIO_BIOMETRIC_TYPE)0x00000008)
#define WINBIO_TYPE_IRIS                    ((WINBIO_BIOMETRIC_TYPE)0x00000010)
#define WINBIO_TYPE_RETINA                  ((WINBIO_BIOMETRIC_TYPE)0x00000020)
#define WINBIO_TYPE_HAND_GEOMETRY           ((WINBIO_BIOMETRIC_TYPE)0x00000040)
#define WINBIO_TYPE_SIGNATURE_DYNAMICS      ((WINBIO_BIOMETRIC_TYPE)0x00000080)
#define WINBIO_TYPE_KEYSTROKE_DYNAMICS      ((WINBIO_BIOMETRIC_TYPE)0x00000100)
#define WINBIO_TYPE_LIP_MOVEMENT            ((WINBIO_BIOMETRIC_TYPE)0x00000200)
#define WINBIO_TYPE_THERMAL_FACE_IMAGE      ((WINBIO_BIOMETRIC_TYPE)0x00000400)
#define WINBIO_TYPE_THERMAL_HAND_IMAGE      ((WINBIO_BIOMETRIC_TYPE)0x00000800)
#define WINBIO_TYPE_GAIT                    ((WINBIO_BIOMETRIC_TYPE)0x00001000)
#define WINBIO_TYPE_SCENT                   ((WINBIO_BIOMETRIC_TYPE)0x00002000)
#define WINBIO_TYPE_DNA                     ((WINBIO_BIOMETRIC_TYPE)0x00004000)
#define WINBIO_TYPE_EAR_SHAPE               ((WINBIO_BIOMETRIC_TYPE)0x00008000)
#define WINBIO_TYPE_FINGER_GEOMETRY         ((WINBIO_BIOMETRIC_TYPE)0x00010000)
#define WINBIO_TYPE_PALM_PRINT              ((WINBIO_BIOMETRIC_TYPE)0x00020000)
#define WINBIO_TYPE_VEIN_PATTERN            ((WINBIO_BIOMETRIC_TYPE)0x00040000)
#define WINBIO_TYPE_FOOT_PRINT              ((WINBIO_BIOMETRIC_TYPE)0x00080000)
//
// WinBio extended types
//
#define WINBIO_TYPE_OTHER                   ((WINBIO_BIOMETRIC_TYPE)0x40000000)
#define WINBIO_TYPE_PASSWORD                ((WINBIO_BIOMETRIC_TYPE)0x80000000)

#define WINBIO_TYPE_ANY                     ((WINBIO_BIOMETRIC_TYPE)(WINBIO_STANDARD_TYPE_MASK |    \
                                                                     WINBIO_TYPE_OTHER |            \
                                                                     WINBIO_TYPE_PASSWORD))

#endif  // MIDL_PASS

//
// WinBio sensor sub-types.  These are defined per Biometric type, and are
// defined only for fingerprints in this version.
//
typedef ULONG WINBIO_BIOMETRIC_SENSOR_SUBTYPE, * PWINBIO_BIOMETRIC_SENSOR_SUBTYPE;

#define WINBIO_SENSOR_SUBTYPE_UNKNOWN       ((WINBIO_BIOMETRIC_SENSOR_SUBTYPE)0x00000000)

#define WINBIO_FP_SENSOR_SUBTYPE_SWIPE      ((WINBIO_BIOMETRIC_SENSOR_SUBTYPE)0x00000001)
#define WINBIO_FP_SENSOR_SUBTYPE_TOUCH      ((WINBIO_BIOMETRIC_SENSOR_SUBTYPE)0x00000002)

///////////////////////////////////////////////////////////////////////////////
//
// Sensor operating modes
//
typedef ULONG WINBIO_SENSOR_MODE, * PWINBIO_SENSOR_MODE;

#define WINBIO_SENSOR_UNKNOWN_MODE      ((WINBIO_SENSOR_MODE)0)
#define WINBIO_SENSOR_BASIC_MODE        ((WINBIO_SENSOR_MODE)1)
#define WINBIO_SENSOR_ADVANCED_MODE     ((WINBIO_SENSOR_MODE)2)
#define WINBIO_SENSOR_NAVIGATION_MODE   ((WINBIO_SENSOR_MODE)3)
#define WINBIO_SENSOR_SLEEP_MODE        ((WINBIO_SENSOR_MODE)4)

///////////////////////////////////////////////////////////////////////////////
//
// Factor-specific value giving additional information about
// a biometric measurement (e.g., *which* finger a fingerprint
// sample was taken from).
//
typedef UCHAR WINBIO_BIOMETRIC_SUBTYPE, * PWINBIO_BIOMETRIC_SUBTYPE;

#define WINBIO_SUBTYPE_NO_INFORMATION       ((WINBIO_BIOMETRIC_SUBTYPE)0x00)
#define WINBIO_SUBTYPE_ANY                  ((WINBIO_BIOMETRIC_SUBTYPE)0xFF)

//
// Bitmask of sensor capabilities
//
typedef ULONG WINBIO_CAPABILITIES, * PWINBIO_CAPABILITIES;

#define WINBIO_CAPABILITY_SENSOR            ((WINBIO_CAPABILITIES)0x00000001)
#define WINBIO_CAPABILITY_MATCHING          ((WINBIO_CAPABILITIES)0x00000002)
#define WINBIO_CAPABILITY_DATABASE          ((WINBIO_CAPABILITIES)0x00000004)
#define WINBIO_CAPABILITY_PROCESSING        ((WINBIO_CAPABILITIES)0x00000008)
#define WINBIO_CAPABILITY_ENCRYPTION        ((WINBIO_CAPABILITIES)0x00000010)
#define WINBIO_CAPABILITY_NAVIGATION        ((WINBIO_CAPABILITIES)0x00000020)
#define WINBIO_CAPABILITY_INDICATOR         ((WINBIO_CAPABILITIES)0x00000040)

#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

#define WINBIO_CAPABILITY_VIRTUAL_SENSOR    ((WINBIO_CAPABILITIES)0x00000080)

#endif // (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)

#define WINBIO_CAPABILITY_SECURE_SENSOR     ((WINBIO_CAPABILITIES)0x00000100)

#endif // (NTDDI_VERSION >= NTDDI_WIN10_RS1)

///////////////////////////////////////////////////////////////////////////////
//
// BIR 'BiometricDataFormat' and 'ProductId' fields:
//
// Identifies a registered data format as a pair consisting of
// an IBIA-assigned owner value plus an owner-assigned format-type
// value.
//
typedef struct _WINBIO_REGISTERED_FORMAT {
    USHORT Owner;
    USHORT Type;
} WINBIO_REGISTERED_FORMAT, * PWINBIO_REGISTERED_FORMAT;

#define WINBIO_NO_FORMAT_OWNER_AVAILABLE    ((USHORT)0)
#define WINBIO_NO_FORMAT_TYPE_AVAILABLE     ((USHORT)0)

//
// OUT payload for IOCTL_BIOMETRIC_GET_ATTRIBUTES
//
typedef struct _WINBIO_SENSOR_ATTRIBUTES {
    DWORD PayloadSize;
    HRESULT WinBioHresult;
    WINBIO_VERSION WinBioVersion;
    WINBIO_BIOMETRIC_TYPE SensorType;
    WINBIO_BIOMETRIC_SENSOR_SUBTYPE SensorSubType;
    WINBIO_CAPABILITIES Capabilities;
    WINBIO_STRING ManufacturerName;
    WINBIO_STRING ModelName;
    WINBIO_STRING SerialNumber;
    WINBIO_VERSION FirmwareVersion;
    DWORD SupportedFormatEntries;
    WINBIO_REGISTERED_FORMAT SupportedFormat[1]; // Defined as a length of 1 to avoid compiler warning.
} WINBIO_SENSOR_ATTRIBUTES, * PWINBIO_SENSOR_ATTRIBUTES;

//
// The WINBIO_DATA structure associates a length, in
// bytes, with an arbitrary block of contiguous memory.
//
typedef struct _WINBIO_DATA {
    DWORD Size;
    BYTE Data[1]; // Defined as a length of 1 to avoid compiler warning.
} WINBIO_DATA, * PWINBIO_DATA;

//
// OUT payload for IOCTL_BIOMETRIC_CALIBRATE
//
typedef struct _WINBIO_CALIBRATION_INFO {
    DWORD PayloadSize;
    HRESULT WinBioHresult;
    WINBIO_DATA CalibrationData;
} WINBIO_CALIBRATION_INFO, * PWINBIO_CALIBRATION_INFO;


//
// OUT payload for IOCTL_BIOMETRIC_GET_SENSOR_STATUS
//
typedef struct _WINBIO_DIAGNOSTICS {
    DWORD PayloadSize;
    HRESULT WinBioHresult;
    WINBIO_SENSOR_STATUS SensorStatus;
    WINBIO_DATA VendorDiagnostics;
} WINBIO_DIAGNOSTICS, * PWINBIO_DIAGNOSTICS;


///////////////////////////////////////////////////////////////////////////////
//
// BIR 'Purpose' field:
//
// A value defining the purpose for which the BIR
//
//  - is intended, when used as input to a WinBio function
//
//  - is suitable, when used as output from a WinBio function
//  or within a BIR header.
//
// NOTE:
//  In a WINBIO BIR, the 'Purpose' field is defined as a set of flag bits
//  rather than an enumerated type (as specified in NISTIR 6529-A). Transferring
//  a WINBIO BIR to another environment (e.g., BioAPI) will require conversion.
//
//  The suggested way to handle this conversion is to generate a set of nested
//  BIRs for any WINBIO BIRs that have multiple 'Purpose' bits set.
//
typedef UCHAR WINBIO_BIR_PURPOSE, * PWINBIO_BIR_PURPOSE;

#define WINBIO_NO_PURPOSE_AVAILABLE                     ((WINBIO_BIR_PURPOSE)0x00)
#define WINBIO_PURPOSE_VERIFY                           ((WINBIO_BIR_PURPOSE)0x01)
#define WINBIO_PURPOSE_IDENTIFY                         ((WINBIO_BIR_PURPOSE)0x02)
#define WINBIO_PURPOSE_ENROLL                           ((WINBIO_BIR_PURPOSE)0x04)
#define WINBIO_PURPOSE_ENROLL_FOR_VERIFICATION          ((WINBIO_BIR_PURPOSE)0x08)
#define WINBIO_PURPOSE_ENROLL_FOR_IDENTIFICATION        ((WINBIO_BIR_PURPOSE)0x10)
#define WINBIO_PURPOSE_AUDIT                            ((WINBIO_BIR_PURPOSE)0x80)

///////////////////////////////////////////////////////////////////////////////
//
// ANSI INCITS 381-2004 -- Finger Image-Based Data Interchange Format
//
///////////////////////////////////////////////////////////////////////////////
//
// SECTION 7 -- Registered Format
//
// WINBIO_BIR_HEADER.BiometricDataFormat.Owner = WINBIO_ANSI_381_FORMAT_OWNER
// WINBIO_BIR_HEADER.BiometricDataFormat.Type  = WINBIO_ANSI_381_FORMAT_TYPE
//
#define WINBIO_ANSI_381_FORMAT_OWNER    ((USHORT)0x001B)    // INCITS Technical Committee M1
#define WINBIO_ANSI_381_FORMAT_TYPE     ((USHORT)0x0401)    // ANSI-381

///////////////////////////////////////////////////////////////////////////////
//
// BIR 'DataFlags' field:
//      * Security and integrity-checking options
//          PRIVACY     - BDB is encrypted
//          INTEGRITY   - BDB is signed or MAC'ed
//          SIGNED      - 1 -> BDB is signed; 0 -> BDB is MAC'ed
//      * Processing level of the data
//
typedef UCHAR WINBIO_BIR_DATA_FLAGS, * PWINBIO_BIR_DATA_FLAGS;

#define WINBIO_DATA_FLAG_PRIVACY                ((UCHAR)0x02)
#define WINBIO_DATA_FLAG_INTEGRITY              ((UCHAR)0x01)
#define WINBIO_DATA_FLAG_SIGNED                 ((UCHAR)0x04)

#define WINBIO_DATA_FLAG_RAW                    ((UCHAR)0x20)
#define WINBIO_DATA_FLAG_INTERMEDIATE           ((UCHAR)0x40)
#define WINBIO_DATA_FLAG_PROCESSED              ((UCHAR)0x80)

#define WINBIO_DATA_FLAG_OPTION_MASK_PRESENT    ((UCHAR)0x08)   // Always '1'.

//
// Parameters for a capture operation.
// IN payload for IOCTL_BIOMETRIC_CAPTURE_DATA
//

typedef struct _WINBIO_CAPTURE_PARAMETERS {
    DWORD PayloadSize;
    WINBIO_BIR_PURPOSE Purpose;
    WINBIO_REGISTERED_FORMAT Format;
    WINBIO_UUID VendorFormat;
    WINBIO_BIR_DATA_FLAGS Flags;
} WINBIO_CAPTURE_PARAMETERS, * PWINBIO_CAPTURE_PARAMETERS;

#include <winbio_err.h>