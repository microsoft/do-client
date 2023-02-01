// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "download_impl.h"

#include <array>

#include <wrl.h>
#include <wrl/client.h>
#include <wrl/implements.h>

#include "do_download_property.h"
#include "do_download_property_internal.h"
#include "do_errors.h"
#include "do_error_helpers.h"

namespace msdo = microsoft::deliveryoptimization;
using namespace Microsoft::WRL;

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

static msdo::download_state ConvertFromComState(DODownloadState platformState)
{
    static const msdo::download_state c_stateMap[] =
    {
        msdo::download_state::created,      // DODownloadState_Created
        msdo::download_state::transferring, // DODownloadState_Transferring
        msdo::download_state::transferred,  // DODownloadState_Transferred
        msdo::download_state::finalized,    // DODownloadState_Finalized
        msdo::download_state::aborted,      // DODownloadState_Aborted
        msdo::download_state::paused,       // DODownloadState_Paused
    };
    auto index = static_cast<size_t>(platformState);
    return (index < ARRAYSIZE(c_stateMap)) ? c_stateMap[index] : msdo::download_state::paused;
}

static msdo::download_status ConvertFromComStatus(const DO_DOWNLOAD_STATUS& platformStatus)
{
    return msdo::download_status(platformStatus.BytesTotal, platformStatus.BytesTransferred, platformStatus.Error,
        platformStatus.ExtendedError, ConvertFromComState(platformStatus.State));
}

static std::error_code ConvertToComProperty(msdo::download_property prop, DODownloadProperty& comProperty)
{
    static const DODownloadProperty c_propMap[] =
    {
        DODownloadProperty_Id,                                  // id
        DODownloadProperty_Uri,                                 // uri
        DODownloadProperty_ContentId,                           // catalog_id
        DODownloadProperty_DisplayName,                         // caller_name
        DODownloadProperty_LocalPath,                           // download_file_path
        DODownloadProperty_HttpCustomHeaders,                   // http_custom_headers
        DODownloadProperty_CostPolicy,                          // cost_policy
        DODownloadProperty_SecurityFlags,                       // security_flags
        DODownloadProperty_CallbackFreqPercent,                 // callback_freq_percent
        DODownloadProperty_CallbackFreqSeconds,                 // callback_freq_seconds
        DODownloadProperty_NoProgressTimeoutSeconds,            // no_progress_timeout_seconds
        DODownloadProperty_ForegroundPriority,                  // use_foreground_priority
        DODownloadProperty_BlockingMode,                        // blocking_mode
        DODownloadProperty_NetworkToken,                        // network_token
        DODownloadProperty_CorrelationVector,                   // correlation_vector
        DODownloadProperty_DecryptionInfo,                      // decryption_info
        DODownloadProperty_IntegrityCheckInfo,                  // integrity_check_info
        DODownloadProperty_IntegrityCheckMandatory,             // integrity_check_mandatory
        DODownloadProperty_TotalSizeBytes,                      // total_size_bytes
        DODownloadProperty_DisallowOnCellular,                  // disallow_on_cellular
        DODownloadProperty_HttpCustomAuthHeaders,               // http_custom_auth_headers
        DODownloadProperty_HttpAllowSecureToNonSecureRedirect,  // allow_http_to_https_redirect
        DODownloadProperty_NonVolatile,                         // non_volatile
    };
    auto index = static_cast<size_t>(prop);
    if (index >= ARRAYSIZE(c_propMap))
    {
        return make_error_code(errc::invalid_arg);
    }
    comProperty = c_propMap[index];
    return DO_OK;
}

class DOStatusCallback :
    public RuntimeClass<RuntimeClassFlags<RuntimeClassType::ClassicCom>, IDODownloadStatusCallback>
{
public:
    HRESULT RuntimeClassInitialize(const msdo::status_callback_t& callback, msdo::download& download)
    {
        _download = &download;
        _callback = callback;
        return S_OK;
    }

    IFACEMETHODIMP OnStatusChange(IDODownload*, const DO_DOWNLOAD_STATUS* comStatus) noexcept override
    {
        msdo::download_status status = ConvertFromComStatus(*comStatus);
        _callback(*_download, status);
        return S_OK;
    }

private:
    msdo::status_callback_t _callback;
    msdo::download* _download;
};

