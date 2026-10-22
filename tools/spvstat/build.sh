#!/bin/sh
# Build spvstat against a host Mesa build that has the panfrost compiler
# (build/host-tools, source /home/flux/src/mesa). Compile flags and the link
# line are taken from that build's panfrost_compile target.
#   sh tools/spvstat/build.sh   -> tools/spvstat/spvstat
D=$(cd "$(dirname "$0")" && pwd)
B=${HOST_BUILD:-$D/../../build/host-tools}
cd "$B" || exit 1
ARGS=$(sed -n '/^build src\/panfrost\/clc\/panfrost_compile.p\/pan_compile.c.o:/,/^$/s/^ ARGS = //p' build.ninja |
       sed "s/-Werror=[a-z-]*//g; s/'-D[A-Z_]*=\"[^\"]*\"'//g")
LIBS=$(sed -n '/^build src\/panfrost\/clc\/panfrost_compile:/,/^$/s/^ LINK_ARGS = //p' build.ninja)
eval gcc $ARGS -Wno-error -c "$D/spvstat.c" -o "$D/spvstat.o" || exit 1
eval g++ -o "$D/spvstat" "$D/spvstat.o" $LIBS
