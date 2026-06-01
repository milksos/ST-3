// Copyright 2021 GHA Test Team

#include <gmock/gmock.h>
#include <gtest/gtest.h>

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
    door = std::make_unique<TimedDoor>(30);
    door->lock();
  }

  void TearDown() override {
    door.reset();
  }
};

TEST_F(TimedDoorFixture, DoorStartsClosedAfterSetup) {
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorFixture, LockKeepsDoorClosed) {
  door->lock();

  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorFixture, TimeoutValueIsReturnedByGetter) {
  EXPECT_EQ(door->getTimeOut(), 30);
}

TEST_F(TimedDoorFixture, ThrowStateDoesNotThrowForClosedDoor) {
  EXPECT_NO_THROW(door->throwState());
}

TEST_F(TimedDoorFixture, ThrowStateThrowsForOpenedDoor) {
  TimedDoor localDoor(0);
  localDoor.lock();

  EXPECT_THROW(localDoor.unlock(), std::runtime_error);

  EXPECT_THROW(localDoor.throwState(), std::runtime_error);
}

TEST(TimedDoorStandalone, UnlockThrowsWhenDoorStaysOpenedUntilTimeout) {
  TimedDoor door(0);
  door.lock();

  EXPECT_THROW(door.unlock(), std::runtime_error);
}

TEST(TimedDoorStandalone, DoorRemainsOpenedAfterUnlockException) {
  TimedDoor door(0);
  door.lock();

  EXPECT_THROW(door.unlock(), std::runtime_error);
  EXPECT_TRUE(door.isDoorOpened());
}

TEST(TimedDoorStandalone, UnlockReturnsNormallyWhenDoorGetsClosedInTime) {
  TimedDoor door(80);
  door.lock();

  auto worker = std::async(std::launch::async, [&door]() {
    try {
      door.unlock();
      return true;
    } catch (const std::runtime_error&) {
      return false;
    }
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(15));
  door.lock();

  EXPECT_TRUE(worker.get());
  EXPECT_FALSE(door.isDoorOpened());
}

TEST(TimedDoorStandalone, NegativeTimeoutIsNormalizedToZero) {
  TimedDoor door(-10);

  EXPECT_EQ(door.getTimeOut(), 0);
}

TEST(TimedDoorStandalone, LockAfterFailedUnlockClosesDoorAgain) {
  TimedDoor door(0);
  door.lock();

  EXPECT_THROW(door.unlock(), std::runtime_error);
  door.lock();

  EXPECT_FALSE(door.isDoorOpened());
}

TEST(TimerStandalone, RegisterWithNullClientDoesNotThrow) {
  Timer timer;

  EXPECT_NO_THROW(timer.tregister(0, nullptr));
}

TEST(TimerStandalone, RegisterInvokesClientTimeoutOnce) {
  Timer timer;
  MockTimerClient client;

  EXPECT_CALL(client, Timeout()).Times(1);
  timer.tregister(0, &client);
}

TEST(TimerStandalone, RegisterWithPositiveDelayStillInvokesClient) {
  Timer timer;
  MockTimerClient client;

  EXPECT_CALL(client, Timeout()).Times(1);
  timer.tregister(5, &client);
}

TEST(TimerStandalone, RegisterWaitsBeforeCallbackForPositiveTimeout) {
  Timer timer;
  MockTimerClient client;
  const auto started = std::chrono::steady_clock::now();

  EXPECT_CALL(client, Timeout()).Times(1);
  timer.tregister(5, &client);

  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - started);
  EXPECT_GE(elapsed.count(), 5);
}

TEST(DoorTimerAdapterStandalone, TimeoutThrowsIfDoorIsStillOpened) {
  TimedDoor door(0);
  door.lock();

  EXPECT_THROW(door.unlock(), std::runtime_error);
  DoorTimerAdapter adapter(door);

  EXPECT_THROW(adapter.Timeout(), std::runtime_error);
}

TEST(DoorTimerAdapterStandalone, TimeoutDoesNotThrowForClosedDoor) {
  TimedDoor door(10);
  door.lock();
  DoorTimerAdapter adapter(door);

  EXPECT_NO_THROW(adapter.Timeout());
}

TEST(DoorTimerAdapterStandalone, TimeoutReflectsDoorStateAfterManualClose) {
  TimedDoor door(10);
  door.lock();
  DoorTimerAdapter adapter(door);

  EXPECT_NO_THROW(adapter.Timeout());
}
