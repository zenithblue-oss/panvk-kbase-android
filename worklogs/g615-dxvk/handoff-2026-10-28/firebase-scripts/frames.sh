#!/bin/sh
# usage: frames.sh <run dir>  -> <run dir>/screenshots/{start,mid,end}.png from raw/*/video.mp4
FF=/tmp/claude-1000/-home-flux-repos-panvk-kbase-android/db381bd6-e816-4c4b-b498-8633cd891ac4/scratchpad/ffdl/ffmpeg-7.0.2-arm64-static/ffmpeg
FP=/tmp/claude-1000/-home-flux-repos-panvk-kbase-android/db381bd6-e816-4c4b-b498-8633cd891ac4/scratchpad/ffdl/ffmpeg-7.0.2-arm64-static/ffprobe
RUN=$1
V=`ls $RUN/raw/*/video.mp4 | head -1`
mkdir -p $RUN/screenshots
D=`$FP -v error -show_entries format=duration -of csv=p=0 $V | cut -d. -f1`
M=`expr $D / 2`
$FF -v error -y -ss 30 -i $V -frames:v 1 $RUN/screenshots/video-start.png
$FF -v error -y -ss $M -i $V -frames:v 1 $RUN/screenshots/video-mid.png
$FF -v error -y -sseof -3 -i $V -frames:v 1 $RUN/screenshots/video-end.png
ls $RUN/screenshots
