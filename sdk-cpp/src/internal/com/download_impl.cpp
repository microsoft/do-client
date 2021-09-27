// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "download_impl.h"

#include "assert.h"

#include <wrl.h>
#include <wrl/client.h>
#include <wrl/implements.h>

#include "deliveryoptimization.h"
#include "do_download_property.h"
#include "do_download_property_internal.h"
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

static DODownloadProperty ConvertToComProperty(msdo::download_property key)
{
    switch (key)
    {
    case msdo::download_property::blocking_mode:
    {
        return DODownloadProperty_BlockingMode;
    }
    case msdo::download_property::callback_interface:
    {
        return DODownloadProperty_CallbackInterface;
    }
    case msdo::download_property::disallow_on_cellular:
    {
        return DODownloadProperty_DisallowOnCellular;
    }
    case msdo::download_property::caller_name:
    {
        return DODownloadProperty_DisplayName;
    }
    case msdo::download_property::catalog_id:
    {
        return DODownloadProperty_ContentId;
    }
    case msdo::download_property::correlation_vector:
    {
        return DODownloadProperty_CorrelationVector;
    }
    case msdo::download_property::cost_policy:
    {
        return DODownloadProperty_CostPolicy;
    }
    case msdo::download_property::decryption_info:
    {
        return DODownloadProperty_DecryptionInfo;
    }
    case msdo::download_property::download_file_path:
    {
        return DODownloadProperty_LocalPath;
    }
    case msdo::download_property::http_custom_auth_headers:
    {
        return DODownloadProperty::DODownloadProperty_HttpCustomAuthHeaders;
    }
    case msdo::download_property::http_custom_headers:
    {
        return DODownloadProperty_HttpCustomHeaders;
    }
    case msdo::download_property::id:
    {
        return DODownloadProperty_Id;
    }
    case msdo::download_property::integrity_check_info:
    {
        return DODownloadProperty_IntegrityCheckInfo;
    }
    case msdo::download_property::integrity_check_mandatory:
    {
        return DODownloadProperty_IntegrityCheckMandatory;
    }
    case msdo::download_property::network_token:
    {
        return DODownloadProperty_NetworkToken;
    }
    case msdo::download_property::no_progress_timeout_seconds:
    {
        return DODownloadProperty_NoProgressTimeoutSeconds;
    }
    case msdo::download_property::stream_interface:
    {
        return DODownloadProperty_StreamInterface;
    }
    case msdo::download_property::security_context:
    {
        return DODownloadProperty_SecurityContext;
    }
    case msdo::download_property::total_size_bytes:
    {
        return DODownloadProperty_TotalSizeBytes;
    }
    case msdo::download_property::uri:
    {
        return DODownloadProperty_Uri;
    }
    case msdo::download_property::use_foreground_priority:
    {
        return DODownloadProperty_ForegroundPriority;
    }
    default:
    {
        throw E_INVALIDARG;
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
    Microsoft::WRL::ComPtr<IDOManager> manager;
    throw_if_fail(CoCreateInstance(__uuidof(DeliveryOptimization), nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&manager)));
    Microsoft::WRL::ComPtr<IDODownload> spDownload;
    throw_if_fail(manager->CreateDownload(&spDownload));

    try {
        throw_if_fail(CoSetProxyBlanket(static_cast<IUnknown*>(spDownload.Get()), RPC_C_AUTHN_DEFAULT,
            RPC_C_AUTHZ_NONE, COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
            nullptr, EOAC_STATIC_CLOAKING));

        download_property_value propUri(uri);
        download_property_value propDownloadFilePath(downloadFilePath);

        _SetPropertyHelper(*spDownload.Get(), download_property::uri, propUri);
        _SetPropertyHelper(*spDownload.Get(), download_property::download_file_path, propDownloadFilePath);
    }
    catch (const exception& e)
    {
        spDownload->Abort();
        throw e;
    }
    _spDownload = std::move(spDownload);
}

// Support only full file downloads for now
void CDownloadImpl::Start()
{
    _DO_DOWNLOAD_RANGES_INFO emptyRanges = {};
    emptyRanges.RangeCount = 0;
    throw_if_fail(_spDownload->Start(&emptyRanges));
}

void CDownloadImpl::Pause()
{
    throw_if_fail(_spDownload->Pause());
}

void CDownloadImpl::Resume()
{
    throw_if_fail(_spDownload->Start(nullptr));
}

void CDownloadImpl::Finalize()
{
    throw_if_fail(_spDownload->Finalize());
}

void CDownloadImpl::Abort()
{
    throw_if_fail(_spDownload->Abort());
}

void CDownloadImpl::SetCallback(const download_property_value::status_callback_t& callback, download& download)
{
    Microsoft::WRL::ComPtr<DOStatusCallback> spCallback;
    throw_if_fail(MakeAndInitialize<DOStatusCallback>(&spCallback, callback, download));

    VARIANT vtCallback;
    VariantInit(&vtCallback);
    V_VT(&vtCallback) = VT_UNKNOWN;
    V_UNKNOWN(&vtCallback) = spCallback.Get();
    spCallback.Get()->AddRef();
    const auto hr = _spDownload->SetProperty(ConvertToComProperty(msdo::download_property::callback_interface), &vtCallback);
    VariantClear(&vtCallback);
    throw_if_fail(hr);
}

msdo::download_status CDownloadImpl::GetStatus()
{
    DO_DOWNLOAD_STATUS status;
    throw_if_fail(_spDownload->GetStatus(&status));
    return ConvertFromComStatus(status);
}

msdo::download_property_value CDownloadImpl::GetProperty(msdo::download_property key)
{
    return _GetPropertyHelper(key);
}

void CDownloadImpl::SetProperty(msdo::download_property key, const msdo::download_property_value& val)
{
    assert(key != msdo::download_property::callback_interface);
    _SetPropertyHelper(*_spDownload.Get(), key, val);
}

void CDownloadImpl::_SetPropertyHelper(IDODownload& download, msdo::download_property key, const msdo::download_property_value& val)
{
    DODownloadProperty prop = ConvertToComProperty(key);

    throw_if_fail(download.SetProperty(prop, &(val._val->native_value())));
}

msdo::download_property_value CDownloadImpl::_GetPropertyHelper(msdo::download_property key)
{
    throw E_NOTIMPL;
}
