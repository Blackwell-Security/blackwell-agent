/*
 * Blackwell shared modules utils
 * Copyright (C) 2015, Blackwell Inc.
 * Nov 1, 2023.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include "blackwellDBQueryBuilder_test.hpp"
#include "blackwellDBQueryBuilder.hpp"
#include <string>

TEST_F(BlackwellDBQueryBuilderTest, GlobalTest)
{
    std::string message = BlackwellDBQueryBuilder::builder().global().selectAll().fromTable("agent").build();
    EXPECT_EQ(message, "global sql SELECT * FROM agent ");
}

TEST_F(BlackwellDBQueryBuilderTest, AgentTest)
{
    std::string message = BlackwellDBQueryBuilder::builder().agent("0").selectAll().fromTable("sys_programs").build();
    EXPECT_EQ(message, "agent 0 sql SELECT * FROM sys_programs ");
}

TEST_F(BlackwellDBQueryBuilderTest, WhereTest)
{
    std::string message = BlackwellDBQueryBuilder::builder()
                              .agent("0")
                              .selectAll()
                              .fromTable("sys_programs")
                              .whereColumn("name")
                              .equalsTo("bash")
                              .build();
    EXPECT_EQ(message, "agent 0 sql SELECT * FROM sys_programs WHERE name = 'bash' ");
}

TEST_F(BlackwellDBQueryBuilderTest, WhereAndTest)
{
    std::string message = BlackwellDBQueryBuilder::builder()
                              .agent("0")
                              .selectAll()
                              .fromTable("sys_programs")
                              .whereColumn("name")
                              .equalsTo("bash")
                              .andColumn("version")
                              .equalsTo("1")
                              .build();
    EXPECT_EQ(message, "agent 0 sql SELECT * FROM sys_programs WHERE name = 'bash' AND version = '1' ");
}

TEST_F(BlackwellDBQueryBuilderTest, WhereOrTest)
{
    std::string message = BlackwellDBQueryBuilder::builder()
                              .agent("0")
                              .selectAll()
                              .fromTable("sys_programs")
                              .whereColumn("name")
                              .equalsTo("bash")
                              .orColumn("version")
                              .equalsTo("1")
                              .build();
    EXPECT_EQ(message, "agent 0 sql SELECT * FROM sys_programs WHERE name = 'bash' OR version = '1' ");
}

TEST_F(BlackwellDBQueryBuilderTest, WhereIsNullTest)
{
    std::string message = BlackwellDBQueryBuilder::builder()
                              .agent("0")
                              .selectAll()
                              .fromTable("sys_programs")
                              .whereColumn("name")
                              .isNull()
                              .build();
    EXPECT_EQ(message, "agent 0 sql SELECT * FROM sys_programs WHERE name IS NULL ");
}

TEST_F(BlackwellDBQueryBuilderTest, WhereIsNotNullTest)
{
    std::string message = BlackwellDBQueryBuilder::builder()
                              .agent("0")
                              .selectAll()
                              .fromTable("sys_programs")
                              .whereColumn("name")
                              .isNotNull()
                              .build();
    EXPECT_EQ(message, "agent 0 sql SELECT * FROM sys_programs WHERE name IS NOT NULL ");
}

TEST_F(BlackwellDBQueryBuilderTest, InvalidValue)
{
    EXPECT_THROW(BlackwellDBQueryBuilder::builder()
                     .agent("0")
                     .selectAll()
                     .fromTable("sys_programs")
                     .whereColumn("name")
                     .equalsTo("bash'")
                     .build(),
                 std::runtime_error);
}

TEST_F(BlackwellDBQueryBuilderTest, InvalidColumn)
{
    EXPECT_THROW(BlackwellDBQueryBuilder::builder()
                     .agent("0")
                     .selectAll()
                     .fromTable("sys_programs")
                     .whereColumn("name'")
                     .equalsTo("bash")
                     .build(),
                 std::runtime_error);
}

TEST_F(BlackwellDBQueryBuilderTest, InvalidTable)
{
    EXPECT_THROW(BlackwellDBQueryBuilder::builder()
                     .agent("0")
                     .selectAll()
                     .fromTable("sys_programs'")
                     .whereColumn("name")
                     .equalsTo("bash")
                     .build(),
                 std::runtime_error);
}

TEST_F(BlackwellDBQueryBuilderTest, GlobalGetCommand)
{
    std::string message = BlackwellDBQueryBuilder::builder().globalGetCommand("agent-info 1").build();
    EXPECT_EQ(message, "global get-agent-info 1 ");
}

TEST_F(BlackwellDBQueryBuilderTest, GlobalFindCommand)
{
    std::string message = BlackwellDBQueryBuilder::builder().globalFindCommand("agent 1").build();
    EXPECT_EQ(message, "global find-agent 1 ");
}

TEST_F(BlackwellDBQueryBuilderTest, GlobalSelectCommand)
{
    std::string message = BlackwellDBQueryBuilder::builder().globalSelectCommand("agent-name 1").build();
    EXPECT_EQ(message, "global select-agent-name 1 ");
}

TEST_F(BlackwellDBQueryBuilderTest, AgentGetOsInfoCommand)
{
    std::string message = BlackwellDBQueryBuilder::builder().agentGetOsInfoCommand("1").build();
    EXPECT_EQ(message, "agent 1 osinfo get ");
}

TEST_F(BlackwellDBQueryBuilderTest, AgentGetHotfixesCommand)
{
    std::string message = BlackwellDBQueryBuilder::builder().agentGetHotfixesCommand("1").build();
    EXPECT_EQ(message, "agent 1 hotfix get ");
}

TEST_F(BlackwellDBQueryBuilderTest, AgentGetPackagesCommand)
{
    std::string message = BlackwellDBQueryBuilder::builder().agentGetPackagesCommand("1").build();
    EXPECT_EQ(message, "agent 1 package get ");
}
