#include <winbio_ioctl.h>

//
// A container for biometric captured data.
// OUT payload for IOCTL_BIOMETRIC_CAPTURE_DATA
//
typedef struct _CRFP_CAPTURE_DATA {
    DWORD PayloadSize;
    HRESULT WinBioHresult;
    UINT32 FPMKBPValue;
} CRFP_CAPTURE_DATA, * PCRFP_CAPTURE_DATA;