#!/usr/bin/env sh

#
# Adds binary directory to path.
#

VAR_DIR=.local/share/yf
BIN_DIR=$VAR_DIR/bin

case "$PATH" in
  *"$HOME/$BIN_DIR"*)
    echo "$HOME/$BIN_DIR already on PATH"
    ;;
  *)
    for file in "$HOME/.bash_profile" "$HOME/.bash_login" "$HOME/.profile"
    do
      if [ -f $file ]; then
        echo "export PATH=\$PATH:$HOME/$BIN_DIR" >> $file
        echo "logout or execute 'source $file' to apply changes"
        break
      fi
    done
    ;;
esac
