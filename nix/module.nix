# NixOS/Home Manager module for Abaddon Discord client
{ config, lib, pkgs, ... }:

let
  cfg = config.programs.abaddon;
  # Merge user settings with empty defaults for each section
  mergedSettings = lib.recursiveUpdate {
    discord = { };
    gui = { };
    http = { };
    style = { };
    notifications = { };
    voice = { };
  } cfg.settings;

  # Generate the INI content
  configContent = lib.generators.toINI {} mergedSettings;

  # Generate CSS content
  cssContent =
    if cfg.css != null then cfg.css
    else if cfg.cssFile != null then builtins.readFile cfg.cssFile
    else null;

in
{
  options.programs.abaddon = {
    enable = lib.mkEnableOption "Abaddon Discord client";

    package = lib.mkOption {
      type = lib.types.package;
      description = "The abaddon package to use.";
    };

    settings = lib.mkOption {
      type = lib.types.submodule {
        freeformType = lib.types.attrsOf (lib.types.attrsOf (lib.types.oneOf [
          lib.types.str
          lib.types.bool
          lib.types.int
          lib.types.float
        ]));

        options = {
          discord = lib.mkOption {
            type = lib.types.submodule {
              freeformType = lib.types.attrsOf (lib.types.oneOf [
                lib.types.str
                lib.types.bool
                lib.types.int
              ]);

              options = {
                api_base = lib.mkOption {
                  type = lib.types.str;
                  default = "https://discord.com/api/v9";
                  description = "Discord API base URL.";
                };

                gateway = lib.mkOption {
                  type = lib.types.str;
                  default = "wss://gateway.discord.gg/?v=9&encoding=json&compress=zlib-stream";
                  description = "Discord gateway WebSocket URL.";
                };

                memory_db = lib.mkOption {
                  type = lib.types.bool;
                  default = false;
                  description = "Use in-memory database instead of file.";
                };

                prefetch = lib.mkOption {
                  type = lib.types.bool;
                  default = false;
                  description = "Prefetch messages on startup.";
                };

                autoconnect = lib.mkOption {
                  type = lib.types.bool;
                  default = false;
                  description = "Automatically connect on startup.";
                };

                keychain = lib.mkOption {
                  type = lib.types.bool;
                  default = true;
                  description = "Store token in system keychain.";
                };
              };
            };
            default = { };
            description = "Discord connection settings.";
          };

          gui = lib.mkOption {
            type = lib.types.submodule {
              freeformType = lib.types.attrsOf (lib.types.oneOf [
                lib.types.str
                lib.types.bool
                lib.types.int
                lib.types.float
              ]);

              options = {
                css = lib.mkOption {
                  type = lib.types.str;
                  default = "main.css";
                  description = "CSS file to use for styling (relative to resource dir).";
                };

                animated_guild_hover_only = lib.mkOption {
                  type = lib.types.bool;
                  default = true;
                  description = "Only animate guild icons on hover.";
                };

                animations = lib.mkOption {
                  type = lib.types.bool;
                  default = true;
                  description = "Enable animations.";
                };

                custom_emojis = lib.mkOption {
                  type = lib.types.bool;
                  default = true;
                  description = "Show custom emojis.";
                };

                owner_crown = lib.mkOption {
                  type = lib.types.bool;
                  default = true;
                  description = "Show crown icon for server owner.";
                };

                save_state = lib.mkOption {
                  type = lib.types.bool;
                  default = true;
                  description = "Save window state between sessions.";
                };

                stock_emojis = lib.mkOption {
                  type = lib.types.bool;
                  default = false;
                  description = "Show stock emojis.";
                };

                unreads = lib.mkOption {
                  type = lib.types.bool;
                  default = true;
                  description = "Show unread indicators.";
                };

                alt_menu = lib.mkOption {
                  type = lib.types.bool;
                  default = false;
                  description = "Use alternative menu style.";
                };

                hide_to_tray = lib.mkOption {
                  type = lib.types.bool;
                  default = false;
                  description = "Hide to system tray on close.";
                };

                show_deleted_indicator = lib.mkOption {
                  type = lib.types.bool;
                  default = true;
                  description = "Show indicator for deleted messages.";
                };

                font_scale = lib.mkOption {
                  type = lib.types.float;
                  default = -1.0;
                  description = "Font scale factor (-1 for default).";
                };

                folder_icon_only = lib.mkOption {
                  type = lib.types.bool;
                  default = false;
                  description = "Show only icons for folders.";
                };

                classic_change_guild_on_open = lib.mkOption {
                  type = lib.types.bool;
                  default = true;
                  description = "Change guild when opening in classic mode.";
                };

                image_embed_clamp_width = lib.mkOption {
                  type = lib.types.int;
                  default = 400;
                  description = "Maximum width for image embeds.";
                };

                image_embed_clamp_height = lib.mkOption {
                  type = lib.types.int;
                  default = 300;
                  description = "Maximum height for image embeds.";
                };

                classic_channels = lib.mkOption {
                  type = lib.types.bool;
                  default = false;
                  description = "Use classic channel list style.";
                };
              };
            };
            default = { };
            description = "GUI settings.";
          };

          http = lib.mkOption {
            type = lib.types.submodule {
              freeformType = lib.types.attrsOf (lib.types.oneOf [
                lib.types.str
                lib.types.int
              ]);

              options = {
                concurrent = lib.mkOption {
                  type = lib.types.int;
                  default = 20;
                  description = "Number of concurrent HTTP connections.";
                };

                user_agent = lib.mkOption {
                  type = lib.types.str;
                  default = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.87 Safari/537.36";
                  description = "User agent string for HTTP requests.";
                };
              };
            };
            default = { };
            description = "HTTP client settings.";
          };

          style = lib.mkOption {
            type = lib.types.submodule {
              freeformType = lib.types.attrsOf lib.types.str;

              options = {
                expandercolor = lib.mkOption {
                  type = lib.types.str;
                  default = "rgba(255, 83, 112, 0)";
                  description = "Color for channel expanders.";
                };

                nsfwchannelcolor = lib.mkOption {
                  type = lib.types.str;
                  default = "#970d0d";
                  description = "Color for NSFW channels.";
                };

                mentionbadgecolor = lib.mkOption {
                  type = lib.types.str;
                  default = "rgba(184, 37, 37, 0)";
                  description = "Background color for mention badges.";
                };

                mentionbadgetextcolor = lib.mkOption {
                  type = lib.types.str;
                  default = "rgba(251, 251, 251, 0)";
                  description = "Text color for mention badges.";
                };

                unreadcolor = lib.mkOption {
                  type = lib.types.str;
                  default = "rgba(255, 255, 255, 0)";
                  description = "Color for unread indicator.";
                };
              };
            };
            default = { };
            description = "Style/color settings.";
          };

          notifications = lib.mkOption {
            type = lib.types.submodule {
              freeformType = lib.types.attrsOf lib.types.bool;

              options = {
                enabled = lib.mkOption {
                  type = lib.types.bool;
                  default = true;
                  description = "Enable desktop notifications.";
                };

                playsound = lib.mkOption {
                  type = lib.types.bool;
                  default = true;
                  description = "Play sound on notifications.";
                };
              };
            };
            default = { };
            description = "Notification settings.";
          };

          voice = lib.mkOption {
            type = lib.types.submodule {
              freeformType = lib.types.attrsOf lib.types.str;

              options = {
                vad = lib.mkOption {
                  type = lib.types.str;
                  default = "rnnoise";
                  description = "Voice activity detection method (rnnoise or gate).";
                };

                backends = lib.mkOption {
                  type = lib.types.str;
                  default = "";
                  description = "Audio backends to use.";
                };
              };
            };
            default = { };
            description = "Voice chat settings.";
          };
        };
      };
      default = { };
      description = ''
        Configuration for Abaddon. Will be written to ~/.config/abaddon/abaddon.ini.
        
        Example:
        ```nix
        programs.abaddon.settings = {
          discord = {
            autoconnect = true;
          };
          gui = {
            animations = false;
            font_scale = 1.2;
          };
          style = {
            nsfwchannelcolor = "#ff0000";
          };
        };
        ```
      '';
    };

    css = lib.mkOption {
      type = lib.types.nullOr lib.types.lines;
      default = null;
      description = ''
        Custom CSS to use for styling Abaddon.
        This will be written to a file and referenced in the config.
        
        Example:
        ```nix
        programs.abaddon.css = '''
          .embed {
            border-radius: 10px;
            background-color: rgba(0, 0, 0, 0.2);
          }
        ''';
        ```
      '';
    };

    cssFile = lib.mkOption {
      type = lib.types.nullOr lib.types.path;
      default = null;
      description = ''
        Path to a custom CSS file for styling Abaddon.
        Alternative to the `css` option.
      '';
    };
  };

  config = lib.mkIf cfg.enable {
    home.packages = [ cfg.package ];

    # Create the config directory and INI file
    xdg.configFile."abaddon/abaddon.ini" = lib.mkIf (cfg.settings != { }) {
      text = configContent;
    };

    # Create custom CSS file if provided
    xdg.configFile."abaddon/css/custom.css" = lib.mkIf (cssContent != null) {
      text = cssContent;
    };

    # Override CSS path in settings if custom CSS is provided
    programs.abaddon.settings.gui.css = lib.mkIf (cssContent != null)
      (lib.mkDefault "custom.css");
  };
}

