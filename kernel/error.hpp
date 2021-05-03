#pragma once

#include <array>
#include <cstdio>

class Error {
public:
  enum Code {
    kSuccess,
    kFull,
    kEmpty,
    kNoEnoughMemory,
    kIndexOutOfRange,
    kHostControllerNotHalted,
    kInvalidSlotID,
    kPortNotConnected,
    kInvalidEndpointNumber,
    kTransferRingNotSet,
    kAlreadyAllocated,
    kNotImplemented,
    kInvalidDescriptor,
    kBufferTooSmall,
    kUnknownDevice,
    kNoCorrespondingSetupStage,
    kTransferFailed,
    kInvalidPhase,
    kUnknownXHCISpeedID,
    kNoWaiter,
    kNoPCIMSI,
    kLastOfCode, // この列挙子は常に最後に配置する
  };

private:
  // <const char *, N> を入れないと以下の警告が出る。ビルドはできる
  // argument list for class template "std::__1::array" is missingC/C++(441)
  // expression must have a constant value -- invalid type "<error-type>" for
  // constant-expression evaluationC/C++(28)
  static constexpr std::array<const char *, 21> code_names_{
      "kSuccess",
      "kFull",
      "kEmpty",
      "kNoEnoughMemory",
      "kIndexOutOfRange",
      "kHostControllerNotHalted",
      "kInvalidSlotID",
      "kPortNotConnected",
      "kInvalidEndpointNumber",
      "kTransferRingNotSet",
      "kAlreadyAllocated",
      "kNotImplemented",
      "kInvalidDescriptor",
      "kBufferTooSmall",
      "kUnknownDevice",
      "kNoCorrespondingSetupStage",
      "kTransferFailed",
      "kInvalidPhase",
      "kUnknownXHCISpeedID",
      "kNoWaiter",
      "kNoPCIMSI"
  };
  static_assert(Error::Code::kLastOfCode == code_names_.size());

public:
  Error(Code code, const char *file, int line)
      : code_{code}, line_{line}, file_{file} {};

  Code Cause() const { return this->code_; }

  operator bool() const { return this->code_ != kSuccess; }

  const char *Name() const {
    return code_names_[static_cast<int>(this->code_)];
  }

  const char *File() const { return this->file_; }

  int Line() const { return this->line_; }

private:
  Code code_;
  int line_;
  const char *file_;
};

#define MAKE_ERROR(code) Error((code), __FILE__, __LINE__)

template <class T> struct WithError {
  T value;
  Error error;
};
