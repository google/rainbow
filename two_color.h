// Copyright 2022 Google LLC
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

#ifndef TWO_COLOR_H_
#define TWO_COLOR_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace gvisor {

// Buffer painter.
//   Distinctive data patterns are used to make buffer ownership traceable.
//   (This is one of many possible schemes for achieving that goal.
//    It's a rough experimental compromise between trains of thought.)
//   Identity is carried by the "colors", i.e. byte values, that a buffer
//   contains.
//   Two colors are used, with a byte encoding scheme providing 899 = 29 * 31
//   distinguishable buffer owners.
//   Given an owner, and thus a pair of colors, the sequencing of the colors
//   is used for several purposes:
//   a. to (loosely) distinguish multiple buffers owned by the same owner
//   b. to (loosely) distinguish cache lines and pages etc of a given buffer
//   c. to reduce the probability that arbitrary memory resembles these buffers
class TwoColor {
 public:
  static constexpr size_t kPeriod = 7;

  TwoColor() {}

  // Paints 'buffer' with prescribed pattern.
  static void Paint(size_t identity, size_t buffer_id, uint8_t *buffer,
                    size_t length);

  // Returns expected color.
  static uint8_t Color(size_t identity, size_t buffer_id, size_t position) {
    return ColorSelector(buffer_id, position) ? HighColor(identity)
                                              : LowColor(identity);
  }

  // Returns length of longest consistently colored prefix of 'buffer'.
  static size_t ColorMatch(size_t identity, size_t phase, const uint8_t *buffer,
                           size_t length);

  struct Identity {
    size_t identity;
    size_t length;
    size_t phase;  // Modulo kPeriod

    std::string ToString() const;
  };

  // Returns identity of longest consistently-painted prefix of 'buffer'.
  // If the length is more than a few bytes, the probability is high that the
  // buffer really belongs to the indicated owner.
  static std::optional<Identity> Identify(const uint8_t *buffer, size_t length);

  // Returns a string describing 'color'.
  static std::string CrackColor(size_t local_identity, uint8_t color);

 private:
  // Color byte format:
  //   High order 3 bits: tag, valid values: kLowTag, kHighTag identify modulus
  //   Low order 5 bits: identity modulo kLowPrime or kHighPrime
  static constexpr uint8_t kLowPrime = 29;
  static constexpr uint8_t kHighPrime = 31;
  static constexpr uint8_t kLowTag = 0x80;
  static constexpr uint8_t kHighTag = 0x40;
  static uint8_t Tag(uint8_t color) { return color & 0xe0; }
  static uint8_t Residue(uint8_t color) { return color & 0x1f; }

  static uint8_t LowColor(size_t identity) {
    return kLowTag | (identity % kLowPrime);
  }
  static uint8_t HighColor(size_t identity) {
    return kHighTag | (identity % kHighPrime);
  }
  static bool IsValidLowColor(uint8_t);
  static bool IsValidHighColor(uint8_t);

  // Defines a periodic in 'position' color pattern, with period relatively
  // prime to most everything. Phase is shifted by 'buffer_id'.
  static size_t ColorSelector(size_t buffer_id, size_t position) {
    size_t v = buffer_id + position;
    return (v % kPeriod) < (kPeriod / 2) ? 0 : 1;
  }

  struct CandidateColors {
    size_t identity;
    size_t phase;
  };

  // Returns candidate color scheme of 'buffer', or nullopt if none.
  static std::optional<CandidateColors> Candidates(const uint8_t *buffer,
                                                   size_t length);
  // Returns identity given colors.
  static size_t Crt(uint8_t c_low, uint8_t c_high);
};

}  // namespace gvisor

#endif  // TWO_COLOR_H_
