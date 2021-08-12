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
    const char* pInputData[] = { "First line", "Second line" };

    DOLog::Init(g_testTempDir.string(), DOLog::Level::Verbose);
    DoLogInfo("%s", pInputData[0]);
    DoLogError("%s", pInputData[1]);
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
        while (fs.getline(readBuf, ARRAYSIZE(readBuf)))
        {
            ASSERT_LT(i, 2);
            ASSERT_NE(strstr(readBuf, pInputData[i]), nullptr);
            ++i;
        }
        ASSERT_EQ(i, 2);
    }
    ASSERT_EQ(nLogFilesFound, 1);
}
