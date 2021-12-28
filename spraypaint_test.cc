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

#include "spraypaint.h"

#include <cstddef>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace gvisor {
namespace {

class MockSprayPaintTest : public  SprayPaint {
 public:
  explicit MockSprayPaintTest(size_t buffer_size)
      : SprayPaint(buffer_size) {
  }
  ~MockSprayPaintTest() override = default;
  MOCK_METHOD(bool, TrySetAffinity, (int lpu), (override));
};

TEST(MockSprayPaintTest, KidTrySetAffinityPass) {
  MockSprayPaintTest spray_paint(10000);
  EXPECT_CALL(spray_paint, TrySetAffinity).WillOnce(testing::Return(true));
  EXPECT_EQ(spray_paint.Kid(11, 2), 0);
}

TEST(MockSprayPaintTest, KidTrySetAffinityFail) {
  MockSprayPaintTest spray_paint(10000);
  EXPECT_CALL(spray_paint, TrySetAffinity).WillOnce(testing::Return(false));
  EXPECT_EQ(spray_paint.Kid(11, 2), 0);
}

TEST(SprayPaint, TrySetAffinityFail) {
  SprayPaint spray_paint(10000);
  EXPECT_EQ(spray_paint.TrySetAffinity(-1), false);
}

TEST(Summarizer, OneRange) {
  SprayPaint spray_paint(20000);
  Summarizer s("test", &spray_paint, spray_paint.buffer());
  for (size_t k = 11; k < 50; k++) {
    s.Report(k, 13, "junk");
  }
  s.Finish();
  EXPECT_EQ(50 - 11, s.total_fails());
  EXPECT_EQ(1, s.range_count());
}

TEST(Summarizer, MultiRange) {
  SprayPaint spray_paint(20000);
  Summarizer s("test", &spray_paint, spray_paint.buffer());
  for (size_t r = 0; r < 3; r++) {
    for (size_t k = 1; k < 3; k++) {
      s.Report(k, 13, "junk");
    }
  }
  s.Finish();
  EXPECT_EQ(6, s.total_fails());
  EXPECT_EQ(3, s.range_count());
}

TEST(MockSprayPaintTest, TestMappedBufferSize) {
  EXPECT_EQ(SprayPaint::GetMappedBufferSize(), sysconf(_SC_PAGESIZE) * 3);
}

}  // namespace
}  // namespace gvisor
