//
// A container for biometric captured data.
// OUT payload for IOCTL_BIOMETRIC_CAPTURE_DATA
//
typedef struct _CRFP_CAPTURE_DATA {
    //Compatible with _WINBIO_CAPTURE_DATA
    DWORD PayloadSize;
    HRESULT WinBioHresult;
    WINBIO_SENSOR_STATUS SensorStatus;
    WINBIO_REJECT_DETAIL RejectDetail;
    DWORD DataSize; //Set to UINT32

    //Start crosfingerprint specific stuff (in place of data)
    UINT32 FPMKBPValue;
} CRFP_CAPTURE_DATA, * PCRFP_CAPTURE_DATA;