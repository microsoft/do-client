// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "download_impl.h"

#include <assert.h>
#include <wrl.h>
#include <wrl/client.h>
#include <wrl/implements.h>

#include "do_download_property.h"
#include "do_download_property_internal.h"
#include "do_errors.h"
#include "do_error_helpers.h"

namespace msdo = microsoft::deliveryoptimization;
using namespace Microsoft::WRL;

#ifndef FAILED
#define FAILED(hr) (((int32_t)(hr)) < 0)
#endif

#ifndef RETURN_IF_FAILED
#define RETURN_IF_FAILED(hr)  {     \
    int32_t __hr = (hr);            \
    if (FAILED(__hr)) return std::error_code(__hr, msdo::details::do_category()); }
#endif

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

static msdo::download_state ConvertFromComState(DODownloadState platformState)
{
    msdo::download_state state;
    switch (platformState)
    {
    case DODownloadState_Created:
    {
        state = msdo::download_state::created;
        break;
    }
    case DODownloadState_Transferring:
    {
        state = msdo::download_state::transferring;
        break;
    }
    case DODownloadState_Transferred:
    {
        state = msdo::download_state::transferred;
        break;
    }
    case DODownloadState_Finalized:
    {
        state = msdo::download_state::finalized;
        break;
    }
    case DODownloadState_Aborted:
    {
        state = msdo::download_state::aborted;
        break;
    }
    case DODownloadState_Paused:
    {
        state = msdo::download_state::paused;
        break;
    }
    }
    return state;
}

static msdo::download_status ConvertFromComStatus(const DO_DOWNLOAD_STATUS& platformStatus)
{
    return msdo::download_status(platformStatus.BytesTotal, platformStatus.BytesTransferred, platformStatus.Error,
        platformStatus.ExtendedError, ConvertFromComState(platformStatus.State));
}

static std::error_code ConvertToComProperty(msdo::download_property key, DODownloadProperty& comProperty)
{
    switch (key)
    {
    case msdo::download_property::blocking_mode:
    {
        comProperty = DODownloadProperty_BlockingMode;
        return DO_OK;
    }
    case msdo::download_property::callback_interface:
    {
        comProperty = DODownloadProperty_CallbackInterface;
        return DO_OK;
    }
    case msdo::download_property::disallow_on_cellular:
    {
        comProperty = DODownloadProperty_DisallowOnCellular;
        return DO_OK;
    }
    case msdo::download_property::caller_name:
    {
        comProperty = DODownloadProperty_DisplayName;
        return DO_OK;
    }
    case msdo::download_property::catalog_id:
    {
        comProperty = DODownloadProperty_ContentId;
        return DO_OK;
    }
    case msdo::download_property::correlation_vector:
    {
        comProperty = DODownloadProperty_CorrelationVector;
        return DO_OK;
    }
    case msdo::download_property::cost_policy:
    {
        comProperty = DODownloadProperty_CostPolicy;
        return DO_OK;
    }
    case msdo::download_property::decryption_info:
    {
        comProperty = DODownloadProperty_DecryptionInfo;
        return DO_OK;
    }
    case msdo::download_property::download_file_path:
    {
        comProperty = DODownloadProperty_LocalPath;
        return DO_OK;
    }
    case msdo::download_property::http_custom_auth_headers:
    {
        comProperty = DODownloadProperty::DODownloadProperty_HttpCustomAuthHeaders;
        return DO_OK;
    }
    case msdo::download_property::http_custom_headers:
    {
        comProperty = DODownloadProperty_HttpCustomHeaders;
        return DO_OK;
    }
    case msdo::download_property::id:
    {
        comProperty = DODownloadProperty_Id;
        return DO_OK;
    }
    case msdo::download_property::integrity_check_info:
    {
        comProperty = DODownloadProperty_IntegrityCheckInfo;
        return DO_OK;
    }
    case msdo::download_property::integrity_check_mandatory:
    {
        comProperty = DODownloadProperty_IntegrityCheckMandatory;
        return DO_OK;
    }
    case msdo::download_property::network_token:
    {
        comProperty = DODownloadProperty_NetworkToken;
        return DO_OK;
    }
    case msdo::download_property::no_progress_timeout_seconds:
    {
        comProperty = DODownloadProperty_NoProgressTimeoutSeconds;
        return DO_OK;
    }
    case msdo::download_property::stream_interface:
    {
        comProperty = DODownloadProperty_StreamInterface;
        return DO_OK;
    }
    case msdo::download_property::security_context:
    {
        comProperty = DODownloadProperty_SecurityContext;
        return DO_OK;
    }
    case msdo::download_property::total_size_bytes:
    {
        comProperty = DODownloadProperty_TotalSizeBytes;
        return DO_OK;
    }
    case msdo::download_property::uri:
    {
        comProperty = DODownloadProperty_Uri;
        return DO_OK;
    }
    case msdo::download_property::use_foreground_priority:
    {
        comProperty = DODownloadProperty_ForegroundPriority;
        return DO_OK;
    }
    default:
    {
        return make_error_code(E_INVALIDARG);
    }
    }
}

