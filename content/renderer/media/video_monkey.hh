#ifndef VIDEO_MONKEY_HH_
#define VIDEO_MONKEY_HH_

#include <sys/time.h>
#include <stdint.h>
#include <vector>

class VideoMonkey {
 public:
  VideoMonkey();
  ~VideoMonkey();
  void add_frame();

 private:
  uint64_t get_current_ms() const;
  std::vector<uint64_t> display_times_;
  uint64_t start_time_;
};

#endif // VIDEO_MONKEY_HH_
