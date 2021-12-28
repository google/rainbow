// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "two_color.h"

#include <cstdint>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace gvisor {
namespace {

TEST(TwoColor, Paint) {
  uint8_t b_0[100];

  TwoColor::Paint(0, 0, b_0, sizeof(b_0));

  EXPECT_EQ(TwoColor::ColorMatch(0, 0, b_0, sizeof(b_0)), sizeof(b_0));
  EXPECT_EQ(TwoColor::ColorMatch(0, 1, b_0 + 1, sizeof(b_0) - 1),
            sizeof(b_0) - 1);

  uint8_t b_1[100];
  TwoColor::Paint(3, 1, b_1, sizeof(b_1));
  EXPECT_EQ(TwoColor::ColorMatch(3, 1, b_1, sizeof(b_1)), sizeof(b_1));

  EXPECT_EQ(TwoColor::ColorMatch(0, 0, b_1, sizeof(b_1)), 0);
}

TEST(TwoColor, Match) {
  uint8_t b[100];
  size_t buf_id = 0;
  for (size_t u = 0; u < 100; u += 17) {
    TwoColor::Paint(u, buf_id, b, sizeof(b));

    auto v = TwoColor::Identify(b, sizeof(b));
    EXPECT_TRUE(v.has_value());
    TwoColor::Identity id = v.value();
    EXPECT_EQ(id.identity, u);
    EXPECT_EQ(id.length, sizeof(b));
    EXPECT_EQ(id.phase, buf_id % TwoColor::kPeriod);

    for (size_t offset = 0; offset < sizeof(b) - TwoColor::kPeriod; offset++) {
      v = TwoColor::Identify(b + offset, sizeof(b) - offset);
      EXPECT_TRUE(v.has_value());
      id = v.value();
      EXPECT_EQ(id.identity, u);
      EXPECT_EQ(id.length, sizeof(b) - offset);
      EXPECT_EQ(id.phase, (buf_id + offset) % TwoColor::kPeriod);
    }
  }
  buf_id++;
}

TEST(TwoColor, ShortMatch) {
  const size_t kBufId = 19;
  uint8_t b[100];
  TwoColor::Paint(98, kBufId, b, sizeof(b));
  b[13] = 0;  // cut the match short
  auto v = TwoColor::Identify(b + 3, sizeof(b) - 3);
  EXPECT_TRUE(v.has_value());
  TwoColor::Identity id = v.value();
  EXPECT_EQ(id.identity, 98);
  EXPECT_EQ(id.length, 13 - 3);
  EXPECT_EQ(id.phase, (3 + kBufId) % TwoColor::kPeriod);
}

TEST(TwoColor, Constant) {
  uint8_t b[100];
  memset(b, 128, sizeof(b));
  auto v = TwoColor::Identify(b, sizeof(b));
  EXPECT_FALSE(v.has_value());
}

TEST(TwoColor, ConstantPreamble) {
  uint8_t b[100];
  TwoColor::Paint(93, 0, b, sizeof(b));
  memset(b, b[10], sizeof(10));
  auto v = TwoColor::Identify(b, sizeof(b));
  EXPECT_FALSE(v.has_value());
}

TEST(CrackColor, Garbage) {
  EXPECT_EQ(TwoColor::CrackColor(3, 11), "11 Garbage");
  EXPECT_EQ(TwoColor::CrackColor(0, 128 + 29), "157 Garbage");
  EXPECT_EQ(TwoColor::CrackColor(0, 64 + 31), "95 Garbage");
}

TEST(CrackColor, Foreign) {
  EXPECT_EQ(TwoColor::CrackColor(3, 128 + 1), "129 Foreign [1 mod 29]");
  EXPECT_EQ(TwoColor::CrackColor(3, 64 + 20), "84 Foreign [20 mod 31]");
}

TEST(CrackColor, Local) {
  EXPECT_EQ(TwoColor::CrackColor(3, 128 + 3), "131 Local [3 mod 29]");
  EXPECT_EQ(TwoColor::CrackColor(7, 64 + 7), "71 Local [7 mod 31]");
}

TEST(CrackColor, Root) {
  EXPECT_EQ(TwoColor::CrackColor(3, 128), "128 Root [0 mod 29]");
  EXPECT_EQ(TwoColor::CrackColor(7, 64), "64 Root [0 mod 31]");
}
}  // namespace
}  // namespace gvisor

