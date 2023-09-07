// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "test_data.h"

const uint64_t g_prodFileSizeBytes = 25006511u;

const std::string g_smallFileUrl = "http://download.windowsupdate.com/phf/dotc/49c591d405d307e25e72a19f7e79b53d69f19954/43A54FC03C6A979E9AAEAE2493757D1429A5C8A8D093FB7B8103E8CC8DF7B6B6";
const std::string g_smallFile2Url = "http://extorigin-int.dcat.dsp.mp.microsoft.com/filestreamingservice/files/81701079-fa80-48b4-825a-27af227e4192";
const std::string g_largeFileUrl = "http://download.windowsupdate.com/phf/dotc/ReplacementDCATFile.txt";
const std::string g_404Url = "http://download.windowsupdate.com/phf/dotc/49c591d405d307e25e72a19f7e79b53d69f19954/nonexistent";
const std::string g_prodFileUrl = "http://dl.delivery.mp.microsoft.com/filestreamingservice/files/52fa8751-747d-479d-8f22-e32730cc0eb1";

// This MCC instance only works within our test lab azure VMs. Can be overriden via cmdline.
std::string g_mccHostName = "10.1.0.70";
