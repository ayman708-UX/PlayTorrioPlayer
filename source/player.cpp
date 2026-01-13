// Copyright (c) 2022-2025 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <filesystem>
#include <fstream>
#include <thread>
#include <romfs/romfs.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_opengl3.h>
#include <fonts/cascadia.h>
#include <fonts/fontawesome.h>
#include <fonts/unifont.h>
#include <strnatcmp.h>
#include "theme.h"
#include "player.h"

// External log file from main.cpp
extern std::ofstream g_logFile;

// Helper to log to both file and console
#define LOGF(...) do { \
  std::string _msg = fmt::format(__VA_ARGS__); \
  if (g_logFile.is_open()) { g_logFile << _msg << std::endl; g_logFile.flush(); } \
  fmt::print("{}\n", _msg); \
} while(0)

namespace ImPlay {
Player::Player(Config *config) : config(config) {
  mpv = new Mpv();
  debug = new Views::Debug(config, mpv);
  playerOverlay = new Views::PlayerOverlay(config, mpv);
}

Player::~Player() {
  delete debug;
  delete playerOverlay;
  delete mpv;
}

bool Player::init(std::map<std::string, std::string> &options) {
  LOGF("[LOG] Player::init() starting...");
  
  mpv->option("config", "yes");
  mpv->option("input-default-bindings", "yes");
  mpv->option("input-vo-keyboard", "yes");
  
  // PlayTorrioPlayer: COMPLETELY disable ALL mpv on-screen UI
  mpv->option("osc", "no");
  mpv->option("osd-level", "0");
  mpv->option("osd-bar", "no");
  mpv->option("osd-playing-msg", "");
  mpv->option("osd-on-seek", "no");
  mpv->option("load-osd-console", "no");
  mpv->option("load-scripts", "no");
  
  LOGF("[LOG] MPV basic options set");
  
  // === ZERO-COPY HARDWARE INTEROP SETTINGS ===
  // Video output: libmpv for direct GPU rendering
  mpv->option("vo", "libmpv");
  
  // Hardware decoding: auto-safe tries hardware first, falls back gracefully
  mpv->option("hwdec", "auto-safe");
  
  // GPU API: let mpv choose the best available
  mpv->option("gpu-api", "auto");
  
  // Video sync: display-resample for smoothest playback synced to display
  mpv->option("video-sync", "display-resample");
  
  // Interpolation for smooth motion (optional, can be disabled for lower latency)
  mpv->option("interpolation", "yes");
  mpv->option("tscale", "oversample");
  
  // Let mpv handle swap timing
  mpv->option("opengl-swapinterval", "1");
  
  // Frame timing
  mpv->option("video-timing-offset", "0");
  
  // Disable expensive post-processing for performance
  mpv->option("deband", "no");
  mpv->option("dither-depth", "no");
  mpv->option("correct-downscaling", "no");
  
  // Fast scalers
  mpv->option("scale", "bilinear");
  mpv->option("dscale", "bilinear");
  mpv->option("cscale", "bilinear");
  
  // Video decoding optimizations
  mpv->option("vd-lavc-fast", "yes");
  mpv->option("vd-lavc-threads", "0");
  
  // Demuxer/cache settings for smooth streaming
  mpv->option("demuxer-max-bytes", "150MiB");
  mpv->option("demuxer-max-back-bytes", "50MiB");
  mpv->option("demuxer-readahead-secs", "20");
  mpv->option("cache", "yes");
  mpv->option("cache-secs", "120");
  mpv->option("cache-pause-initial", "yes");
  mpv->option("cache-pause-wait", "3");
  
  // Seeking optimizations
  mpv->option("hr-seek-framedrop", "yes");
  
  mpv->option("screenshot-directory", "~~desktop/");

  LOGF("[LOG] MPV video options set");

  // Display refresh rate for video-sync
  mpv->option<int64_t, MPV_FORMAT_INT64>("override-display-fps", GetMonitorRefreshRate());
  mpv->option<int64_t, MPV_FORMAT_INT64>("display-fps-override", GetMonitorRefreshRate());

  if (!config->Data.Mpv.UseConfig) {
    LOGF("[LOG] Writing MPV config...");
    writeMpvConf();
    mpv->option("config-dir", config->dir().c_str());
  }

  if (config->Data.Window.Single) mpv->option("input-ipc-server", config->ipcSocket().c_str());

  for (const auto &[key, value] : options) {
    if (int err = mpv->option(key.c_str(), value.c_str()); err < 0) {
      fmt::print(fg(fmt::color::red), "mpv: {} [{}={}]\n", mpv_error_string(err), key, value);
      return false;
    }
  }

  LOGF("[LOG] Initializing debug view...");
  debug->init();

  {
    LOGF("[LOG] Loading logo texture and initializing MPV...");
    ContextGuard guard(this);
    logoTexture = ImGui::LoadTexture("icon.png");
    LOGF("[LOG] Logo texture: {}\n", logoTexture);
    mpv->init(GetGLAddrFunc(), GetWid());
    LOGF("[LOG] MPV initialized");
  }

  SetWindowDecorated(mpv->property<int, MPV_FORMAT_FLAG>("border"));
  mpv->property<int64_t, MPV_FORMAT_INT64>("volume", config->Data.Mpv.Volume);
  if (config->Data.Recent.SpaceToPlayLast) mpv->command("keybind SPACE 'script-message-to implay play-pause'");
  
  LOGF("[LOG] Initializing observers...");
  initObservers();
  LOGF("[LOG] Player::init() complete");

  return true;
}

void Player::setExternalSubtitleProviders(const std::vector<CmdSubtitleProvider>& providers) {
  std::vector<Views::SubtitleProvider> overlayProviders;
  for (const auto& p : providers) {
    Views::SubtitleProvider sp;
    sp.name = p.name;
    for (const auto& s : p.subtitles) {
      Views::ExternalSubtitle es;
      es.name = s.name;
      es.url = s.url;
      sp.subtitles.push_back(es);
    }
    overlayProviders.push_back(sp);
  }
  playerOverlay->setExternalProviders(overlayProviders);
}

void Player::draw() {
  static bool firstDraw = true;
  if (firstDraw) LOGF("[LOG] draw() starting, idle={}\n", idle);
  
  drawVideo();
  if (firstDraw) LOGF("[LOG] draw: drawVideo() done");

  // Draw the PlayTorrioPlayer overlay
  if (!idle) {
    // Playing - show ONLY the PlayTorrioPlayer controls overlay
    // No old ImPlay UI elements during playback
    if (firstDraw) LOGF("[LOG] draw: calling playerOverlay->draw()");
    playerOverlay->draw();
  } else {
    // Idle - show PlayTorrioPlayer welcome screen
    if (firstDraw) LOGF("[LOG] draw: calling playerOverlay->drawIdleScreen()");
    playerOverlay->drawIdleScreen();
  }
  if (firstDraw) LOGF("[LOG] draw: overlay done");

  // Only draw dialogs (not the old UI views)
  drawOpenURL();
  drawDialog();
  
  if (firstDraw) {
    LOGF("[LOG] draw() complete");
    firstDraw = false;
  }
}

void Player::drawVideo() {
  auto vp = ImGui::GetMainViewport();
  auto drawList = ImGui::GetBackgroundDrawList(vp);

  if (!idle) {
    drawList->AddImage((ImTextureID)(intptr_t)tex, vp->WorkPos, vp->WorkPos + vp->WorkSize);
  } else if (logoTexture != 0 && !mpv->forceWindow) {
    const ImVec2 center = vp->GetWorkCenter();
    const ImVec2 delta(64, 64);
    drawList->AddImage(logoTexture, center - delta, center + delta);
  }
}

void Player::render() {
  static bool firstRender = true;
  static int renderCount = 0;
  renderCount++;
  
  if (firstRender) {
    LOGF("[LOG] Player::render() first call");
  }
  
  auto g = ImGui::GetCurrentContext();
  if (g != nullptr && g->WithinFrameScope) return;

  {
    if (firstRender) LOGF("[LOG] render: ContextGuard 1");
    ContextGuard guard(this);

    if (idle) {
      if (firstRender) LOGF("[LOG] render: clearing FBO (idle)");
      glBindFramebuffer(GL_FRAMEBUFFER, fbo);
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    if (config->FontReload) {
      loadFonts();
      config->FontReload = false;
    }
    if (firstRender) LOGF("[LOG] render: ImGui_ImplOpenGL3_NewFrame");
    ImGui_ImplOpenGL3_NewFrame();
  }

  if (firstRender) LOGF("[LOG] render: BackendNewFrame");
  BackendNewFrame();
  if (firstRender) LOGF("[LOG] render: ImGui::NewFrame");
  ImGui::NewFrame();

#if defined(_WIN32) && defined(IMGUI_HAS_VIEWPORT)
  if (config->Data.Mpv.UseWid) {
    ImGuiViewport *vp = ImGui::GetMainViewport();
    vp->Flags &= ~ImGuiViewportFlags_CanHostOtherWindows;
  }
#endif

  if (firstRender) LOGF("[LOG] render: calling draw()");
  draw();
  if (firstRender) LOGF("[LOG] render: draw() returned");

#if defined(_WIN32) && defined(IMGUI_HAS_VIEWPORT)
  if (config->Data.Mpv.UseWid && mpv->ontop) {
    ImGuiContext *ctx = ImGui::GetCurrentContext();
    for (int i = 1; i < ctx->Windows.Size; i++) {
      ImGuiWindow *w = ctx->Windows[i];
      if (w->Flags & ImGuiWindowFlags_Popup) {
        w->WindowClass.ViewportFlagsOverrideSet = ImGuiViewportFlags_TopMost;
      }
    }
  }
#endif

  if (firstRender) LOGF("[LOG] render: ImGui::Render");
  ImGui::Render();

  {
    if (firstRender) LOGF("[LOG] render: ContextGuard 2");
    ContextGuard guard(this);
    GetFramebufferSize(&width, &height);
    glViewport(0, 0, width, height);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    if (firstRender) LOGF("[LOG] render: ImGui_ImplOpenGL3_RenderDrawData");
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    SetSwapInterval(config->Data.Interface.Fps > 60 ? 0 : 1);
    if (firstRender) LOGF("[LOG] render: SwapBuffers");
    SwapBuffers();
    mpv->reportSwap();

#ifdef IMGUI_HAS_VIEWPORT
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      if (firstRender) LOGF("[LOG] render: UpdatePlatformWindows");
      ImGui::UpdatePlatformWindows();
      if (firstRender) LOGF("[LOG] render: RenderPlatformWindowsDefault");
      ImGui::RenderPlatformWindowsDefault();
      if (firstRender) LOGF("[LOG] render: Viewports done");
    }
#endif
  }
  
  if (firstRender) {
    LOGF("[LOG] render: first frame complete!");
    firstRender = false;
  }
}

void Player::renderVideo() {
  ContextGuard guard(this);

  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glBindTexture(GL_TEXTURE_2D, tex);

  // Use GL_RGBA like the working version (not GL_RGBA8)
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  mpv->render(width, height, fbo, false);
}

void Player::initGui() {
  LOGF("[LOG] initGui() starting...");
  ContextGuard guard(this);

#ifdef IMGUI_IMPL_OPENGL_ES3
  LOGF("[LOG] Loading GLES2...");
  if (!gladLoadGLES2((GLADloadfunc)GetGLAddrFunc())) throw std::runtime_error("Failed to load GLES 2!");
#else
  LOGF("[LOG] Loading GL...");
  if (!gladLoadGL((GLADloadfunc)GetGLAddrFunc())) throw std::runtime_error("Failed to load GL!");
#endif
  LOGF("[LOG] GL loaded successfully");
  SetSwapInterval(1);  // Enable VSync

  LOGF("[LOG] Creating ImGui context...");
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  LOGF("[LOG] ImGui context created");

  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.ConfigWindowsMoveFromTitleBarOnly = true;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef IMGUI_HAS_DOCK
  if (config->Data.Interface.Docking) io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
#ifdef IMGUI_HAS_VIEWPORT
  if (config->Data.Interface.Viewports || config->Data.Mpv.UseWid) io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif

  LOGF("[LOG] Loading fonts...");
  loadFonts();
  LOGF("[LOG] Fonts loaded");

  // Create FBO for video rendering
  LOGF("[LOG] Creating FBO...");
  glGenFramebuffers(1, &fbo);
  glGenTextures(1, &tex);

  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glBindTexture(GL_TEXTURE_2D, tex);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  LOGF("[LOG] FBO created");

#ifdef IMGUI_IMPL_OPENGL_ES3
  LOGF("[LOG] Initializing ImGui OpenGL3 (ES3)...");
  ImGui_ImplOpenGL3_Init("#version 300 es");
#elif defined(__APPLE__)
  LOGF("[LOG] Initializing ImGui OpenGL3 (Apple)...");
  ImGui_ImplOpenGL3_Init("#version 150");
#else
  LOGF("[LOG] Initializing ImGui OpenGL3...");
  ImGui_ImplOpenGL3_Init("#version 130");
#endif
  LOGF("[LOG] initGui() complete");
}

void Player::exitGui() {
  MakeContextCurrent();

  ImGui_ImplOpenGL3_Shutdown();
  glDeleteTextures(1, &tex);
  glDeleteFramebuffers(1, &fbo);

  ImGui::DestroyContext();
}

void Player::saveState() {
  if (config->Data.Window.Save) {
    GetWindowPos(&config->Data.Window.X, &config->Data.Window.Y);
    GetWindowSize(&config->Data.Window.W, &config->Data.Window.H);
  }
  config->Data.Mpv.Volume = mpv->volume;
  config->save();
}

void Player::restoreState() {
  int mw, mh;
  GetMonitorSize(&mw, &mh);
  int w = std::max((int)(mw * 0.4), 600);
  int h = std::max((int)(mh * 0.4), 400);
  int x = (mw - w) / 2;
  int y = (mh - h) / 2;
  if (config->Data.Window.Save) {
    if (config->Data.Window.W > 0) w = config->Data.Window.W;
    if (config->Data.Window.H > 0) h = config->Data.Window.H;
    if (config->Data.Window.X >= 0) x = config->Data.Window.X;
    if (config->Data.Window.Y >= 0) y = config->Data.Window.Y;
  }
  SetWindowSize(w, h);
  SetWindowPos(x, y);
}

void Player::loadFonts() {
  LOGF("[LOG] loadFonts() starting...");
  
  auto interface = config->Data.Interface;
  float baseFontSize = config->Data.Font.Size;
  float scale = interface.Scale;
  
  if (scale == 0) {
    float xscale, yscale;
    GetWindowScale(&xscale, &yscale);
    scale = std::max(xscale, yscale);
  }
  if (scale <= 0) scale = 1.0f;
  
  // Larger base font for crisp rendering
  float fontSize = std::floor(std::max(baseFontSize, 16.0f) * scale);
  float iconSize = std::floor(fontSize * 1.1f);  // Icons slightly larger
  
  LOGF("[LOG] Font size: {}, Icon size: {}, Scale: {}\n", fontSize, iconSize, scale);

  ImGuiStyle style;
  LOGF("[LOG] Setting theme: {}\n", interface.Theme);
  ImGui::SetTheme(interface.Theme.c_str(), &style, interface.Rounding, interface.Shadow);

  ImGuiIO &io = ImGui::GetIO();
#if defined(_WIN32) && defined(IMGUI_HAS_VIEWPORT)
  if (config->Data.Mpv.UseWid) io.ConfigViewportsNoAutoMerge = true;
#endif

  style.ScaleAllSizes(scale);
  ImGui::GetStyle() = style;

  io.Fonts->Clear();
  LOGF("[LOG] Fonts cleared");

  // Font config for smooth rendering
  ImFontConfig cfg;
  cfg.SizePixels = fontSize;
  cfg.OversampleH = 2;  // Better horizontal antialiasing
  cfg.OversampleV = 2;  // Better vertical antialiasing
  cfg.PixelSnapH = false;  // Smoother subpixel positioning

  const ImWchar *font_range = config->buildGlyphRanges();
  LOGF("[LOG] Glyph ranges built");
  
  // Use Cascadia as primary font (modern, clean look)
  LOGF("[LOG] Loading Cascadia font (size={}, data={}, compressed_size={})...\n", 
             fontSize, (void*)cascadia_compressed_data, cascadia_compressed_size);
  auto* font1 = io.Fonts->AddFontFromMemoryCompressedTTF(cascadia_compressed_data, cascadia_compressed_size, fontSize, &cfg, font_range);
  if (font1 == nullptr) {
    fmt::print(fg(fmt::color::red), "[ERROR] Failed to load Cascadia font! Using default.\n");
    io.Fonts->AddFontDefault();
  } else {
    LOGF("[LOG] Cascadia font loaded successfully");
  }

  // Merge FontAwesome icons with larger size
  cfg.MergeMode = true;
  cfg.GlyphMinAdvanceX = iconSize;  // Consistent icon width
  static ImWchar fa_range[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
  LOGF("[LOG] Loading FontAwesome (size={})...\n", iconSize);
  auto* font2 = io.Fonts->AddFontFromMemoryCompressedTTF(fa_compressed_data, fa_compressed_size, iconSize, &cfg, fa_range);
  if (font2 == nullptr) {
    fmt::print(fg(fmt::color::red), "[ERROR] Failed to load FontAwesome!\n");
  } else {
    LOGF("[LOG] FontAwesome loaded successfully");
  }
  
  // Add unifont as fallback for international characters
  cfg.MergeMode = true;
  cfg.GlyphMinAdvanceX = 0;
  if (fileExists(config->Data.Font.Path)) {
    LOGF("[LOG] Loading custom font from: {}\n", config->Data.Font.Path);
    io.Fonts->AddFontFromFileTTF(config->Data.Font.Path.c_str(), fontSize, &cfg, font_range);
  } else {
    LOGF("[LOG] Loading unifont fallback...");
    io.Fonts->AddFontFromMemoryCompressedTTF(unifont_compressed_data, unifont_compressed_size, fontSize, &cfg, font_range);
  }
  
  // Build font atlas
  LOGF("[LOG] Building font atlas...");
  bool built = io.Fonts->Build();
  LOGF("[LOG] Font atlas build result: {}\n", built ? "SUCCESS" : "FAILED");
  LOGF("[LOG] loadFonts() complete");
}

void Player::shutdown() { mpv->command(config->Data.Mpv.WatchLater ? "quit-watch-later" : "quit"); }

void Player::onCursorEvent(double x, double y) {
  std::string xs = std::to_string((int)x);
  std::string ys = std::to_string((int)y);
  mpv->commandv("mouse", xs.c_str(), ys.c_str(), nullptr);
}

void Player::onScrollEvent(double x, double y) {
  if (abs(x) > 0) onKeyEvent(x > 0 ? "WHEEL_LEFT" : "WHEEL_RIGH");
  if (abs(y) > 0) onKeyEvent(y > 0 ? "WHEEL_UP" : "WHEEL_DOWN");
}

void Player::onKeyEvent(std::string name) { mpv->commandv("keypress", name.c_str(), nullptr); }

void Player::onKeyDownEvent(std::string name) { mpv->commandv("keydown", name.c_str(), nullptr); }

void Player::onKeyUpEvent(std::string name) { mpv->commandv("keyup", name.c_str(), nullptr); }

void Player::onDropEvent(int count, const char **paths) {
  std::sort(paths, paths + count, [](const auto &a, const auto &b) { return strnatcasecmp(a, b) < 0; });
  std::vector<std::filesystem::path> files;
  for (int i = 0; i < count; i++) files.emplace_back(reinterpret_cast<char8_t *>(const_cast<char *>(paths[i])));
  load(files);
}

void Player::updateWindowState() {
  int width = (int)mpv->property<int64_t, MPV_FORMAT_INT64>("dwidth");
  int height = (int)mpv->property<int64_t, MPV_FORMAT_INT64>("dheight");
  if (width > 0 && height > 0) {
    int x, y, w, h;
    GetWindowPos(&x, &y);
    GetWindowSize(&w, &h);
    if ((w != width || h != height) && mpv->autoResize) {
      SetWindowSize(width, height);
      SetWindowPos(x + (w - width) / 2, y + (h - height) / 2);
    }
    if (mpv->keepaspect && mpv->keepaspectWindow) SetWindowAspectRatio(width, height);
  }
}

void Player::initObservers() {
  mpv->observeEvent(MPV_EVENT_SHUTDOWN, [this](void *data) { SetWindowShouldClose(true); });

  mpv->observeEvent(MPV_EVENT_VIDEO_RECONFIG, [this](void *data) {
    if (!mpv->fullscreen) updateWindowState();
  });

  mpv->observeEvent(MPV_EVENT_FILE_LOADED, [this](void *data) {
    auto path = mpv->property("path");
    if (path != "" && path != "bd://" && path != "dvd://") config->addRecentFile(path, mpv->property("media-title"));
    mpv->property("force-media-title", "");
    mpv->property("start", "none");
  });

  mpv->observeEvent(MPV_EVENT_CLIENT_MESSAGE, [this](void *data) {
    auto msg = static_cast<mpv_event_client_message *>(data);
    execute(msg->num_args, msg->args);
  });

  mpv->observeProperty<int, MPV_FORMAT_FLAG>("idle-active", [this](int flag) {
    idle = static_cast<bool>(flag);
    if (idle) {
      SetWindowTitle(PLAYER_NAME);
      SetWindowAspectRatio(-1, -1);
    }
  });

  mpv->observeProperty<char *, MPV_FORMAT_STRING>("media-title", [this](char *data) { SetWindowTitle(data); });
  mpv->observeProperty<int, MPV_FORMAT_FLAG>("border", [this](int flag) { SetWindowDecorated(flag); });
  mpv->observeProperty<int, MPV_FORMAT_FLAG>("ontop", [this](int flag) { SetWindowFloating(flag); });
  mpv->observeProperty<int, MPV_FORMAT_FLAG>("window-maximized", [this](int flag) { SetWindowMaximized(flag); });
  mpv->observeProperty<int, MPV_FORMAT_FLAG>("window-minimized", [this](int flag) { SetWindowMinimized(flag); });
  mpv->observeProperty<double, MPV_FORMAT_DOUBLE>("window-scale", [this](double scale) {
    int w = (int)mpv->property<int64_t, MPV_FORMAT_INT64>("dwidth");
    int h = (int)mpv->property<int64_t, MPV_FORMAT_INT64>("dheight");
    if (w > 0 && h > 0) SetWindowSize((int)(w * scale), (int)(h * scale));
  });
  mpv->observeProperty<int, MPV_FORMAT_FLAG>("fullscreen", [this](int flag) { SetWindowFullscreen(flag); });
}

void Player::writeMpvConf() {
  auto path = dataPath();
  auto mpvConf = path / "mpv.conf";
  auto inputConf = path / "input.conf";

  if (!std::filesystem::exists(mpvConf)) {
    std::ofstream file(mpvConf, std::ios::binary);
    auto content = romfs::get("mpv/mpv.conf");
    file.write(reinterpret_cast<const char *>(content.data()), content.size()) << "\n";
    
    // Zero-copy hardware interop settings
    file << "# PlayTorrioPlayer - Optimized for performance\n";
    file << "profile=gpu-hq\n";
    file << "hwdec=auto-safe\n";
    file << "video-sync=display-resample\n";
    file << "interpolation=yes\n";
    file << "tscale=oversample\n\n";
    
    // Disable expensive effects
    file << "# Performance optimizations\n";
    file << "deband=no\n";
    file << "dither-depth=no\n\n";
    
    // Disable all mpv UI
    file << "# Disable mpv UI (PlayTorrioPlayer has its own)\n";
    file << "osc=no\n";
    file << "osd-level=0\n";
    file << "osd-bar=no\n";
  }

  if (!std::filesystem::exists(inputConf)) {
    std::ofstream file(inputConf, std::ios::binary);
    auto content = romfs::get("mpv/input.conf");
    file.write(reinterpret_cast<const char *>(content.data()), content.size()) << "\n";
    file << "`            script-message-to implay metrics\n";
  }
}

void Player::execute(int n_args, const char **args_) {
  if (n_args == 0) return;

  static std::map<std::string, std::function<void(int, const char **)>> commands = {
      {"open", [&](int n, const char **args) { openFilesDlg(mediaFilters); }},
      {"open-folder", [&](int n, const char **args) { openFolderDlg(); }},
      {"open-disk", [&](int n, const char **args) { openFolderDlg(false, true); }},
      {"open-iso", [&](int n, const char **args) { openFileDlg(isoFilters); }},
      {"open-clipboard", [&](int n, const char **args) { openClipboard(); }},
      {"open-url", [&](int n, const char **args) { openURL(); }},
      {"open-config-dir", [&](int n, const char **args) { openUrl(config->dir()); }},
      {"load-sub", [&](int n, const char **args) { openFilesDlg(subtitleFilters); }},
      {"playlist-add-files", [&](int n, const char **args) { openFilesDlg(mediaFilters, true); }},
      {"playlist-add-folder", [&](int n, const char **args) { openFolderDlg(true); }},
      {"playlist-sort", [&](int n, const char **args) { playlistSort(n > 0 && strcmp(args[0], "true") == 0); }},
      {"play-pause",
       [&](int n, const char **args) {
         auto count = mpv->property<int64_t, MPV_FORMAT_INT64>("playlist-count");
         if (count > 0)
           mpv->command("cycle pause");
         else if (config->getRecentFiles().size() > 0) {
           for (auto &file : config->getRecentFiles()) {
             if (fileExists(file.path) || file.path.find("://") != std::string::npos) {
               mpv->commandv("loadfile", file.path.c_str(), nullptr);
               mpv->commandv("set", "force-media-title", file.title.c_str(), nullptr);
               break;
             }
           }
         }
       }},
      {"metrics", [&](int n, const char **args) { debug->show(); }},
      {"show-message",
       [&](int n, const char **args) {
         if (n > 1) messageBox(args[0], args[1]);
       }},
  };

  const char *cmd = args_[0];
  auto it = commands.find(cmd);
  try {
    if (it != commands.end()) it->second(n_args - 1, args_ + 1);
  } catch (const std::exception &e) {
    messageBox("Error", fmt::format("{}: {}", cmd, e.what()));
  }
}

void Player::openFileDlg(NFD::Filters filters, bool append) {
  mpv->command("set pause yes");
  if (auto res = NFD::openFile(filters)) load({*res}, append);
  mpv->command("set pause no");
}

void Player::openFilesDlg(NFD::Filters filters, bool append) {
  mpv->command("set pause yes");
  if (auto res = NFD::openFiles(filters)) load(*res, append);
  mpv->command("set pause no");
}

void Player::openFolderDlg(bool append, bool disk) {
  mpv->command("set pause yes");
  if (auto res = NFD::openFolder()) load({*res}, append, disk);
  mpv->command("set pause no");
}

void Player::openClipboard() {
  auto content = GetClipboardString();
  if (content != "") {
    auto str = trim(content);
    mpv->commandv("loadfile", str.c_str(), nullptr);
    mpv->commandv("show-text", str.c_str(), nullptr);
  }
}

void Player::openURL() { m_openURL = true; }

void Player::openDvd(std::filesystem::path path) {
  mpv->property("dvd-device", path.string().c_str());
  mpv->commandv("loadfile", "dvd://", nullptr);
}

void Player::openBluray(std::filesystem::path path) {
  mpv->property("bluray-device", path.string().c_str());
  mpv->commandv("loadfile", "bd://", nullptr);
}

void Player::playlistSort(bool reverse) {
  if (mpv->playlist.empty()) return;
  std::vector<Mpv::PlayItem> items(mpv->playlist);
  std::sort(items.begin(), items.end(), [&](const auto &a, const auto &b) {
    std::string str1 = a.title != "" ? a.title : a.filename();
    std::string str2 = b.title != "" ? b.title : b.filename();
    return strnatcasecmp(str1.c_str(), str2.c_str()) < 0;
  });
  if (reverse) std::reverse(items.begin(), items.end());

  int64_t timePos = mpv->timePos;
  int64_t pos = -1;
  for (int i = 0; i < items.size(); i++) {
    if (items[i].id == mpv->playlistPos) {
      pos = i;
      break;
    }
  }
  std::vector<std::string> playlist = {"#EXTM3U"};
  for (auto &item : items) {
    if (item.title != "") playlist.push_back(fmt::format("#EXTINF:-1,{}", item.title));
    playlist.push_back(item.path.string());
  }
  mpv->property<int64_t, MPV_FORMAT_INT64>("playlist-start", pos);
  mpv->property("start", fmt::format("+{}", timePos).c_str());
  if (!mpv->playing()) mpv->command("playlist-clear");
  mpv->commandv("loadlist", fmt::format("memory://{}", join(playlist, "\n")).c_str(),
                mpv->playing() ? "replace" : "append", nullptr);
}

void Player::load(std::vector<std::filesystem::path> files, bool append, bool disk) {
  int i = 0;
  for (auto &file : files) {
    if (std::filesystem::is_directory(file)) {
      if (disk) {
        if (std::filesystem::exists(file / u8"BDMV"))
          openBluray(file);
        else
          openDvd(file);
        break;
      }
      for (const auto &entry : std::filesystem::recursive_directory_iterator(file)) {
        auto path = entry.path().string();
        if (isMediaFile(path)) {
          const char *action = append ? "append" : (i > 0 ? "append-play" : "replace");
          mpv->commandv("loadfile", path.c_str(), action, nullptr);
          i++;
        }
      }
    } else {
      if (file.extension() == ".iso") {
        if ((double)std::filesystem::file_size(file) / 1000 / 1000 / 1000 > 4.7)
          openBluray(file);
        else
          openDvd(file);
        break;
      } else if (isSubtitleFile(file.string())) {
        mpv->commandv("sub-add", file.string().c_str(), append ? "auto" : "select", nullptr);
      } else {
        const char *action = append ? "append" : (i > 0 ? "append-play" : "replace");
        mpv->commandv("loadfile", file.string().c_str(), action, nullptr);
      }
      i++;
    }
  }
}

void Player::drawOpenURL() {
  if (!m_openURL) return;
  ImGui::OpenPopup("views.dialog.open_url.title"_i18n);

  ImVec2 wSize = ImGui::GetMainViewport()->WorkSize;
  ImGui::SetNextWindowSize(ImVec2(std::min(wSize.x * 0.8f, scaled(50)), 0), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal("views.dialog.open_url.title"_i18n, &m_openURL)) {
    if (ImGui::IsKeyDown(ImGuiKey_Escape)) m_openURL = false;
    static char url[256] = {0};
    bool loadfile = false;
    if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputTextWithHint("##Input URL", "views.dialog.open_url.hint"_i18n, url, IM_ARRAYSIZE(url),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
      if (url[0] != '\0') loadfile = true;
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - scaled(10));
    if (ImGui::Button("views.dialog.open_url.cancel"_i18n, ImVec2(scaled(5), 0))) m_openURL = false;
    ImGui::SameLine();
    if (url[0] == '\0') ImGui::BeginDisabled();
    if (ImGui::Button("views.dialog.open_url.ok"_i18n, ImVec2(scaled(5), 0))) loadfile = true;
    if (url[0] == '\0') ImGui::EndDisabled();
    if (loadfile) {
      m_openURL = false;
      mpv->commandv("loadfile", url, nullptr);
    }
    if (!m_openURL) url[0] = '\0';
    ImGui::EndPopup();
  }
}

void Player::drawDialog() {
  if (!m_dialog) return;
  ImGui::OpenPopup(m_dialog_title.c_str());

  ImGui::SetNextWindowSize(ImVec2(scaled(30), 0), ImGuiCond_Always);
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal(m_dialog_title.c_str(), &m_dialog)) {
    ImGui::TextWrapped("%s", m_dialog_msg.c_str());
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - scaled(5));
    if (ImGui::Button("OK", ImVec2(scaled(5), 0))) m_dialog = false;
    ImGui::EndPopup();
  }
}

void Player::messageBox(std::string title, std::string msg) {
  m_dialog_title = title;
  m_dialog_msg = msg;
  m_dialog = true;
}

bool Player::isMediaFile(std::string file) {
  auto ext = std::filesystem::path(file).extension().string();
  if (ext.empty()) return false;
  if (ext[0] == '.') ext = ext.substr(1);
  if (std::find(videoTypes.begin(), videoTypes.end(), ext) != videoTypes.end()) return true;
  if (std::find(audioTypes.begin(), audioTypes.end(), ext) != audioTypes.end()) return true;
  if (std::find(imageTypes.begin(), imageTypes.end(), ext) != imageTypes.end()) return true;
  return false;
}

bool Player::isSubtitleFile(std::string file) {
  auto ext = std::filesystem::path(file).extension().string();
  if (ext.empty()) return false;
  if (ext[0] == '.') ext = ext.substr(1);
  if (std::find(subtitleTypes.begin(), subtitleTypes.end(), ext) != subtitleTypes.end()) return true;
  return false;
}
}  // namespace ImPlay