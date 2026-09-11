{
  description = "Abaddon - A Discord client built with GTK";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    # Submodule sources
    ixwebsocket-src = {
      url = "github:machinezone/ixwebsocket";
      flake = false;
    };
    miniaudio-src = {
      url = "github:mackron/miniaudio";
      flake = false;
    };
    keychain-src = {
      url = "github:hrantzsch/keychain";
      flake = false;
    };
    rnnoise-src = {
      url = "github:xiph/rnnoise";
      flake = false;
    };
    qrcodegen-src = {
      url = "github:nayuki/QR-Code-generator";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, flake-utils, ixwebsocket-src, miniaudio-src, keychain-src, rnnoise-src, qrcodegen-src }:
    let
      # System-independent outputs
      systemIndependentOutputs = {
        # NixOS module (system-wide installation)
        nixosModules.default = { config, lib, pkgs, ... }: {
          imports = [ ./nix/nixos-module.nix ];
          programs.abaddon.package = lib.mkDefault self.packages.${pkgs.system}.abaddon;
        };
        nixosModules.abaddon = self.nixosModules.default;

        # Home Manager module (user-level configuration)
        homeManagerModules.default = { config, lib, pkgs, ... }: {
          imports = [ ./nix/module.nix ];
          programs.abaddon.package = lib.mkDefault self.packages.${pkgs.system}.abaddon;
        };
        homeManagerModules.abaddon = self.homeManagerModules.default;

        # Overlay for adding abaddon to pkgs
        overlays.default = final: prev: {
          abaddon = self.packages.${prev.system}.abaddon;
        };
      };

      # Per-system outputs
      perSystemOutputs = flake-utils.lib.eachDefaultSystem (system:
        let
          pkgs = import nixpkgs { inherit system; };

          abaddon = pkgs.stdenv.mkDerivation rec {
            pname = "abaddon";
            version = "0.2.1";

            src = ./.;

            postPatch = ''
              # Create subprojects directory and copy submodule sources
              mkdir -p subprojects

              cp -r ${ixwebsocket-src} subprojects/ixwebsocket
              chmod -R u+w subprojects/ixwebsocket

              cp -r ${miniaudio-src} subprojects/miniaudio
              chmod -R u+w subprojects/miniaudio

              cp -r ${keychain-src} subprojects/keychain
              chmod -R u+w subprojects/keychain

              cp -r ${rnnoise-src} subprojects/rnnoise
              chmod -R u+w subprojects/rnnoise

              cp -r ${qrcodegen-src} subprojects/qrcodegen
              chmod -R u+w subprojects/qrcodegen
            '';

            nativeBuildInputs = with pkgs; [
              cmake
              pkg-config
              wrapGAppsHook3
            ];

            buildInputs = with pkgs; [
              # Required
              gtkmm3
              nlohmann_json
              curl
              zlib
              sqlite
              spdlog
              openssl

              # Optional but enabled by default
              libhandy
              libopus
              libsodium
              libsecret  # Required for keychain
              fontconfig
              rnnoise  # For voice activity detection

              # GTK/GLib ecosystem
              glib
              gtk3
              glibmm
              cairomm
              pangomm
              atkmm
              harfbuzz
              sysprof  # For sysprof-capture-4 pkg-config

              # X11 dependencies
              xorg.libXdmcp

              # Icons
              hicolor-icon-theme
              adwaita-icon-theme
            ];

            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=Release"
              "-DABADDON_RESOURCE_DIR=${placeholder "out"}/share/abaddon"
              "-DUSE_LIBHANDY=ON"
              "-DENABLE_VOICE=ON"
              "-DUSE_KEYCHAIN=ON"
              "-DENABLE_NOTIFICATION_SOUNDS=ON"
              "-DENABLE_RNNOISE=ON"
              "-DENABLE_QRCODE_LOGIN=ON"
            ];

            postInstall = ''
              # Install desktop file and icon
              install -Dm644 $src/res/desktop/io.github.uowuo.abaddon.desktop $out/share/applications/io.github.uowuo.abaddon.desktop
              install -Dm644 $src/res/desktop/icon.svg $out/share/icons/hicolor/scalable/apps/io.github.uowuo.abaddon.svg
              install -Dm644 $src/res/desktop/io.github.uowuo.abaddon.metainfo.xml $out/share/metainfo/io.github.uowuo.abaddon.metainfo.xml
            '';

            meta = with pkgs.lib; {
              description = "Alternative Discord client made in C++ with GTK";
              homepage = "https://github.com/uowuo/abaddon";
              license = licenses.gpl3Plus;
              platforms = platforms.linux;
              mainProgram = "abaddon";
            };
          };
        in
        {
          packages = {
            default = abaddon;
            abaddon = abaddon;
          };

          devShells.default = pkgs.mkShell {
            inputsFrom = [ abaddon ];
            packages = with pkgs; [
              gdb
              valgrind
            ];
          };
        }
      );
    in
    # Merge system-independent and per-system outputs
    perSystemOutputs // systemIndependentOutputs;
}
