#!/bin/sh
# Use this script to create the compile_commands.json file.
# This is necessary for clangd completion.

cmake . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=True