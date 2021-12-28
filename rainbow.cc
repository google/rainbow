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
//
// ==========================================================================
//
// Exercises kernel memory management and data transfer from user space.
// Suitable for native and hypervisor environments.
//
// WARNING: This code forks. Be sparing with Google3 use.
// See go/no-single-threaded

#include <sys/wait.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "absl/debugging/failure_signal_handler.h"
#include "absl/debugging/symbolize.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"
#include "absl/time/clock.h"
#include "log.h"
#include "spraypaint.h"

ABSL_FLAG(int, kids, std::thread::hardware_concurrency(),
          "Number of sub processes");
ABSL_FLAG(absl::Duration, run_time, absl::InfiniteDuration(), "Run time");
ABSL_FLAG(size_t, buf_size, 5, "Size of data buffer");

namespace gvisor {
int Guts() {
  SprayPaint spray_paint(absl::GetFlag(FLAGS_buf_size));

  int round = 0;
  int failures = 0;
  absl::Time start = absl::Now();
  for (; absl::Now() - start < absl::GetFlag(FLAGS_run_time); round++) {
    // Fork children processes.
    std::vector<pid_t> pids;
    for (int kid = 1; kid <= absl::GetFlag(FLAGS_kids); kid++) {
      pid_t kid_pid = fork();
      if (kid_pid < 0) {
        SAFELOG(FATAL) << "Fork(" << kid << ") failed: " << strerror(errno);
      }
      if (kid_pid == 0) {
        // Child process.
        return spray_paint.Kid(round, kid);
      }
      pids.push_back(kid_pid);
    }

    // Reap children processes.
    for (int kid = 1; kid <= absl::GetFlag(FLAGS_kids); kid++) {
      int status = 0;
      pid_t kid_pid = pids[kid - 1];
      pid_t p = waitpid(kid_pid, &status, 0);
      if (p != kid_pid) {
        SAFELOG(ERROR) << "Huh! Round: " << round << " kid: " << kid
                       << " pid: " << kid_pid << " p: " << p;
        failures++;
      } else {
        if (WIFSIGNALED(status)) {
          SAFELOG(ERROR) << "Round: " << round << " kid: " << kid
                         << " pid: " << kid_pid
                         << " failed, signal: " << WTERMSIG(status);
          failures++;
        } else {
          if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status) {
              SAFELOG(ERROR)
                  << "Round: " << round << " kid: " << kid
                  << " pid: " << kid_pid
                  << " failed rc: " << exit_status;
              failures++;
            }
          } else {
            SAFELOG(ERROR) << "Round: " << round << " kid: " << kid
                           << " pid: " << kid_pid
                           << " stopped I don't know why.";
            failures++;
          }
        }
      }
    }
    SAFELOG_EVERY_N_SECS(INFO, 30)
        << "Completed round: " << round << " failures: " << failures;
  }

  if (!spray_paint.ColorIsRight("Dtor")) {
    SAFELOG(FATAL) << "Papa buffer corrupted at exit";
  }
  if (failures) {
    SAFELOG(ERROR) << "Completed round: " << round << " failures: " << failures;
    return 1;
  } else {
    SAFELOG(INFO) << "Completed round: " << round << " failures: " << failures;
    return 0;
  }
}
}  // namespace gvisor

int main(int argc, char **argv) {
  // Initialize the symbolizer to get a human-readable stack trace.
  absl::InitializeSymbolizer(argv[0]);
  // Install more informative than usual signal handlers, inherited too by
  // forked-off children.
  absl::FailureSignalHandlerOptions options;
  absl::InstallFailureSignalHandler(options);
  absl::ParseCommandLine(argc, argv);
  return gvisor::Guts();
}
