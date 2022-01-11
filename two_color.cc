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

#include "two_color.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "absl/strings/str_format.h"
#include "log.h"

namespace gvisor {

bool TwoColor::IsValidLowColor(uint8_t c) {
  if (Tag(c) != kLowTag) return false;
  const uint8_t residue = Residue(c);
  if (residue >= kLowPrime) return false;
  return true;
}

bool TwoColor::IsValidHighColor(uint8_t c) {
  if (Tag(c) != kHighTag) return false;
  const uint8_t residue = Residue(c);
  if (residue >= kHighPrime) return false;
  return true;
}

void TwoColor::Paint(size_t identity, size_t buffer_id, uint8_t *buffer,
                     size_t length) {
  for (size_t k = 0; k < length; k++) {
    buffer[k] = Color(identity, buffer_id, k);
  }
}

size_t TwoColor::ColorMatch(size_t identity, size_t phase,
                            const uint8_t *buffer, size_t length) {
  for (size_t k = 0; k < length; k++) {
    if (buffer[k] != Color(identity, phase, k)) return k;
  }
  return length;
}

std::string TwoColor::CrackColor(size_t local_identity, uint8_t color) {
  if (!(IsValidLowColor(color) || IsValidHighColor(color))) {
    return absl::StrFormat("%d Garbage", color);
  }
  const bool low_color = IsValidLowColor(color);
  const uint8_t modulus = low_color ? kLowPrime : kHighPrime;
  const uint8_t residue = Residue(color);

  const bool root = residue == 0;
  const bool local = color == (low_color ? LowColor(local_identity)
                                         : HighColor(local_identity));
  std::string provenance = root ? "Root" : (local ? "Local" : "Foreign");
  return absl::StrFormat("%d %s [%d mod %d]", color, provenance, residue,
                         modulus);
}

std::optional<TwoColor::CandidateColors> TwoColor::Candidates(
    const uint8_t *buffer, size_t length) {
  if (length < 2) return std::nullopt;
  // Find first color transition.
  const uint8_t c_0 = buffer[0];
  size_t k = 1;
  while (k < std::min(length, kPeriod) && buffer[k] == c_0) {
    k++;
  }
  if (k >= std::min(length, kPeriod)) return std::nullopt;
  const uint8_t c_1 = buffer[k];

  // SAFELOG(INFO) << absl::StrFormat("candid lo %d hi %d k %d", c_0, c_1, k);

  const bool valid_low_0 = IsValidLowColor(c_0);
  const bool valid_low_1 = IsValidLowColor(c_1);
  const bool valid_high_0 = IsValidHighColor(c_0);
  const bool valid_high_1 = IsValidHighColor(c_1);

  const bool low_0_high_1 = valid_low_0 && valid_high_1;
  const bool low_1_high_0 = valid_low_1 && valid_high_0;
  if (!(low_0_high_1 || low_1_high_0)) return std::nullopt;

  uint8_t c_low;
  uint8_t c_high;
  size_t phase;
  if (low_0_high_1) {
    c_low = c_0;
    c_high = c_1;
    phase = (kPeriod + (kPeriod / 2) - k) % kPeriod;
    // SAFELOG(INFO) << "LowHigh";
  } else {
    c_low = c_1;
    c_high = c_0;
    phase = kPeriod - k;
    // SAFELOG(INFO) << "HighLow";
  }

  CandidateColors c;
  c.identity = Crt(c_low, c_high);
  c.phase = phase;
  return c;
}

size_t TwoColor::Crt(uint8_t c_low, uint8_t c_high) {
  // The dumbest possible implementation.
  for (size_t k = 1; k < kLowPrime * kHighPrime; k++) {
    if (LowColor(k) == c_low && HighColor(k) == c_high) return k;
  }
  return 0;  // Must be zero
}

std::optional<TwoColor::Identity> TwoColor::Identify(const uint8_t *buffer,
                                                     size_t length) {
  auto c = Candidates(buffer, length);
  if (!c.has_value()) return std::nullopt;
  // SAFELOG(INFO) << absl::StrFormat("Id %d ph %d", c->identity, c->phase);
  Identity ident;
  ident.phase = c->phase;
  ident.identity = c->identity;
  ident.length = ColorMatch(ident.identity, ident.phase, buffer, length);
  if (ident.length <= kPeriod) return std::nullopt;
  return ident;
}

std::string TwoColor::Identity::ToString() const {
  return absl::StrFormat("Identity: %d Length: %d Phase: %d", identity, length,
                         phase);
}
}  // namespace gvisor

