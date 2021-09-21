#include "download_impl.h"

#include "assert.h"

#include <wrl.h>
#include <wrl/client.h>
#include <wrl/implements.h>

#include "deliveryoptimization.h"
#include "do_download_property.h"
#include "do_exceptions.h"

namespace msdo = microsoft::deliveryoptimization;
using namespace microsoft::deliveryoptimization::details;
using namespace Microsoft::WRL;

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

static int32_t ConvertToComProperty(msdo::download_property key, DODownloadProperty& comProperty)
{
    switch (key)
    {
    case msdo::download_property::blocking_mode:
    {
        comProperty = DODownloadProperty_BlockingMode;
        return S_OK;
    }
    case msdo::download_property::callback_interface:
    {
        comProperty = DODownloadProperty_CallbackInterface;
        return S_OK;
    }
    case msdo::download_property::disallow_on_cellular:
    {
        comProperty = DODownloadProperty_DisallowOnCellular;
        return S_OK;
    }
    case msdo::download_property::caller_name:
    {
        comProperty = DODownloadProperty_DisplayName;
        return S_OK;
    }
    case msdo::download_property::catalog_id:
    {
        comProperty = DODownloadProperty_ContentId;
        return S_OK;
    }
    case msdo::download_property::correlation_vector:
    {
        comProperty = DODownloadProperty_CorrelationVector;
        return S_OK;
    }
    case msdo::download_property::cost_policy:
    {
        comProperty = DODownloadProperty_CostPolicy;
        return S_OK;
    }
    case msdo::download_property::decryption_info:
    {
        comProperty = DODownloadProperty_DecryptionInfo;
        return S_OK;
    }
    case msdo::download_property::download_file_path:
    {
        comProperty = DODownloadProperty_LocalPath;
        return S_OK;
    }
    case msdo::download_property::http_custom_auth_headers:
    {
        comProperty = DODownloadProperty::DODownloadProperty_HttpCustomAuthHeaders;
        return S_OK;
    }
    case msdo::download_property::http_custom_headers:
    {
        comProperty = DODownloadProperty_HttpCustomHeaders;
        return S_OK;
    }
    case msdo::download_property::id:
    {
        comProperty = DODownloadProperty_Id;
        return S_OK;
    }
    case msdo::download_property::integrity_check_info:
    {
        comProperty = DODownloadProperty_IntegrityCheckInfo;
        return S_OK;
    }
    case msdo::download_property::integrity_check_mandatory:
    {
        comProperty = DODownloadProperty_IntegrityCheckMandatory;
        return S_OK;
    }
    case msdo::download_property::network_token:
    {
        comProperty = DODownloadProperty_NetworkToken;
        return S_OK;
    }
    case msdo::download_property::no_progress_timeout_seconds:
    {
        comProperty = DODownloadProperty_NoProgressTimeoutSeconds;
        return S_OK;
    }
    case msdo::download_property::stream_interface:
    {
        comProperty = DODownloadProperty_StreamInterface;
        return S_OK;
    }
    case msdo::download_property::security_context:
    {
        comProperty = DODownloadProperty_SecurityContext;
        return S_OK;
    }
    case msdo::download_property::total_size_bytes:
    {
        comProperty = DODownloadProperty_TotalSizeBytes;
        return S_OK;
    }
    case msdo::download_property::uri:
    {
        comProperty = DODownloadProperty_Uri;
        return S_OK;
    }
    case msdo::download_property::use_foreground_priority:
    {
        comProperty = DODownloadProperty_ForegroundPriority;
        return S_OK;
    }
    default:
    {
        return E_INVALIDARG;
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

    IFACEMETHODIMP OnStatusChange(IDODownload* download, const DO_DOWNLOAD_STATUS* comStatus)
    {
        try
        {
            msdo::download_status status = ConvertFromComStatus(*comStatus);
            _callback(*_download, status);
        }
        catch (const std::bad_alloc&)
        {
            return E_OUTOFMEMORY;
        }
        catch (const std::exception&)
        {
            return HRESULT_FROM_WIN32(ERROR_UNHANDLED_EXCEPTION);
        }
        catch (...)
        {
            return E_UNEXPECTED;
        }
        return S_OK;
    }

private:
    msdo::download_property_value::status_callback_t _callback;
    msdo::download* _download;
};


CDownloadImpl::CDownloadImpl(const std::string& uri, const std::string& downloadFilePath)
{
    _codeInit = _Initialize(uri, downloadFilePath);
}


int32_t CDownloadImpl::_Initialize(const std::string& uri, const std::string& downloadFilePath)
{
    Microsoft::WRL::ComPtr<IDOManager> manager;
    RETURN_HR_IF_FAILED(CoCreateInstance(__uuidof(DeliveryOptimization), nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&manager)));
    Microsoft::WRL::ComPtr<IDODownload> spDownload;
    RETURN_HR_IF_FAILED(manager->CreateDownload(&spDownload));

    RETURN_HR_IF_FAILED(CoSetProxyBlanket(static_cast<IUnknown*>(spDownload.Get()), RPC_C_AUTHN_DEFAULT,
        RPC_C_AUTHZ_NONE, COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr, EOAC_STATIC_CLOAKING));

    download_property_value propUri(uri);
    download_property_value propDownloadFilePath(downloadFilePath);

    RETURN_HR_IF_FAILED(_SetPropertyHelper(*spDownload.Get(), download_property::uri, propUri));
    RETURN_HR_IF_FAILED(_SetPropertyHelper(*spDownload.Get(), download_property::download_file_path, propDownloadFilePath));

    _spDownload = std::move(spDownload);
    return 0;
}

// Support only full file downloads for now
int32_t CDownloadImpl::Start()
{
    RETURN_HR_IF_FAILED(_codeInit);
    _DO_DOWNLOAD_RANGES_INFO emptyRanges = {};
    emptyRanges.RangeCount = 0;
    return _spDownload->Start(&emptyRanges);
}

int32_t CDownloadImpl::Pause()
{
    RETURN_HR_IF_FAILED(_codeInit);
    return _spDownload->Pause();
}

int32_t CDownloadImpl::Resume()
{
    RETURN_HR_IF_FAILED(_codeInit);
    return _spDownload->Start(nullptr);
}

int32_t CDownloadImpl::Finalize()
{
    RETURN_HR_IF_FAILED(_codeInit);
    return _spDownload->Finalize();
}

int32_t CDownloadImpl::Abort()
{
    RETURN_HR_IF_FAILED(_codeInit);
    return _spDownload->Abort();
}

int32_t CDownloadImpl::SetCallback(const download_property_value::status_callback_t& callback, download& download)
{
    RETURN_HR_IF_FAILED(_codeInit);

    Microsoft::WRL::ComPtr<DOStatusCallback> spCallback;
    RETURN_HR_IF_FAILED(MakeAndInitialize<DOStatusCallback>(&spCallback, callback, download));

    VARIANT vtCallback;
    VariantInit(&vtCallback);
    V_VT(&vtCallback) = VT_UNKNOWN;
    V_UNKNOWN(&vtCallback) = spCallback.Get();
    spCallback.Get()->AddRef();
    DODownloadProperty prop;
    RETURN_HR_IF_FAILED(ConvertToComProperty(msdo::download_property::callback_interface, prop));
    const auto hr = _spDownload->SetProperty(prop, &vtCallback);
    VariantClear(&vtCallback);
    return hr;
}

// Return error code, pass in download_status by reference
int32_t CDownloadImpl::GetStatus(msdo::download_status& status)
{
    RETURN_HR_IF_FAILED(_codeInit);
    DO_DOWNLOAD_STATUS comStatus;
    RETURN_HR_IF_FAILED(_spDownload->GetStatus(&comStatus));
    status = ConvertFromComStatus(comStatus);
    return S_OK;
}

int32_t CDownloadImpl::GetProperty(msdo::download_property key, msdo::download_property_value& value)
{
    return _GetPropertyHelper(key, value);
}

int32_t CDownloadImpl::SetProperty(msdo::download_property key, const msdo::download_property_value& val)
{
    RETURN_HR_IF_FAILED(_codeInit);

    assert(key != msdo::download_property::callback_interface);
    return _SetPropertyHelper(*_spDownload.Get(), key, val);
}

int32_t CDownloadImpl::_SetPropertyHelper(IDODownload& download, msdo::download_property key, const msdo::download_property_value& val)
{
    DODownloadProperty prop;
    RETURN_HR_IF_FAILED(ConvertToComProperty(key, prop));

    // COM API SetProperty() is not declared with const reference for variant argument so need to const_cast
    return download.SetProperty(prop, &const_cast<msdo::download_property_value::native_type&>(val.native_value()));
}

int32_t CDownloadImpl::_GetPropertyHelper(msdo::download_property key, msdo::download_property_value& value)
{
    return E_NOTIMPL;
}
