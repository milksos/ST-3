// Copyright 2021 GHA Test Team

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdint>
#include <chrono>
#include <future>
#include <memory>
#include <stdexcept>
#include <thread>
#include "TimedDoor.h"

class MockTimerClient : public TimerClient {
 public:
  MOCK_METHOD(void, Timeout, (), (override));
};

class TimedDoorFixture : public ::testing::Test {
 protected:
  std::unique_ptr<TimedDoor> door;
  void SetUp() override {
    door = std::make_unique<TimedDoor>(15);
    door->lock();
  }
  void TearDown() override {
    door.reset();
  }
};

TEST_F(TimedDoorFixture, DoorIsClosedAfterSetup) {
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorFixture, LockKeepsDoorClosed) {
  door->lock();
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorFixture, ThrowStateDoesNotThrowForClosedDoor) {
  EXPECT_NO_THROW(door->throwState());
}

TEST_F(TimedDoorFixture, ThrowStateThrowsForOpenedDoor) {
  try {
    door->unlock();
  } catch (const std::runtime_error&) {
  }
  EXPECT_THROW(door->throwState(), std::runtime_error);
}

TEST(TimedDoorStandalone, UnlockThrowsIfDoorIsStillOpenAfterTimeout) {
  TimedDoor door(0);
  door.lock();
  EXPECT_THROW(door.unlock(), std::runtime_error);
}

TEST(TimedDoorStandalone, TimeoutValueIsStoredInDoor) {
  TimedDoor door(456);
  EXPECT_EQ(door.getTimeOut(), 456);
}

TEST(TimedDoorStandalone, DoorRemainsOpenedAfterUnlockException) {
  TimedDoor door(0);
  door.lock();
  try {
    door.unlock();
  } catch (const std::runtime_error&) {
  }
  EXPECT_TRUE(door.isDoorOpened());
}

TEST(TimedDoorStandalone, UnlockDoesNotThrowIfDoorGetsClosedBeforeTimeout) {
  TimedDoor door(100);
  door.lock();
  auto worker = std::async(std::launch::async, [&door]() -> bool {
    try {
      door.unlock();
      return false;
    } catch (const std::runtime_error&) {
      return true;
    }
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  door.lock();
  EXPECT_FALSE(worker.get());
  EXPECT_FALSE(door.isDoorOpened());
}

TEST(TimerStandalone, RegisterWithNullClientDoesNotThrow) {
  Timer timer;
  EXPECT_NO_THROW(timer.tregister(0, nullptr));
}

TEST(TimerStandalone, RegisterCallsClientTimeout) {
  Timer timer;
  MockTimerClient client;
  EXPECT_CALL(client, Timeout()).Times(1);
  timer.tregister(0, &client);
}

TEST(TimerStandalone, RegisterWithDelayStillCallsClientTimeout) {
  Timer timer;
  MockTimerClient client;
  EXPECT_CALL(client, Timeout()).Times(1);
  timer.tregister(5, &client);
}

TEST(DoorTimerAdapterStandalone, TimeoutThrowsWhenDoorRemainsOpened) {
  TimedDoor door(0);
  door.lock();
  try {
    door.unlock();
  } catch (const std::runtime_error&) {
  }
  DoorTimerAdapter adapter(door);
  EXPECT_THROW(adapter.Timeout(), std::runtime_error);
}

TEST(DoorTimerAdapterStandalone, TimeoutDoesNotThrowWhenDoorClosed) {
  TimedDoor door(0);
  door.lock();
  DoorTimerAdapter adapter(door);
  EXPECT_NO_THROW(adapter.Timeout());
}
