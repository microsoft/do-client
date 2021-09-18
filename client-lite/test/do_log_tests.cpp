#include "test_common.h"

#include <fstream>
#include "do_log.h"

class DOLoggerTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        ClearTestTempDir();
    }
};

TEST_F(DOLoggerTests, BasicWriteToFile)
{
    const char* pTestData[] = { "First line", "Second line", "{~LoggerImpl} Logger stats" };

    DOLog::Init(g_testTempDir.string(), DOLog::Level::Verbose);
    DoLogInfo("%s", pTestData[0]);
    DoLogError("%s", pTestData[1]);
    // The 3rd string is written by the logger on close
    DOLog::Close();

    UINT nLogFilesFound = 0;
    for (cppfs::recursive_directory_iterator itr(g_testTempDir); itr != cppfs::recursive_directory_iterator{}; ++itr)
    {
        ++nLogFilesFound;
        const auto filePath = itr->path();
        ASSERT_NE(strstr(filePath.c_str(), "do-agent."), nullptr);
        ASSERT_GT(cppfs::file_size(filePath), 0);

        std::ifstream fs;
        fs.open(filePath.string());
        ASSERT_TRUE(fs.is_open());

        char readBuf[256];
        UINT i = 0;
        // Expect the 2 lines and the final stats line for a total of 3 lines
        while (fs.getline(readBuf, ARRAYSIZE(readBuf)))
        {
            ASSERT_LT(i, 3);
            ASSERT_NE(strstr(readBuf, pTestData[i]), nullptr);
            ++i;
        }
        ASSERT_EQ(i, 3);
    }
    ASSERT_EQ(nLogFilesFound, 1);
}
