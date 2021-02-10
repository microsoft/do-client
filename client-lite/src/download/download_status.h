#pragma once

enum class DownloadState
{
    Created,
    Transferring,
    Transferred,
    Finalized,
    Aborted,
    Paused,
};

struct DownloadStatus
{
    friend class Download;

    UINT64 BytesTotal { 0 };
    UINT64 BytesTransferred { 0 };
    DownloadState State { DownloadState::Created };
    HRESULT Error { S_OK };
    HRESULT ExtendedError { S_OK };

    bool IsTransientError() const noexcept
    {
        return (State == DownloadState::Paused) && (Error == S_OK) && FAILED(ExtendedError);
    }

private:
    void _Transferring()
    {
        State = DownloadState::Transferring;
        Error = S_OK;
        ExtendedError = S_OK;
    }
    void _Paused(HRESULT hrError = S_OK, HRESULT hrExtendedError = S_OK)
    {
        State = DownloadState::Paused;
        Error = hrError;
        ExtendedError = hrExtendedError;
    }
    void _Transferred()
    {
        State = DownloadState::Transferred;
    }
    void _Finalized()
    {
        State = DownloadState::Finalized;
    }
    void _Aborted()
    {
        State = DownloadState::Aborted;
    }
};
