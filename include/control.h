#pragma once

enum RunState {
  STATE_START,
  STATE_UNPAIRED,
  STATE_PAIRED,
  STATE_RUNNING,
  STATE_PAUSED,
  STATE_SNAPSHOT,
  STATE_SNAPSHOT_WAIT
};

#if defined __cplusplus
extern "C" {
#endif
void setState(enum RunState);
enum RunState getState(void);

void addTime(int time);
void setTime(int time);
void startStop(bool start);
int getShutterTimeout(void);
void doSnapshot(void);
#if defined __cplusplus
}
#endif
