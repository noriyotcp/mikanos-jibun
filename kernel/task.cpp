#include "task.hpp"

#include "asmfunc.h"
#include "segment.hpp"
#include "timer.hpp"

// #@@range_begin(task_ctor)
Task::Task(uint64_t id) : id_{id} {}
// #@@range_end(task_ctor)

// #@@range_begin(task_initctx)
Task &Task::InitContext(TaskFunc *f, int64_t data) {
  const size_t stack_size = kDefaultStackBytes / sizeof(stack_[0]);
  stack_.resize(stack_size);
  uint64_t stack_end = reinterpret_cast<uint64_t>(&stack_[stack_size]);

  memset(&context_, 0, sizeof(context_));
  context_.cr3 = GetCR3();
  context_.rflags = 0x202;
  context_.cs = kKernelCS;
  context_.ss = kKernelSS;
  context_.rsp = (stack_end & ~0xflu) - 8;

  context_.rip = reinterpret_cast<uint64_t>(f);
  context_.rdi = id_;
  context_.rsi = data;

  // MXCSR の全ての例外をマスクする
  *reinterpret_cast<uint32_t *>(&context_.fxsave_area[24]) = 0x1f80;

  return *this;
}
// #@@range_end(task_initctx)

TaskContext &Task::Context() { return context_; }

uint64_t Task::ID() const { return id_; }

Task &Task::Sleep() {
  task_manager->Sleep(this);
  return *this;
}

Task &Task::Wakeup() {
  task_manager->Wakeup(this);
  return *this;
}

TaskManager::TaskManager() { running_.push_back(&NewTask()); }

// #@@range_begin(taskmgr_newtask)
Task &TaskManager::NewTask() {
  ++latest_id_;
  return *tasks_.emplace_back(new Task{latest_id_});
}
// #@@range_end(taskmgr_newtask)

// #@@range_begin(taskmgr_switchtask)
void TaskManager::SwitchTask(bool current_sleep) {
  Task *current_task = running_.front();
  running_.pop_front();
  if (!current_sleep) {
    running_.push_back(current_task);
  }

  Task *next_task = running_.front();

  SwitchContext(&next_task->Context(), &current_task->Context());
}
// #@@range_end(taskmgr_switchtask)

// #@@range_begin(taskmgr_sleep)
void TaskManager::Sleep(Task *task) {
  auto it = std::find(running_.begin(), running_.end(), task);

  if (it == running_.begin()) { // Task is running
    SwitchTask(true);           // Switch task to sleep a running task
  }

  if (it == running_.end()) { // Task is not in the run-queue
    return;
  }
  // タスクがランキューの先頭以外に存在する
  running_.erase(it); // タスクをランキューから外す。タスク切り替えはしない
}
// #@@range_end(taskmgr_sleep)

// #@@range_begin(taskmgr_sleep_id)
Error TaskManager::Sleep(uint64_t id) {
  auto it =
      std::find_if(tasks_.begin(), tasks_.end(), [id](const auto &t) { return t->ID() == id; });
  if (it == tasks_.end()) {
    return MAKE_ERROR(Error::kNoSuchTask);
  }

  Sleep(it->get());
  return MAKE_ERROR(Error::kSuccess);
}
// #@@range_end(taskmgr_sleep_id)

// #@@range_begin(taskmgr_wakeup)
void TaskManager::Wakeup(Task *task) {
  auto it = std::find(running_.begin(), running_.end(), task);
  if (it == running_.end()) {
    running_.push_back(task);
  }
}
// #@@range_end(taskmgr_wakeup)

// #@@range_begin(taskmgr_wakeup_id)
Error TaskManager::Wakeup(uint64_t id) {
  auto it =
      std::find_if(tasks_.begin(), tasks_.end(), [id](const auto &t) { return t->ID() == id; });
  if (it == tasks_.end()) {
    return MAKE_ERROR(Error::kNoSuchTask);
  }

  Wakeup(it->get());
  return MAKE_ERROR(Error::kSuccess);
}
// #@@range_end(taskmgr_wakeup_id)

TaskManager *task_manager;

// #@@range_begin(inittask)
void InitializeTask() {
  task_manager = new TaskManager;

  __asm__("cli");
  timer_manager->AddTimer(Timer{timer_manager->CurrentTick() + kTaskTimerPeriod, kTaskTimerValue});
  __asm__("sti");
}
// #@@range_end(inittask)
