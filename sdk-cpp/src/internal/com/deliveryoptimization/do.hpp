#ifndef _do_hpp_
#define _do_hpp_
extern "C" {

// Prerequisites. These may already be included via a precompiled header, possibly via other headers such as windows.h or ole.h.
//#include "rpc.h"
//#include "rpcndr.h"
//#include "oaidl.h"

// DO reference: https://docs.microsoft.com/en-us/windows/win32/delivery_optimization/do-reference

// https://docs.microsoft.com/en-us/windows/win32/delivery_optimization/deliveryoptimizationdownloadtypes/ns-deliveryoptimizationdownloadtypes-do_download_range
typedef struct _DO_DOWNLOAD_RANGE
{
    UINT64 Offset;
    UINT64 Length;
} DO_DOWNLOAD_RANGE;

// https://docs.microsoft.com/en-us/windows/win32/delivery_optimization/do/ns-do-do_download_range_info
typedef struct _DO_DOWNLOAD_RANGES_INFO
{
    UINT RangeCount;
    DO_DOWNLOAD_RANGE Ranges[1];
} DO_DOWNLOAD_RANGES_INFO;

// https://docs.microsoft.com/en-us/windows/win32/delivery_optimization/deliveryoptimizationdownloadtypes/ne-deliveryoptimizationdownloadtypes-dodownloadstate
typedef enum _DODownloadState
{
    DODownloadState_Created = 0,
    DODownloadState_Transferring,
    DODownloadState_Transferred,
    DODownloadState_Finalized,
    DODownloadState_Aborted,
    DODownloadState_Paused
} DODownloadState;

// https://docs.microsoft.com/en-us/windows/win32/delivery_optimization/do/ns-do-do_download_status
typedef struct _DO_DOWNLOAD_STATUS
{
    UINT64 BytesTotal;
    UINT64 BytesTransferred;
    DODownloadState State;
    HRESULT Error;
    HRESULT ExtendedError;
} DO_DOWNLOAD_STATUS;

// https://docs.microsoft.com/en-us/windows/win32/delivery_optimization/deliveryoptimizationdownloadtypes/ne-deliveryoptimizationdownloadtypes-dodownloadcostpolicy
typedef enum _DODownloadCostPolicy
{
    DODownloadCostPolicy_Always = 0,
    DODownloadCostPolicy_Unrestricted,
    DODownloadCostPolicy_Standard,
    DODownloadCostPolicy_NoRoaming,
    DODownloadCostPolicy_NoSurcharge,
    DODownloadCostPolicy_NoCellular
} DODownloadCostPolicy;

// https://docs.microsoft.com/en-us/windows/win32/delivery_optimization/deliveryoptimizationdownloadtypes/ne-deliveryoptimizationdownloadtypes-dodownloadproperty
typedef enum _DODownloadProperty
{
    DODownloadProperty_Id = 0,
    DODownloadProperty_Uri,
    DODownloadProperty_ContentId,
    DODownloadProperty_DisplayName,
    DODownloadProperty_LocalPath,
    DODownloadProperty_HttpCustomHeaders,
    DODownloadProperty_CostPolicy,
    DODownloadProperty_SecurityFlags,
    DODownloadProperty_CallbackFreqPercent,
    DODownloadProperty_CallbackFreqSeconds,
    DODownloadProperty_NoProgressTimeoutSeconds,
    DODownloadProperty_ForegroundPriority,
    DODownloadProperty_BlockingMode,
    DODownloadProperty_CallbackInterface,
    DODownloadProperty_StreamInterface,
    DODownloadProperty_SecurityContext,
    DODownloadProperty_NetworkToken,
    DODownloadProperty_CorrelationVector,
    DODownloadProperty_DecryptionInfo,
    DODownloadProperty_IntegrityCheckInfo,
    DODownloadProperty_IntegrityCheckMandatory,
    DODownloadProperty_TotalSizeBytes,
    DODownloadProperty_DisallowOnCellular,                  // TODO: Document this (Win11)
    DODownloadProperty_HttpCustomAuthHeaders,               // TODO: Document this (Win11)
    DODownloadProperty_HttpAllowSecureToNonSecureRedirect,  // TODO: Document this (Nickel)
    DODownloadProperty_NonVolatile                          // TODO: Document this (Nickel)
} DODownloadProperty;

// https://docs.microsoft.com/en-us/windows/win32/delivery_optimization/do/ns-do-do_download_enum_category
typedef struct _DO_DOWNLOAD_ENUM_CATEGORY
{
    DODownloadProperty Property;
    LPCWSTR Value;
} DO_DOWNLOAD_ENUM_CATEGORY;

// https://docs.microsoft.com/en-us/windows/win32/delivery_optimization/do/nn-do-idodownload
// TODO: Document the method order and IID
interface DECLSPEC_UUID("FBBD7FC0-C147-4727-A38D-827EF071EE77") DECLSPEC_NOVTABLE
IDODownload : public IUnknown
{
public:
    IFACEMETHOD(Start)(const DO_DOWNLOAD_RANGES_INFO *ranges) = 0; // TODO: Doc nit: add 'const'
    IFACEMETHOD(Pause)() = 0;
    IFACEMETHOD(Abort)() = 0;
    IFACEMETHOD(Finalize)() = 0;
    IFACEMETHOD(GetStatus)(DO_DOWNLOAD_STATUS* status) = 0;
    IFACEMETHOD(GetProperty)(DODownloadProperty propId, VARIANT* propVal) = 0;
    IFACEMETHOD(SetProperty)(DODownloadProperty propId, const VARIANT* propVal) = 0; // TODO: Doc nit: add 'const'
};
DEFINE_GUID(IID_IDODownload, 0xFBBD7FC0, 0xC147, 0x4727, 0xA3, 0x8D, 0x82, 0x7E, 0xF0, 0x71, 0xEE, 0x77);

// https://docs.microsoft.com/en-us/windows/win32/delivery_optimization/do/nn-do-idodownloadstatuscallback
// TODO: Document the IID
interface DECLSPEC_UUID("D166E8E3-A90E-4392-8E87-05E996D3747D") DECLSPEC_NOVTABLE
IDODownloadStatusCallback : public IUnknown
{
public:
    IFACEMETHOD(OnStatusChange)(IDODownload* download, const DO_DOWNLOAD_STATUS* status) = 0;
};
DEFINE_GUID(IID_IDODownloadStatusCallback, 0xD166E8E3, 0xA90E, 0x4392, 0x8E, 0x87, 0x05, 0xE9, 0x96, 0xD3, 0x74, 0x7D);

// https://docs.microsoft.com/en-us/windows/win32/delivery_optimization/do/nn-do-idomanager
// TODO: Document the method order and IID. Call CoCreateInstance with CLSID_DeliveryOptimization and CLSCTX_LOCAL_SERVER to obtain IDOManager.
interface DECLSPEC_UUID("400E2D4A-1431-4C1A-A748-39CA472CFDB1") DECLSPEC_NOVTABLE
IDOManager : public IUnknown
{
public:
    IFACEMETHOD(CreateDownload)(IDODownload** download) = 0;
    IFACEMETHOD(EnumDownloads)(const DO_DOWNLOAD_ENUM_CATEGORY* category, IEnumUnknown** ppEnum) = 0; // TODO: Doc nit: add 'const'
};
DEFINE_GUID(IID_IDOManager, 0x400E2D4A, 0x1431, 0x4C1A, 0xA7, 0x48, 0x39, 0xCA, 0x47, 0x2C, 0xFD, 0xB1);

// TODO: Document this CLSID
DEFINE_GUID(CLSID_DeliveryOptimization, 0x5b99fa76, 0x721c, 0x423c, 0xad, 0xac, 0x56, 0xd0, 0x3c, 0x8a, 0x80, 0x07);
class DECLSPEC_UUID("5b99fa76-721c-423c-adac-56d03c8a8007") DeliveryOptimization;

} // extern "C"
#endif // _do_hpp_
