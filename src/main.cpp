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
  /// Start a new process with the given command line.
  /// This function does __not__ wait until the new process exists.
  void start_process(std::wstring_view cmd_line) {
    // create a modifiable, null-terminated copy of the command line
    auto cmd_line_copy = std::wstring{cmd_line};
    cmd_line_copy.push_back('\0');

    auto startup_info = STARTUPINFO{.cb = sizeof(STARTUPINFO)};
    auto process_info = PROCESS_INFORMATION{};
    const auto result = CreateProcess(nullptr,               // application name
                                      cmd_line_copy.data(),  // command line
                                      nullptr,           // process attributes
                                      nullptr,           // thread attributes
                                      false,             // inherit handles
                                      CREATE_NO_WINDOW,  // creation flags
                                      nullptr,           // environment
                                      nullptr,           // current directory
                                      &startup_info, &process_info);

    // handle process creation error
    if (result == 0) {
      const auto error_code = GetLastError();
      TraceLoggingWrite(logging_provider, "CreateProcessFailed",
                        TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                        TraceLoggingWinError(error_code));
      return;
    }

    TraceLoggingWrite(logging_provider, "ProcessCreated",
                      TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                      TraceLoggingWideString(cmd_line.data(), "CommandLine"));

    WaitForInputIdle(process_info.hProcess, INFINITE);
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);
  }

  void main(std::wstring_view cmd_line) {
    TraceLoggingRegister(logging_provider);

    start_process(cmd_line);

    TraceLoggingUnregister(logging_provider);
  }
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance,
                    wchar_t* cmd_line, int cmd_show) {
  run_hidden::main(cmd_line);
  return 0;
}
