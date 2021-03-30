#include "tests_common.h"

#include <experimental/filesystem>
#include "do_config.h"
#include "do_download.h"
#include "do_download_status.h"
#include "do_exceptions.h"
#include "do_persistence.h"
#include "test_data.h"
#include "test_helpers.h"

namespace cppfs = std::experimental::filesystem;
namespace msdo = microsoft::deliveryoptimization;
namespace msdod = microsoft::deliveryoptimization::details;

class MCCDownloadTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        TestHelpers::CleanTestDir();
        if (cppfs::exists(msdod::GetConfigFilePath()))
        {
            cppfs::remove(msdod::GetConfigFilePath());
        }
    }

    void TearDown() override
    {
        SetUp();
        // Make sure docs reloads config immediately
        TestHelpers::RestartService(g_docsSvcName);
    }
};

TEST_F(MCCDownloadTests, DownloadWithMockIoTConnectionString)
{
    // Mock connecting string with GatewayHostName pointing to a valid MCC instance
    // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Not credentials (false positive)")]
    std::string iotConnString("HostName=instance-company-iothub-ver.host.tld;DeviceId=user-dev-name;SharedAccessKey=abcdefghijklmnopqrstuvwxyzABCDE123456789012=;GatewayHostName=");
    iotConnString += g_mccHostName;
    std::cout << "Using mock IoT connections string: " << iotConnString << '\n';
    int ret = deliveryoptimization_set_iot_connection_string(iotConnString.data());
    ASSERT_EQ(ret, 0);

    msdo::download::download_url_to_path(g_prodFileUrl, g_tmpFileName);
    // TODO(jimson): Parse docs logs and check download occurred via MCC
}

TEST_F(MCCDownloadTests, DownloadWithMockIoTConnectionStringInvalidHost)
{
    // Mock connecting string with GatewayHostName pointing to an invalid MCC instance
    // [SuppressMessage("Microsoft.Security", "CS002:SecretInNextLine", Justification="Not credentials (false positive)")]
    const char* expectedValue = "HostName=instance-company-iothub-ver.host.tld;DeviceId=user-dev-name;SharedAccessKey=abcdefghijklmnopqrstuvwxyzABCDE123456789012=;GatewayHostName=ahdkhkasdhaksd";
    int ret = deliveryoptimization_set_iot_connection_string(expectedValue);
    ASSERT_EQ(ret, 0);

    // Download should still succeed in a reasonable time
    msdo::download::download_url_to_path(g_prodFileUrl, g_tmpFileName, std::chrono::seconds(60));
}