class DOStatusCallback :
    public RuntimeClass<RuntimeClassFlags<RuntimeClassType::ClassicCom>, IDODownloadStatusCallback>
{
public:
    HRESULT RuntimeClassInitialize(const msdo::download_property_value::status_callback_t& callback, msdo::download& download)
    {
        _download = &download;
        _callback = callback;
        return S_OK;
    }

    IFACEMETHODIMP OnStatusChange(IDODownload* download, const DO_DOWNLOAD_STATUS* comStatus) noexcept
    {
        msdo::download_status status = ConvertFromComStatus(*comStatus);
        _callback(*_download, status);
        return S_OK;
    }

private:
    msdo::download_property_value::status_callback_t _callback;
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

    DO_RETURN_IF_FAILED(_SetPropertyHelper(*spDownload.Get(), download_property::uri, propUri));
    DO_RETURN_IF_FAILED(_SetPropertyHelper(*spDownload.Get(), download_property::download_file_path, propDownloadFilePath));

    _spDownload = std::move(spDownload);
    return DO_OK;;
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

std::error_code CDownloadImpl::GetProperty(msdo::download_property key, msdo::download_property_value& value) noexcept
{
    return _GetPropertyHelper(key, value);
}

std::error_code CDownloadImpl::SetProperty(msdo::download_property key, const msdo::download_property_value& val) noexcept
{
    assert(key != msdo::download_property::callback_interface);
    return _SetPropertyHelper(*_spDownload.Get(), key, val);
}

std::error_code CDownloadImpl::SetCallback(const download_property_value::status_callback_t& callback, download& download) noexcept
{
    Microsoft::WRL::ComPtr<DOStatusCallback> spCallback;
    RETURN_IF_FAILED(MakeAndInitialize<DOStatusCallback>(&spCallback, callback, download));

    VARIANT vtCallback;
    VariantInit(&vtCallback);
    V_VT(&vtCallback) = VT_UNKNOWN;
    V_UNKNOWN(&vtCallback) = spCallback.Get();
    spCallback.Get()->AddRef();
    DODownloadProperty prop;
    ConvertToComProperty(msdo::download_property::callback_interface, prop);
    const auto hr = _spDownload->SetProperty(prop, &vtCallback);
    VariantClear(&vtCallback);
    return make_error_code(hr);
}

std::error_code CDownloadImpl::_SetPropertyHelper(IDODownload& download, msdo::download_property key, const msdo::download_property_value& val) noexcept
{
    DODownloadProperty prop;
    DO_RETURN_IF_FAILED(ConvertToComProperty(key, prop));

    return make_error_code(download.SetProperty(prop, &val._val->native_value()));
}

std::error_code CDownloadImpl::_GetPropertyHelper(msdo::download_property key, msdo::download_property_value& value) noexcept
{
    return make_error_code(errc::e_not_impl);
}

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
