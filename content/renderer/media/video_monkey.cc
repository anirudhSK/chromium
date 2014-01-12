#include <stdio.h>
#include <assert.h>
#include "content/renderer/media/video_monkey.hh"

VideoMonkey::VideoMonkey()
    : display_times_(),
      start_time_(get_current_ms()) {
  printf( "Initialized constructor once\n");
}

VideoMonkey::~VideoMonkey() {
  printf("Called destructor once\n");
  for (uint32_t i = 0; i < display_times_.size(); i++) {
    printf("%lu\n", display_times_.at( i ));
  }
}

void VideoMonkey::add_frame() {
  uint64_t current_time = get_current_ms();
  assert(current_time > start_time_);
  display_times_.push_back(current_time - start_time_);
}

uint64_t VideoMonkey::get_current_ms() const {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000.0;
}
