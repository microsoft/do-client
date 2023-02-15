// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "download_impl.h"

#include "do_download_property_internal.h"
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

class DOStreamCallback :
    public RuntimeClass<RuntimeClassFlags<RuntimeClassType::ClassicCom>, ChainInterfaces<IStream, ISequentialStream>>
{
public:
    HRESULT RuntimeClassInitialize(const msdo::output_stream_callback_t& callback)
    {
        _callback = callback;
        return S_OK;
    }

    // ISequentialStream
    IFACEMETHODIMP Read(void*, ULONG, ULONG*) override { return E_NOTIMPL; }
    IFACEMETHODIMP Write(const void* pv, ULONG cb, ULONG* /* pcbWritten */) override
    {
        return HRESULT_FROM_WIN32(_callback(static_cast<const unsigned char*>(pv), cb).value());
    }
    
    // IStream (ISequentialStream would be sufficient but DO QI's for IStream)
    IFACEMETHODIMP Seek(LARGE_INTEGER, DWORD, ULARGE_INTEGER*) override { return E_NOTIMPL; }
    IFACEMETHODIMP SetSize(ULARGE_INTEGER) override { return E_NOTIMPL; }
    IFACEMETHODIMP CopyTo(IStream*, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER*) override { return E_NOTIMPL; }
    IFACEMETHODIMP Commit(DWORD) override { return E_NOTIMPL; }
    IFACEMETHODIMP Revert() override { return E_NOTIMPL; }
    IFACEMETHODIMP LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) override { return E_NOTIMPL; }
    IFACEMETHODIMP UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) override { return E_NOTIMPL; }
    IFACEMETHODIMP Stat(STATSTG*, DWORD) override { return E_NOTIMPL; }
    IFACEMETHODIMP Clone(IStream**) override { return E_NOTIMPL; }

private:
    msdo::output_stream_callback_t _callback;
};

std::error_code CDownloadImpl::Init(const std::string& uri, const std::string& downloadFilePath) noexcept
{
    ComPtr<IDOManager> manager;
    RETURN_IF_FAILED(CoCreateInstance(__uuidof(DeliveryOptimization), nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&manager)));
    ComPtr<IDODownload> spDownload;
    RETURN_IF_FAILED(manager->CreateDownload(&spDownload));

    RETURN_IF_FAILED(CoSetProxyBlanket(spDownload.Get(), RPC_C_AUTHN_DEFAULT,
        RPC_C_AUTHZ_NONE, COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr, EOAC_STATIC_CLOAKING));

    _spDownload = std::move(spDownload);

    // Assume full file until told otherwise
    _spRanges = std::make_unique<DO_DOWNLOAD_RANGES_INFO>();
    _spRanges->RangeCount = 0; // empty == full file

    download_property_value propUri;
    DO_RETURN_IF_FAILED(download_property_value::make(uri, propUri));
    DO_RETURN_IF_FAILED(SetProperty(download_property::uri, propUri));

    if (!downloadFilePath.empty())
    {
        download_property_value propDownloadFilePath;
        DO_RETURN_IF_FAILED(download_property_value::make(downloadFilePath, propDownloadFilePath));
        DO_RETURN_IF_FAILED(SetProperty(download_property::download_file_path, propDownloadFilePath));
    }

    return DO_OK;
}

std::error_code CDownloadImpl::Start() noexcept
{
    // Note: ranges are reset on Start. It is correct to pass nullptr for subsequent Start calls (i.e. "Resume").
    std::unique_ptr<DO_DOWNLOAD_RANGES_INFO> spRanges = std::move(_spRanges);
    return make_error_code(_spDownload->Start(spRanges.get()));
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
    ComPtr<DOStatusCallback> spCallback;
    RETURN_IF_FAILED(MakeAndInitialize<DOStatusCallback>(&spCallback, callback, download));

    unique_variant vtCallback;
    V_VT(&vtCallback) = VT_UNKNOWN;
    V_UNKNOWN(&vtCallback) = spCallback.Detach();
    RETURN_IF_FAILED(_spDownload->SetProperty(DODownloadProperty_CallbackInterface, &vtCallback));
    return DO_OK;
}
std::error_code CDownloadImpl::SetStreamCallback(const msdo::output_stream_callback_t& callback) noexcept
{
    ComPtr<DOStreamCallback> spCallback;
    RETURN_IF_FAILED(MakeAndInitialize<DOStreamCallback>(&spCallback, callback));

    unique_variant vtCallback;
    V_VT(&vtCallback) = VT_UNKNOWN;
    V_UNKNOWN(&vtCallback) = spCallback.Detach();
    RETURN_IF_FAILED(_spDownload->SetProperty(DODownloadProperty_StreamInterface, &vtCallback));

    // Streamed output requires ranges. Convert empty ranges into a full file range.
    if (_spRanges && (_spRanges->RangeCount == 0))
    {
        _spRanges->RangeCount = 1;
        _spRanges->Ranges[0].Offset = 0; // [0]: Minimum struct size includes 1 array entry. No need to reallocate.
        _spRanges->Ranges[0].Length = DO_LENGTH_TO_EOF;
    }

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
        return make_error_code(errc::unexpected);
    }
    return DO_OK;
}

