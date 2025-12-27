# Home Manager module wrapper
# This file imports the main module for use with Home Manager
{ config, lib, pkgs, ... }:

{
  imports = [ ./module.nix ];
}

