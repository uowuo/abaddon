# NixOS system-wide module for Abaddon
# This provides system-level installation (not per-user configuration)
{ config, lib, pkgs, ... }:

let
  cfg = config.programs.abaddon;
in
{
  options.programs.abaddon = {
    enable = lib.mkEnableOption "Abaddon Discord client";

    package = lib.mkOption {
      type = lib.types.package;
      description = "The abaddon package to use.";
    };
  };

  config = lib.mkIf cfg.enable {
    environment.systemPackages = [ cfg.package ];
  };
}