// msdo::download_range and DO_DOWNLOAD_RANGE are equivalent -- safe to typecast/copy from one to the other
static_assert(sizeof(download_range) == sizeof(DO_DOWNLOAD_RANGE));
static_assert(FIELD_OFFSET(download_range, offset) == FIELD_OFFSET(DO_DOWNLOAD_RANGE, Offset));
static_assert(FIELD_OFFSET(download_range, length) == FIELD_OFFSET(DO_DOWNLOAD_RANGE, Length));
static_assert(length_to_eof == DO_LENGTH_TO_EOF);

std::error_code CDownloadImpl::SetRanges(const download_range* ranges, size_t count) noexcept
{
    size_t cbBufferSize = sizeof(DO_DOWNLOAD_RANGES_INFO); // includes 1 DO_DOWNLOAD_RANGE
    if (count > 1)
    {
        cbBufferSize += ((count - 1) * sizeof(DO_DOWNLOAD_RANGE));
    }

    void *pvBuffer = ::operator new(cbBufferSize, std::nothrow);
    if (pvBuffer == nullptr)
    {
        return make_error_code(E_OUTOFMEMORY);
    }

    std::unique_ptr<DO_DOWNLOAD_RANGES_INFO> spRanges(new(pvBuffer) DO_DOWNLOAD_RANGES_INFO());
    spRanges->RangeCount = static_cast<uint32_t>(count);
    memcpy(&spRanges->Ranges[0], ranges, count * sizeof(ranges[0]));

    _spRanges = std::move(spRanges);
    return DO_OK;
}

std::error_code CDownloadImpl::EnumDownloads(std::vector<std::unique_ptr<IDownload>>& out) noexcept
{
    out.clear();
    return _EnumDownloads(nullptr, out);
}

std::error_code CDownloadImpl::EnumDownloads(download_property prop, const std::string& value, std::vector<std::unique_ptr<IDownload>>& out) noexcept
{
    out.clear();
    std::wstring wval;
    DO_RETURN_IF_FAILED(UTF8toWstr(value, wval));
    return EnumDownloads(prop, wval, out);
}

std::error_code CDownloadImpl::EnumDownloads(download_property prop, const std::wstring& value, std::vector<std::unique_ptr<IDownload>>& out) noexcept
{
    out.clear();
    DO_DOWNLOAD_ENUM_CATEGORY category;
    DO_RETURN_IF_FAILED(ConvertToComProperty(prop, category.Property));
    category.Value = value.c_str();
    return _EnumDownloads(&category, out);
}

std::error_code CDownloadImpl::_EnumDownloads(const DO_DOWNLOAD_ENUM_CATEGORY* pCategory, std::vector<std::unique_ptr<IDownload>>& out) noexcept
{
    out.clear();
    ComPtr<IDOManager> manager;
    RETURN_IF_FAILED(CoCreateInstance(__uuidof(DeliveryOptimization), nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&manager)));
    ComPtr<IEnumUnknown> spEnum;
    RETURN_IF_FAILED(manager->EnumDownloads(pCategory, &spEnum));

    std::vector<std::unique_ptr<IDownload>> results;
    ULONG cFetched = 0;
    do
    {
        ComPtr<IUnknown> spunk;
        RETURN_IF_FAILED(spEnum->Next(1, &spunk, &cFetched));
        if (cFetched == 1)
        {
            ComPtr<IDODownload> spDownload;
            RETURN_IF_FAILED(spunk->QueryInterface(IID_PPV_ARGS(&spDownload)));

            RETURN_IF_FAILED(CoSetProxyBlanket(spDownload.Get(), RPC_C_AUTHN_DEFAULT,
                RPC_C_AUTHZ_NONE, COLE_DEFAULT_PRINCIPAL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
                nullptr, EOAC_STATIC_CLOAKING));

            auto tmp = std::make_unique<CDownloadImpl>();
            tmp->_spDownload = std::move(spDownload);

            // Assume full file in created state, else leave ranges null
            msdo::download_status status;
            DO_RETURN_IF_FAILED(tmp->GetStatus(status));
            if (status.state() == msdo::download_state::created)
            {
                tmp->_spRanges = std::make_unique<DO_DOWNLOAD_RANGES_INFO>();
                tmp->_spRanges->RangeCount = 0; // empty == full file
            }

            results.push_back(std::move(tmp));
        }
    } while (cFetched > 0);

    out = std::move(results);
    return DO_OK;
}

} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
