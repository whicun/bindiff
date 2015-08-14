#include "testing/base/public/gunit-spi.h"
#include "gtest/gtest.h"
#include "third_party/zynamics/bindiff/instruction.h"

namespace {

TEST(InstructionTest, CacheEntry) {
  CacheEntry entry1(1234, "no operands");
  EXPECT_EQ(1234, entry1.prime_);
  EXPECT_STREQ("no operands", entry1.operands_.c_str());
  CacheEntry entry2(1234, "no operands");
  EXPECT_EQ(entry1, entry2);
}

TEST(InstructionTest, Instruction) {
  Instruction::Cache cache;
  Instruction instruction(&cache, 0xbaadf00dbaadf00d,
                          "mov", 1234, "eax, 47011");
  EXPECT_STREQ("mov", instruction.GetMnemonic(&cache).c_str());
  EXPECT_EQ(1234, instruction.GetPrime());
  EXPECT_STREQ("eax, 47011", instruction.GetOperandString().c_str());
  EXPECT_EQ(0xbaadf00dbaadf00d, instruction.GetAddress());
}

TEST(InstructionTest, Instructions) {
  Instruction::Cache cache;
  Instructions instructions;
  instructions.emplace_back(&cache, 0x1000000010000000, "one",
                            1234, "47, 11");
  instructions.emplace_back(&cache, 0x1000000010000001, "two",
                            1235, "47, 11");
  instructions.emplace_back(&cache, 0x1000000010000005, "three",
                            1236, "47, 11");
  EXPECT_EQ(0x1000000010000001,
            GetInstruction(instructions, 0x1000000010000001)->GetAddress());
  EXPECT_STREQ("three", GetInstruction(instructions, 0x1000000010000005)
                            ->GetMnemonic(&cache)
                            .c_str());

  Instructions instructions2;
  instructions2.emplace_back(&cache, 0x1000000010012000, "one",
                             1232, "47, 11");
  instructions2.emplace_back(&cache, 0x1000000010012302, "one",
                             1234, "47, 11");
  instructions2.emplace_back(&cache, 0x1000000010033300, "one",
                             1237, "47, 11");
  instructions2.emplace_back(&cache, 0x1000000010112334, "two",
                             1235, "47, 11");
  instructions2.emplace_back(&cache, 0x1000000010234205, "three",
                             1236, "47, 11");
  instructions2.emplace_back(&cache, 0x1000000010234206, "three",
                             1237, "47, 11");
  instructions2.emplace_back(&cache, 0x1000000010234207, "three",
                             1236, "47, 11");
  InstructionMatches matches;
  ComputeLcs(instructions.begin(), instructions.end(),
             instructions2.begin(), instructions2.end(), matches);
  EXPECT_EQ(3, matches.size());
  EXPECT_EQ(0x1000000010000000, matches[0].first->GetAddress());
  EXPECT_EQ(0x1000000010012302, matches[0].second->GetAddress());
  EXPECT_EQ(0x1000000010000001, matches[1].first->GetAddress());
  EXPECT_EQ(0x1000000010112334, matches[1].second->GetAddress());
  EXPECT_EQ(0x1000000010000005, matches[2].first->GetAddress());
  EXPECT_EQ(0x1000000010234207, matches[2].second->GetAddress());
  EXPECT_EQ(1236, matches[2].second->GetPrime());
}

TEST(InstructionTest, LcsEmpty) {
  Instructions instructions1;
  Instructions instructions2;
  InstructionMatches matches;
  ComputeLcs(instructions1.begin(), instructions1.end(),
             instructions2.begin(), instructions2.end(), matches);
  EXPECT_EQ(0, matches.size());
}

TEST(InstructionTest, LcsEmptyOneSided) {
  Instructions instructions1;
  Instruction::Cache cache;
  instructions1.emplace_back(&cache, 0x1000000010000000, "one",
                             1234, "47, 11");
  instructions1.emplace_back(&cache, 0x1000000010000001, "two",
                             1235, "47, 11");
  instructions1.emplace_back(&cache, 0x1000000010000005, "three",
                             1236, "47, 11");

  Instructions instructions2;
  InstructionMatches matches;
  ComputeLcs(instructions1.begin(), instructions1.end(),
             instructions2.begin(), instructions2.end(), matches);
  EXPECT_EQ(0, matches.size());
}

TEST(InstructionTest, CommonPrefix) {
  Instructions instructions1;
  Instruction::Cache cache;
  instructions1.emplace_back(&cache, 0x1000000010000000, "one", 1234, "47, 11");
  instructions1.emplace_back(&cache, 0x1000000010000001, "two", 1235, "47, 11");
  instructions1.emplace_back(&cache, 0x1000000010000005, "three", 1236,
                             "47, 11");

  Instructions instructions2;
  instructions2.emplace_back(&cache, 0x1000000010000000, "one", 1234, "47, 11");
  instructions2.emplace_back(&cache, 0x1000000010000001, "two", 1235, "47, 11");
  instructions2.emplace_back(&cache, 0x1000000010000005, "unmatched", 7777,
                             "47, 11");
  InstructionMatches matches;
  ComputeLcs(instructions1.begin(), instructions1.end(),
             instructions2.begin(), instructions2.end(), matches);
  EXPECT_EQ(2, matches.size());
  EXPECT_EQ(0x1000000010000000, matches[0].first->GetAddress());
  EXPECT_EQ(0x1000000010000000, matches[0].second->GetAddress());
  EXPECT_EQ(0x1000000010000001, matches[1].first->GetAddress());
  EXPECT_EQ(0x1000000010000001, matches[1].second->GetAddress());
}

}  // namespace