#!/bin/bash


which fru-dump >/dev/null 2>&1 && echo 'fru-dump installed successfully!' || { echo 'fru-dump failed to install!'; exit 1; }

[ -d /usr/lib/fmc-tools ] && [ "$(ls -l /usr/lib/fmc-tools | wc -l)" -eq "$(ls -l masterfiles/ | wc -l)" ] && echo "fmc-tools binaries installed successfully!" || { echo "fmc-tools binaries failed to install!"; exit 1; }

[ -f /usr/share/man/man1/fru-dump.1.gz ] && echo 'fru-dump.1 installed successfully!' || { echo 'fru-dump.1 failed to install!'; exit 1; }