std::error_code CDownloadImpl::Init(const std::string& uri, const std::string& downloadFilePath) noexcept
{
    Microsoft::WRL::ComPtr<IDOManager> manager;
    RETURN_IF_FAILED(CoCreateInstance(__uuidof(DeliveryOptimization), nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&manager)));
    Microsoft::WRL::ComPtr<IDODownload> spDownload;
    RETURN_IF_FAILED(manager->CreateDownload(&spDownload));

    RETURN_IF_FAILED(CoSetProxyBlanket(static_cast<IUnknown*>(spDownload.Get()), RPC_C_AUTHN_DEFAULT,
        RPC_C_AUTHZ_NONE, COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr, EOAC_STATIC_CLOAKING));

    download_property_value propUri;
    download_property_value propDownloadFilePath;
    DO_RETURN_IF_FAILED(download_property_value::make(uri, propUri));
    DO_RETURN_IF_FAILED(download_property_value::make(downloadFilePath, propDownloadFilePath));

    DO_RETURN_IF_FAILED(SetProperty(download_property::uri, propUri));
    DO_RETURN_IF_FAILED(SetProperty(download_property::download_file_path, propDownloadFilePath));

    _spDownload = std::move(spDownload);
    return DO_OK;
}

// Support only full file downloads for now
std::error_code CDownloadImpl::Start() noexcept
{
    _DO_DOWNLOAD_RANGES_INFO emptyRanges = {};
    emptyRanges.RangeCount = 0;
    return make_error_code(_spDownload->Start(&emptyRanges));
}

std::error_code CDownloadImpl::Pause() noexcept
{
    return make_error_code(_spDownload->Pause());
}

std::error_code CDownloadImpl::Resume() noexcept
{
    return make_error_code(_spDownload->Start(nullptr));
}

std::error_code CDownloadImpl::Finalize() noexcept
{
    return make_error_code(_spDownload->Finalize());
}

std::error_code CDownloadImpl::Abort() noexcept
{
    return make_error_code(_spDownload->Abort());
}

std::error_code CDownloadImpl::GetStatus(msdo::download_status& status) noexcept
{
    DO_DOWNLOAD_STATUS comStatus;
    RETURN_IF_FAILED(_spDownload->GetStatus(&comStatus));
    status = ConvertFromComStatus(comStatus);
    return DO_OK;
}

std::error_code CDownloadImpl::SetStatusCallback(const msdo::status_callback_t& callback, msdo::download& download) noexcept
{
    Microsoft::WRL::ComPtr<DOStatusCallback> spCallback;
    RETURN_IF_FAILED(MakeAndInitialize<DOStatusCallback>(&spCallback, callback, download));

    unique_variant vtCallback;
    V_VT(&vtCallback) = VT_UNKNOWN;
    V_UNKNOWN(&vtCallback) = spCallback.Detach();
    RETURN_IF_FAILED(_spDownload->SetProperty(DODownloadProperty_CallbackInterface, &vtCallback));
    return DO_OK;
}

std::error_code CDownloadImpl::SetProperty(msdo::download_property key, const msdo::download_property_value& val) noexcept
{
    DODownloadProperty prop;
    DO_RETURN_IF_FAILED(ConvertToComProperty(key, prop));
    RETURN_IF_FAILED(_spDownload->SetProperty(prop, &val._val->native_value()));
    return DO_OK;
}

std::error_code CDownloadImpl::GetProperty(msdo::download_property key, msdo::download_property_value& value) noexcept
{
    DODownloadProperty prop;
    DO_RETURN_IF_FAILED(ConvertToComProperty(key, prop));

    unique_variant var;
    RETURN_IF_FAILED(_spDownload->GetProperty(prop, &var));

    switch (V_VT(&var))
    {
    case VT_BOOL:
        DO_RETURN_IF_FAILED(download_property_value::make(var.boolVal != VARIANT_FALSE, value));
        break;

    case VT_UI4:
        DO_RETURN_IF_FAILED(download_property_value::make(var.uintVal, value));
        break;

    case VT_UI8:
        DO_RETURN_IF_FAILED(download_property_value::make(var.ullVal, value));
        break;

    case VT_BSTR:
        DO_RETURN_IF_FAILED(download_property_value::make(var.bstrVal, value));
        break;

    default:
        return make_error_code(E_UNEXPECTED);
    }
    return DO_OK;
}

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
