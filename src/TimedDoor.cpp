// Copyright 2021 GHA Test Team

#include <chrono>
#include <stdexcept>
#include <thread>

#include "TimedDoor.h"

DoorTimerAdapter::DoorTimerAdapter(TimedDoor& door) : door(door) {}

void DoorTimerAdapter::Timeout() {
  door.throwState();
}

TimedDoor::TimedDoor(int timeout)
    : adapter(new DoorTimerAdapter(*this)),
      iTimeout(timeout < 0 ? 0 : timeout),
      isOpened(false) {}

bool TimedDoor::isDoorOpened() {
  return isOpened;
}

void TimedDoor::unlock() {
  Timer timer;

  isOpened = true;
  timer.tregister(iTimeout, adapter);
}

void TimedDoor::lock() {
  isOpened = false;
}

int TimedDoor::getTimeOut() const {
  return iTimeout;
}

void TimedDoor::throwState() {
  if (isOpened) {
    throw std::runtime_error("Door left opened for too long");
  }
}

void Timer::sleep(int timeout) {
  if (timeout > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
  }
}

void Timer::tregister(int timeout, TimerClient* cl) {
  client = cl;
  sleep(timeout);

  if (client != nullptr) {
    client->Timeout();
  }
}
