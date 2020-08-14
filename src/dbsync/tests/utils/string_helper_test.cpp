#include "string_helper_test.h"
#include "string_helper.h"

void StringUtilsTest::SetUp() {};

void StringUtilsTest::TearDown() {};

TEST_F(StringUtilsTest, CheckReplacement) 
{
    std::string string_base { "hello_world" };
    const auto ret_val { Utils::replaceAll(string_base, "hello_", "bye_") };
    EXPECT_EQ(string_base, "bye_world");
    EXPECT_TRUE(ret_val);
}

TEST_F(StringUtilsTest, CheckNotReplacement) 
{
    std::string string_base {"hello_world" };
    const auto ret_val { Utils::replaceAll(string_base, "nothing_", "bye_") };
    EXPECT_EQ(string_base, "hello_world");
    EXPECT_FALSE(ret_val);
}

TEST_F(StringUtilsTest, SplitEmptyString) 
{
    auto split_text_vector { Utils::split("", '.') };
    EXPECT_EQ(0ull, split_text_vector.size());
}

TEST_F(StringUtilsTest, SplitDelimiterNoCoincidence) 
{
    const auto split_text_vector { Utils::split("hello_world", '.') };
    EXPECT_EQ(1ull, split_text_vector.size());
}

TEST_F(StringUtilsTest, SplitDelimiterCoincidence) 
{
    const auto split_text_vector { Utils::split("hello.world", '.') };
    EXPECT_EQ(2ull, split_text_vector.size());
    EXPECT_EQ(split_text_vector[0], "hello");
    EXPECT_EQ(split_text_vector[1], "world");
}

