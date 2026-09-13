// Copyright (c) 2026, WH, All rights reserved.
#include "Paths.h"

#include "BaseEnvironment.h"
#include "EngineConfig.h"
#include "Environment.h"
#include "File.h"
#include "LaunchArgs.h"
#include "Logging.h"
#include "UniString.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>

#include <filesystem>
#include <memory>

namespace Mc::Paths {

namespace {
struct Dirs {
    std::string exe_dir, assets, fonts, materials, libs;
    std::string data, cfg, maps, skins, replays, screenshots, exports, db, cache, logs;
} s_dirs;

std::string strip_trailing_slashes(std::string_view path) {
    while(path.size() > 1 && (path.ends_with('/') || path.ends_with('\\'))) path.remove_suffix(1);
    return std::string{path};
}
}  // namespace

namespace detail {
void init(std::string_view exe_path) {
    // fix the working directory in case the user launched us from the wrong folder, so that relative paths resolve
    // next to the executable. only done while MCENGINE_DATA_DIR is at its default, since a packager who changed it
    // clearly wants the executable somewhere else
    if constexpr(!Env::cfg(OS::WASM) && MCENGINE_DATA_DIR[0] == '.' && MCENGINE_DATA_DIR[1] == '/') {
        namespace fs = std::filesystem;
        fs::path exe_fspath;
        if constexpr(Env::cfg(OS::WINDOWS)) {
            exe_fspath = UniString::to_wide(exe_path);
        } else {
            exe_fspath = exe_path;
        }

        bool failed = true;
        std::error_code ec;
        if(!exe_fspath.empty() && exe_fspath.has_parent_path()) {
            fs::current_path(exe_fspath.parent_path(), ec);
            failed = !!ec;
        }
        if(failed) {
            debugLog("WARNING: failed to set working directory to parent of {}", exe_path);
        }
    }

    const auto sep = exe_path.find_last_of("/\\");
    s_dirs.exe_dir = sep == std::string_view::npos ? "." : std::string{exe_path.substr(0, sep)};
    if(s_dirs.exe_dir.empty()) s_dirs.exe_dir = "/";

    // build-time defaults: assets and data both live next to the executable (which is the working directory
    // after setcwdexe()), or wherever the packager pointed MCENGINE_DATA_DIR/APP_DATA_DIR at
    s_dirs.assets = strip_trailing_slashes(MCENGINE_DATA_DIR);
    s_dirs.libs = s_dirs.assets + "/lib";
    s_dirs.data = strip_trailing_slashes(APP_DATA_DIR);
    std::string cache;  // empty = derived below
    std::string logs;

    if constexpr(Env::cfg(OS::MAC)) {
        // inside an app bundle the executable directory is sealed by the code signature, so assets come from
        // Contents/Resources, dynamic libraries from Contents/Frameworks, and everything writable goes to the
        // usual per-user locations
        constexpr std::string_view exe_subdir = "/Contents/MacOS";
        if(s_dirs.exe_dir.ends_with(exe_subdir)) {
            const std::string contents = s_dirs.exe_dir.substr(0, s_dirs.exe_dir.size() - sizeof("/MacOS") + 1);
            s_dirs.assets = contents + "/Resources";
            s_dirs.libs = contents + "/Frameworks";

            // ~/Library/Application Support/neomod
            if(std::unique_ptr<char[], decltype(&SDL_free)> pref{SDL_GetPrefPath("", PACKAGE_NAME), &SDL_free}) {
                s_dirs.data = strip_trailing_slashes(pref.get());
            }
            if(const std::string home = Environment::getEnvVariable("HOME"); !home.empty()) {
                if(s_dirs.data.empty()) s_dirs.data = home + "/Library/Application Support/" PACKAGE_NAME;
                cache = home + "/Library/Caches/" PACKAGE_NAME;
                logs = home + "/Library/Logs/" PACKAGE_NAME;
            }
        }
    }

    // -datadir moves all writable data (including cache and logs) somewhere else, e.g. for tests
    if(const auto datadir = LaunchArgs::has_arg(LaunchArgs::MISC_DATA_DIR); datadir && !datadir->empty()) {
        s_dirs.data = strip_trailing_slashes(*datadir);
        File::normalizeSlashes(s_dirs.data, '\\', '/');
        cache = s_dirs.data + "/cache";
        logs = s_dirs.data + "/logs";
    }

    s_dirs.fonts = s_dirs.assets + "/fonts";
    s_dirs.materials = s_dirs.assets + "/materials";

    s_dirs.cfg = s_dirs.data + "/cfg";
    s_dirs.maps = s_dirs.data + "/maps";
    s_dirs.skins = s_dirs.data + "/skins";
    s_dirs.replays = s_dirs.data + "/replays";
    s_dirs.screenshots = s_dirs.data + "/screenshots";
    s_dirs.exports = s_dirs.data + "/exports";
    s_dirs.db = s_dirs.data;

    if(cache.empty()) {
        cache = [&]() -> std::string {
            if constexpr(Env::cfg(OS::WASM)) return "/persist/cache";
            if constexpr(Env::cfg(OS::LINUX) || Env::cfg(OS::MAC)) {
                // $XDG_CACHE_HOME/neomod, else $HOME/.cache/neomod
                if(const std::string xdg_cache_home = Environment::getEnvVariable("XDG_CACHE_HOME");
                   !xdg_cache_home.empty()) {
                    return xdg_cache_home + "/" PACKAGE_NAME;
                }
                if(const std::string home = Environment::getEnvVariable("HOME"); !home.empty()) {
                    return home + "/.cache/" PACKAGE_NAME;
                }
            }
            return s_dirs.data + "/cache";
        }();
    }
    s_dirs.cache = std::move(cache);

    // logs stay in the non-persistent fs on wasm
    if(logs.empty()) logs = (Env::cfg(OS::WASM) ? s_dirs.assets : s_dirs.data) + "/logs";
    s_dirs.logs = std::move(logs);
}
}  // namespace detail

const std::string &exe_dir() { return s_dirs.exe_dir; }
const std::string &assets() { return s_dirs.assets; }
const std::string &fonts() { return s_dirs.fonts; }
const std::string &materials() { return s_dirs.materials; }
const std::string &libs() { return s_dirs.libs; }

const std::string &data() { return s_dirs.data; }
const std::string &cfg() { return s_dirs.cfg; }
const std::string &maps() { return s_dirs.maps; }
const std::string &skins() { return s_dirs.skins; }
const std::string &replays() { return s_dirs.replays; }
const std::string &screenshots() { return s_dirs.screenshots; }
const std::string &exports() { return s_dirs.exports; }
const std::string &db() { return s_dirs.db; }
const std::string &cache() { return s_dirs.cache; }
const std::string &logs() { return s_dirs.logs; }

}  // namespace Mc::Paths
