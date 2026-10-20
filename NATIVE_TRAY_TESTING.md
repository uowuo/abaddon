# Native Linux tray validation

## Scope

The native StatusNotifier path is optional at build time and still requires
`[gui] hide_to_tray=true` at runtime. The ordinary minimize button is unchanged.
Panel conventions determine whether left-click opens a menu; use **Show Abaddon**
to restore the application. No polling loop or helper process is introduced.

Related prior proposal: https://github.com/uowuo/abaddon/pull/315. This change is
an independent implementation against current master, including the CMake opt-out
requested there and handling for absent/lost tray hosts.

## Verified on 2026-10-20

Environment: Arch-based Linux, KDE Plasma 6.8, native Wayland, GTK 3,
Ayatana AppIndicator 0.6.1. Optimized `MinSizeRel` build with voice support enabled.

Nine functional checks passed using an isolated D-Bus session with a controllable
StatusNotifierWatcher and disposable configuration (`keychain=false`,
`autoconnect=false`, empty token). `DISPLAY` was unset and `GDK_BACKEND=wayland`.
Only the test client's windows were controlled through the compositor.

1. One native Wayland window and a registered StatusNotifier item at startup.
2. Closing with X hides the window without terminating the process.
3. **Show Abaddon** restores exactly one window.
4. **Hide Abaddon** hides the window without terminating the process.
5. Repeated Show requests do not create duplicate windows.
6. Losing the tray host while hidden automatically restores the window.
7. Closing after tray loss exits normally.
8. Starting without a tray host still permits normal exit with X.
9. **Quit** exits normally while the window is hidden.

The legacy `Gtk::StatusIcon` compilation path also passed a syntax-only C++17
check of `src/abaddon.cpp` with both AppIndicator definitions absent. This is not
a claim of a full legacy build or runtime test. `git diff --check` passed.

## Build configuration

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build --parallel 1
```

To explicitly retain the legacy icon:

```sh
cmake -S . -B build-legacy -DUSE_APPINDICATOR=OFF
```

The native build has one deprecation warning for the AppIndicator constructor
in Ayatana 0.6.1; it uses the common API also available in legacy libappindicator.
Windows, macOS, and full XEmbed-only runtime validation were not performed.
With the native path selected, a connected StatusNotifier host is required for
close-to-tray; use the opt-out on XEmbed-only panels. No RAM reduction is claimed.
