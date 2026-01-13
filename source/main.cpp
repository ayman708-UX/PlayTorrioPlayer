// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <cstring>
#include <stdexcept>
#include <fstream>
#include <chrono>
#include <ctime>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif
#include <nlohmann/json.hpp>
#include "helpers/utils.h"
#include "window.h"

// Simple file logger for crash debugging - writes to exe directory
std::ofstream g_logFile;  // Made non-static so other files can use it

static void initLog() {
  // Write log file next to the executable
#ifdef _WIN32
  wchar_t exePath[MAX_PATH];
  GetModuleFileNameW(NULL, exePath, MAX_PATH);
  std::filesystem::path logPath = std::filesystem::path(exePath).parent_path() / "crash_debug.log";
#else
  std::filesystem::path logPath = "crash_debug.log";
#endif
  
  g_logFile.open(logPath, std::ios::out | std::ios::trunc);
  if (g_logFile.is_open()) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    g_logFile << "=== PlayTorrioPlayer Crash Debug Log ===" << std::endl;
    g_logFile << "Started: " << std::ctime(&time);
    g_logFile << "Log file: " << logPath.string() << std::endl;
    g_logFile << "========================================" << std::endl;
    g_logFile.flush();
  }
}

static void log(const char* msg) {
  if (g_logFile.is_open()) {
    g_logFile << "[LOG] " << msg << std::endl;
    g_logFile.flush();  // Flush immediately so we don't lose logs on crash
  }
  fmt::print("[LOG] {}\n", msg);
}

static void log(const std::string& msg) {
  log(msg.c_str());
}

// Log with format support
template<typename... Args>
static void logf(const char* format, Args&&... args) {
  std::string msg = fmt::format(format, std::forward<Args>(args)...);
  if (g_logFile.is_open()) {
    g_logFile << msg << std::endl;
    g_logFile.flush();
  }
  fmt::print("{}\n", msg);
}

static const char* usage =
    "Usage:   playtp [options] [url|path/]filename [provider \"subname\" \"suburl\" ...]\n"
    "\n"
    "Examples:\n"
    " playtp video.mp4                    play a local file\n"
    " playtp https://example.com/v        play a URL\n"
    " playtp --fs video.mp4               play fullscreen\n"
    "\n"
    "External Subtitles:\n"
    " playtp \"streamurl\" OpenSubs \"English\" \"http://sub1.srt\" \"Spanish\" \"http://sub2.srt\"\n"
    " playtp \"streamurl\" Provider1 \"Sub1\" \"url1\" Provider2 \"Sub2\" \"url2\" \"Sub3\" \"url3\"\n"
    "\n"
    " Format: playtp \"media\" ProviderName \"SubName1\" \"SubURL1\" \"SubName2\" \"SubURL2\" ...\n"
    " - Provider names group subtitles into tabs in the UI\n"
    " - Subtitles are passed as name/URL pairs after each provider\n"
    " - Multiple providers can be specified one after another\n"
    " - Subtitles are NOT loaded automatically, only shown in the menu\n"
    "\n"
    "Basic options:\n"
    " --start=<time>    seek to given (percent, seconds, or hh:mm:ss) position\n"
    " --no-audio        do not play sound\n"
    " --no-video        do not play video\n"
    " --fs              fullscreen playback\n"
    " --sub-file=<file> specify subtitle file to use\n"
    " --playlist=<file> specify playlist file\n"
    "\n"
    "Visit https://mpv.io/manual/stable to get full mpv options.\n";

static int run_headless(ImPlay::OptionParser& parser) {
  mpv_handle* ctx = mpv_create();
  if (!ctx) throw std::runtime_error("could not create mpv handle");

  for (const auto& [key, value] : parser.options) {
    if (int err = mpv_set_option_string(ctx, key.c_str(), value.c_str()); err < 0) {
      fmt::print(fg(fmt::color::red), "mpv: {} [{}={}]\n", mpv_error_string(err), key, value);
      return 1;
    }
  }
  if (mpv_initialize(ctx) < 0) throw std::runtime_error("could not initialize mpv context");

  for (auto& path : parser.paths) {
    const char* cmd[] = {"loadfile", path.c_str(), "append-play", NULL};
    mpv_command(ctx, cmd);
  }

  while (ctx) {
    mpv_event* event = mpv_wait_event(ctx, -1);
    if (event->event_id == MPV_EVENT_SHUTDOWN) break;
  }

  mpv_terminate_destroy(ctx);

  return 0;
}

