#!/bin/bash

version=$1
architecture=$(dpkg --print-architecture)

source_code=$(basename "$PWD")

# Use sudo only if not running as root
if [ "$(id -u)" -eq 0 ]; then
    apt-get update
    apt-get install -y build-essential make devscripts debhelper
else
    sudo apt-get update
    sudo apt-get install -y build-essential make devscripts debhelper
fi

#Replace placeholders inside the debian template files
sed -i "s/@VERSION@/$version-1/" packaging/debian/changelog
sed -i "s/@DATE@/$(date -R)/" packaging/debian/changelog
sed -i "s/@ARCHITECTURE@/$architecture/" packaging/debian/control

cp -r packaging/debian .

rm -rf packaging

pushd ..
tar czf ${source_code}_${version}.orig.tar.gz \
    --exclude='.git' \
    --exclude='debian' \
    $source_code
popd

debuild
