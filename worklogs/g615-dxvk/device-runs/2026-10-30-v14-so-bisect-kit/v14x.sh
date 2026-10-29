#!/bin/zsh
# v14 suspect builds: build 119 tree (jm-v7-wip 0159) with one of 0155-0158 reverted.
# Output: /var/tmp/panvk/build-out/2026-10-29-119r<NNNN>/libvulkan_panfrost.so (bisect kit name 119r<NNNN>).
W=/var/tmp/panvk/mesa-v14x; BASE=0146a719f6f; P=/var/tmp/panvk/b122/v14x.progress
[ -d $W ] || git -C /var/tmp/panvk/mesa-119-final worktree add --detach $W $BASE
typeset -A SHA; SHA=(0155 6f7ca1b634d 0156 d5023b90d39 0157 63b0aa353b5 0158 5eef5d34f52)
for n in 0156 0158 0157 0155; do
  git -C $W reset -q --hard $BASE
  if ! git -C $W revert --no-edit ${SHA[$n]} >> $P 2>&1; then echo "$n REVERT-CONFLICT" >> $P; git -C $W revert --abort; continue; fi
  rm -f /var/tmp/panvk/bionic-v14x-dist/libvulkan_panfrost.so
  MESA=$W BDIR=/var/tmp/panvk/bionic-v14x DDIR=/var/tmp/panvk/bionic-v14x-dist sh /var/tmp/panvk/b122/build.sh
  s=/var/tmp/panvk/bionic-v14x-dist/libvulkan_panfrost.so; [ -f $s ] || s=
  if grep -q 'BUILD-EXIT 0' /var/tmp/panvk/b122/build.exit && [ -n "$s" ]; then
    o=/var/tmp/panvk/build-out/2026-10-29-119r$n; mkdir -p $o; cp $s $o/; cp /var/tmp/panvk/b122/build.out $o/build.out
    echo "$n OK $(sha256sum $o/libvulkan_panfrost.so | cut -c1-12) $(git -C $W rev-parse --short HEAD) $(date +%T)" >> $P
  else echo "$n BUILD-FAIL $(date +%T)" >> $P; fi
done
echo DONE >> $P
