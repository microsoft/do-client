#pragma once

#include "do_download_status.h"

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{

class IDownload
{
public:
    virtual ~IDownload() = default;

    virtual void Start() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual void Finalize() = 0;
    virtual void Abort() = 0;

    virtual download_status GetStatus() = 0;
};
} // namespace details
} // namespace deliveryoptimization
} // namespace microsoft