static std::string build_ipc_cmd(std::string path) {
  nlohmann::json j = {{"command", {"loadfile", path, "append-play"}}};
  return fmt::format("{}\n", j.dump());
}

#ifdef _WIN32
static bool send_ipc(std::string sock, std::vector<std::string> paths) {
  HANDLE hPipe = CreateFile(sock.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
  if (hPipe == INVALID_HANDLE_VALUE) return false;

  for (auto& path : paths) {
    std::string payload = build_ipc_cmd(path);
    if (!WriteFile(hPipe, payload.c_str(), payload.size(), NULL, NULL)) {
      fmt::print("WriteFile failed: {}, payload: {}\n", GetLastError(), payload);
    }
  }

  CloseHandle(hPipe);
  return true;
}
#else
static bool send_ipc(std::string sock, std::vector<std::string> paths) {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd == -1) return false;

  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, sock.c_str(), sizeof(addr.sun_path) - 1);

  if ((connect(fd, (struct sockaddr*)&addr, sizeof(addr))) == -1) {
    close(fd);
    return false;
  }

  for (auto& path : paths) {
    std::string payload = build_ipc_cmd(path);
    if (write(fd, payload.c_str(), payload.size()) == -1) {
      fmt::print("write failed: {}, payload: {}\n", errno, payload);
    }
  }

  close(fd);
  return true;
}
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
  // Always allocate console for debugging FIRST
  AllocConsole();
  freopen("CONOUT$", "w", stdout);
  freopen("CONOUT$", "w", stderr);
  freopen("CONIN$", "r", stdin);
  fmt::print("[EARLY] Console allocated\n");
#endif

  fmt::print("[EARLY] main() starting, argc={}\n", argc);
  
  initLog();
  log("PlayTorrioPlayer starting...");

  fmt::print("[EARLY] About to parse args\n");
  ImPlay::OptionParser parser;
  log("Parsing command line arguments...");
  parser.parse(argc, argv);
  fmt::print("[EARLY] Args parsed\n");
  
  if (parser.options.contains("help")) {
    fmt::print("{}", usage);
    return 0;
  }

  try {
    log("Checking for headless mode...");
    if (parser.options.contains("o") || parser.check("video", "no") || parser.check("vid", "no")) {
      log("Running in headless mode");
      return run_headless(parser);
    }

    fmt::print("[EARLY] About to load config\n");
    log("Loading config...");
    ImPlay::Config config;
    config.load();
    log("Config loaded successfully");
    fmt::print("[EARLY] Config loaded, theme={}\n", config.Data.Interface.Theme);

    if (config.Data.Window.Single && send_ipc(config.ipcSocket(), parser.paths)) {
      log("Sent to existing instance via IPC");
      return 0;
    }

    fmt::print("[EARLY] About to create Window object\n");
    log("Creating window...");
    ImPlay::Window window(&config);
    log("Window object created");
    fmt::print("[EARLY] Window object created\n");
    
    fmt::print("[EARLY] About to call window.init()\n");
    log("Initializing window...");
    if (!window.init(parser)) {
      log("ERROR: Window initialization failed!");
      return 1;
    }
    log("Window initialized successfully");
    fmt::print("[EARLY] Window initialized\n");

    fmt::print("[EARLY] About to start main loop\n");
    log("Starting main loop...");
    window.run();
    
    log("Exiting normally");
    return 0;
  } catch (const std::exception& e) {
    std::string errMsg = fmt::format("EXCEPTION: {}", e.what());
    log(errMsg);
    fmt::print(fg(fmt::color::red), "Error: {}\n", e.what());
    
#ifdef _WIN32
    MessageBoxA(NULL, e.what(), "PlayTorrioPlayer Error", MB_OK | MB_ICONERROR);
#endif
    return 1;
  } catch (...) {
    log("UNKNOWN EXCEPTION!");
    fmt::print(fg(fmt::color::red), "UNKNOWN EXCEPTION!\n");
#ifdef _WIN32
    MessageBoxA(NULL, "Unknown error occurred", "PlayTorrioPlayer Error", MB_OK | MB_ICONERROR);
#endif
    return 1;
  }
}