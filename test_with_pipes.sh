#!/bin/bash

pushd build

AUDIO_PIPE_CONFIG=boris_config.fifo \
AUDIO_PIPE_PLAYBACK=boris_playback.fifo \
 src/stretchplayer-cli -F

# src/stretchplayer-cli -F /1.mp3 --device=hw:1,0

popd
