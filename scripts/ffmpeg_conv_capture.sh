#!/bin/sh
# This can be used from your captures directory to convert the output to video, using ffmpeg
ffmpeg -framerate 60 -i '%d.qoi' -c:v libx264 out.mp4
