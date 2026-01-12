# PlayTorrio Player

PlayTorrio Player is a Cross-Platform Desktop Media Player, built on top of [IMPlay](https://github.com/tsl0922/ImPlay) [mpv](https://mpv.io) and [ImGui](https://github.com/ocornut/imgui).

PlayTorrio Player aims to be mpv compatible, which means almost all mpv features from the [manual](https://mpv.io/manual) are (or will be) available.

# Features

- Modern purple-themed UI with sleek player overlay controls
- Upload Subtitles button for easy subtitle loading
- External subtitle providers via command line
- Highly compatible with mpv
  - GPU Video Decoding
  - High Quality Video Output
  - [Lua](https://mpv.io/manual/stable/#lua-scripting) and [Javascript](https://mpv.io/manual/stable/#javascript) Scripting
  - [User Scripts](https://github.com/mpv-player/mpv/wiki/User-Scripts) and [Config Files](https://mpv.io/manual/stable/#configuration-files)
  - [Command Line](https://mpv.io/manual/stable/#usage) Interface
  - [Keyboard / Mouse](https://mpv.io/manual/stable/#interactive-control) Control
- Cross platform: Windows, Linux, macOS

# Command Line Usage

## Basic Usage

```bash
# Play a local file
playtp video.mp4

# Play a URL
playtp "https://example.com/video.m3u8"

# Play fullscreen
playtp --fs video.mp4
```

## External Subtitle Providers

You can pass external subtitles from different providers (like OpenSubtitles, Wyzie, etc.) via command line. These subtitles appear in the UI organized by provider tabs, and are only loaded when you select them.

**Format:**
```bash
playtp "media_url" ProviderName "SubtitleName1" "SubtitleURL1" "SubtitleName2" "SubtitleURL2" ...
```

**Single Provider Example:**
```bash
playtp "https://stream.example.com/movie.m3u8" OpenSubtitles "English" "https://subs.com/en.srt" "Spanish" "https://subs.com/es.srt"
```

**Multiple Providers Example:**
```bash
playtp "https://stream.example.com/movie.m3u8" \
  OpenSubtitles "English" "https://opensubs.com/en.srt" "French" "https://opensubs.com/fr.srt" \
  Wyzie "English HD" "https://wyzie.com/en-hd.srt" "English SDH" "https://wyzie.com/en-sdh.srt"
```

**How it works:**
- Provider names (e.g., `OpenSubtitles`, `Wyzie`) create tabs in the subtitle menu
- Subtitles are passed as name/URL pairs after each provider name
- Subtitles are NOT loaded automatically - they appear in the menu for you to select
- When you click a subtitle in the menu, it gets loaded
- The "Built-in" tab always shows embedded subtitles from the video file

## All Options

```bash
playtp [options] [url|path] [provider "subname" "suburl" ...]

Options:
  --start=<time>    Seek to position (percent, seconds, or hh:mm:ss)
  --no-audio        Disable audio
  --no-video        Disable video  
  --fs              Fullscreen playback
  --sub-file=<file> Load subtitle file
  --playlist=<file> Load playlist file
  --help            Show help
```

See [mpv manual](https://mpv.io/manual/stable) for all available options.

# Installation

- Binary version: download from the [Releases](https://github.com/tsl0922/ImPlay/releases) page
- Build from source: check the [Compiling](https://github.com/tsl0922/ImPlay/wiki/Compiling) document

Read the [FAQ](https://github.com/tsl0922/ImPlay/wiki/FAQ).
# Credits

ImPlay uses the following projects, thanks to their authors and contributors.

- [mpv](https://mpv.io): Command line video player
- [imgui](https://github.com/ocornut/imgui): Bloat-free Graphical User interface for C++ with minimal dependencies
  - [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h): public domain image loader
- [glfw](https://www.glfw.org): an Open Source, multi-platform library for OpenGL, OpenGL ES and Vulkan development on the desktop
- [glad](https://glad.dav1d.de): Multi-Language GL/GLES/EGL/GLX/WGL Loader-Generator based on the official specs
- [fmt](https://fmt.dev): A modern formatting library
- [json](https://json.nlohmann.me): JSON for Modern C++
- [inipp](https://github.com/mcmtroffaes/inipp): Simple C++ ini parser
- [libromfs](https://github.com/WerWolv/libromfs): Simple library for embedding static resources into C++ binaries using CMake
- [nativefiledialog](https://github.com/btzy/nativefiledialog-extended): Cross platform (Windows, Mac, Linux) native file dialog library
- [Cascadia Code](https://github.com/microsoft/cascadia-code) / [Font Awesome](https://fontawesome.com) / [Unifont](https://unifoundry.com/unifont.html): Fonts embeded in ImPlay

# License

[GPLv2](LICENSE.txt).
