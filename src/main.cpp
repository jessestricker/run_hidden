#include <optional>
#include <string>
#include <string_view>

#include "win32.hpp"

/*
 * Declare and define the TraceLogging provider.
 * The GUID is derived from the provider name using ETW name-hashing.
 */
TRACELOGGING_DECLARE_PROVIDER(logging_provider);
TRACELOGGING_DEFINE_PROVIDER(logging_provider, "JesseStricker.run_hidden",
                             (0x23c4c550, 0x884e, 0x5fe9, 0x7e, 0x82, 0x3a,
                              0xc5, 0xf2, 0xa8, 0x52, 0x60));

namespace run_hidden {
  /// Wait for a process to end and log the exit code, if available.
  std::optional<int> wait_for_process(HANDLE process) {
    if (WaitForSingleObject(process, INFINITE) == WAIT_FAILED) {
      const auto error_code = GetLastError();
      TraceLoggingWrite(logging_provider, "WaitForSingleObjectFailed",
                        TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                        TraceLoggingWinError(error_code));
      return std::nullopt;
    }

    // get exit code
    auto exit_code = DWORD{};
    if (!GetExitCodeProcess(process, &exit_code)) {
      const auto error_code = GetLastError();
      TraceLoggingWrite(logging_provider, "WaitForSingleObjectFailed",
                        TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                        TraceLoggingWinError(error_code));
      TraceLoggingWrite(logging_provider, "ProcessTerminated",
                        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
      return std::nullopt;
    }
    if (exit_code == STILL_ACTIVE) {
      // unreachable, since we already waited for the process to terminate
      TraceLoggingWrite(
          logging_provider, "InvalidReturnValue",
          TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
          TraceLoggingValue("GetExitCodeProcess", "FunctionName"));
      TraceLoggingWrite(logging_provider, "ProcessTerminated",
                        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
      return std::nullopt;
    }

    // log with exit code
    TraceLoggingWrite(logging_provider, "ProcessTerminated",
                      TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                      TraceLoggingValue(exit_code, "ExitCode"));
    return {static_cast<int>(exit_code)};
  }

  /// Start a new process with the given command line and wait until it
  /// terminates.
  int run_process(std::wstring_view cmd_line) {
    // create a modifiable, null-terminated copy of the command line
    auto cmd_line_copy = std::wstring{cmd_line};
    cmd_line_copy.push_back('\0');

    // create process
    auto startup_info = STARTUPINFO{.cb = sizeof(STARTUPINFO)};
    auto process_info = PROCESS_INFORMATION{};
    if (!CreateProcess(nullptr,               // application name
                       cmd_line_copy.data(),  // command line
                       nullptr,               // process attributes
                       nullptr,               // thread attributes
                       false,                 // inherit handles
                       CREATE_NO_WINDOW,      // creation flags
                       nullptr,               // environment
                       nullptr,               // current directory
                       &startup_info, &process_info)) {
      const auto error_code = GetLastError();
      TraceLoggingWrite(logging_provider, "CreateProcessFailed",
                        TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                        TraceLoggingWinError(error_code));
      return 1;
    }

    TraceLoggingWrite(logging_provider, "ProcessCreated",
                      TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                      TraceLoggingWideString(cmd_line.data(), "CommandLine"));

    // wait until process terminates
    const auto exit_code = wait_for_process(process_info.hProcess);

    // close process handles
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);

    return exit_code.value_or(1);
  }

  int main(std::wstring_view cmd_line) {
    TraceLoggingRegister(logging_provider);
    const auto exit_code = run_process(cmd_line);
    TraceLoggingUnregister(logging_provider);
    return exit_code;
  }
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance,
                    wchar_t* cmd_line, int cmd_show) {
  return run_hidden::main(cmd_line);
}
