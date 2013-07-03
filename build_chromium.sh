./build/gyp_chromium -Dffmpeg_branding="Chrome" -Dproprietary_codecs=1 -Duse_system_ffmpeg=1
ninja -C out/Release chrome
echo "Binary files are located in ./out/Release"
