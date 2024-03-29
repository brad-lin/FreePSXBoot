#!/bin/sh

ROOT=$(dirname $0)
CWD=$(pwd)
cd $ROOT
ROOT=$(pwd)
cd $CWD

docker pull ghcr.io/grumpycoders/pcsx-redux-build:latest
docker run --rm -i -w/project${CWD#$ROOT} -v "${ROOT}:/project" -u `id -u`:`id -g` ghcr.io/grumpycoders/pcsx-redux-build:latest make $@
