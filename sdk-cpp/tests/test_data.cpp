#include "test_data.h"
#include "boost/filesystem.hpp"

using namespace std::chrono_literals; // NOLINT(build/namespaces)

const uint64_t g_smallFileSizeBytes = 1837u;
const uint64_t g_largeFileSizeBytes = 536870440u;
const uint64_t g_prodFileSizeBytes = 25006511u;

const std::string g_largeFileUrl = "http://main.oremdl.microsoft.com.nsatc.net/dotc/ReplacementDCATFile.txt";
const std::string g_malformedFilePath = "?f309adfasdf///dfasdfj39fjasldfjasdf///           ///.1";
const std::string g_tmpFileName = (boost::filesystem::temp_directory_path() / boost::filesystem::path("docsdk_testfile.txt")).string();
const std::string g_tmpFileName2 = (boost::filesystem::temp_directory_path() / boost::filesystem::path("docsdk_testfile2.txt")).string();
const std::string g_tmpFileName3 = (boost::filesystem::temp_directory_path() / boost::filesystem::path("docsdk_testfile3.txt")).string();
const std::string g_smallFileUrl = "http://main.oremdl.microsoft.com.nsatc.net/dotc/49c591d405d307e25e72a19f7e79b53d69f19954/43A54FC03C6A979E9AAEAE2493757D1429A5C8A8D093FB7B8103E8CC8DF7B6B6";
const std::string g_404Url = "http://main.oremdl.microsoft.com.nsatc.net/dotc/49c591d405d307e25e72a19f7e79b53d69f19954/nonexistent";
const std::string g_prodFileUrl = "http://dl.delivery.mp.microsoft.com/filestreamingservice/files/52fa8751-747d-479d-8f22-e32730cc0eb1";


#if (DO_PLATFORM_ID == DO_PLATFORM_ID_LINUX)
const std::string g_docsProcName = "docs";
const std::string g_docsSvcName = "do-client-lite.service";

// This MCC instance only works within our test lab azure VMs. Can be overriden via cmdline.
std::string g_mccHostName = "10.1.0.70";
#endif