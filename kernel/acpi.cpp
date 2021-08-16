#include "acpi.hpp"

#include "asmfunc.h"
#include "logger.hpp"
#include <cstdlib>
#include <cstring>

namespace {
  template <typename T>
  uint8_t SumBytes(const T *data, size_t bytes) {
    return SumBytes(reinterpret_cast<const uint8_t *>(data), bytes);
  }

  template <>
  uint8_t SumBytes<uint8_t>(const uint8_t *data, size_t bytes) {
    uint8_t sum = 0;
    for (size_t i = 0; i < bytes; ++i) {
      sum += data[i];
    }
    return sum;
  }
} // namespace

namespace acpi {
  bool RSDP::IsValid() const {
    if (strncmp(this->signature, "RSD PTR ", 8) != 0) {
      Log(kDebug, "invalid signature: %.8s\n", this->signature);
      return false;
    }
    if (this->revision != 2) {
      Log(kDebug, "ACPI revision must be 2: %d\n", this->revision);
      return false;
    }
    if (auto sum = SumBytes(this, 20); sum != 0) {
      Log(kDebug, "sum of 20 bytes must be 0: %d\n", sum);
      return false;
    }
    if (auto sum = SumBytes(this, 36); sum != 0) {
      Log(kDebug, "sum of 36 bytes must be 0: %d\n", sum);
      return false;
    }
    return true;
  }

  bool DescriptionHeader::IsValid(const char *expected_signature) const {
    if (strncmp(this->signature, expected_signature, 4) != 0) {
      Log(kDebug, "invalid signature: %.4s\n", this->signature);
      return false;
    }
    if (auto sum = SumBytes(this, this->length); sum != 0) {
      Log(kDebug, "sum of %u bytes must be 0: %d\n", this->length, sum);
      return false;
    }
    return true;
  }

  // XSDT は記述ヘッダの後に各データ構造へのアドレスが並ぶ。そこを得るための []
  const DescriptionHeader &XSDT::operator[](size_t i) const {
    auto entries = reinterpret_cast<const uint64_t *>(&this->header + 1);
    return *reinterpret_cast<const DescriptionHeader *>(entries[i]);
  }

  // XSDT が保持するデータ構造のアドレスの個数を表す
  size_t XSDT::Count() const {
    return (this->header.length - sizeof(DescriptionHeader)) / sizeof(uint64_t);
  }

  const FADT *fadt;

  // #@@range_begin(wait_ms)
  void WaitMilliseconds(unsigned long msec) {
    // bit 8 を取り出す。ACPI PM timer
    // のカウンタが32ビット幅の場合に真、24ビット幅の場合に偽
    const bool pm_timer_32 = (fadt->flags >> 8) & 1;
    // カウンタは fadt->pm_tmr_blk が表す IO ポートに存在するので、IoIn32
    // でその時点のカウント値を読み取る
    const uint32_t start = IoIn32(fadt->pm_tmr_blk);
    uint32_t end = start + kPMTimerFreq * msec / 1000; // kPMTimerFreq == 3.579545 MHz
    if (!pm_timer_32) { // カウンタが24ビットの場合に end を24ビットに制限する
      end &= 0x00ffffffu;
    }

    if (end < start) { // overflow
      while (IoIn32(fadt->pm_tmr_blk) >= start) {
      };
    }
    // ACPI PM timer が指定したミリ秒だけ待つ
    while (IoIn32(fadt->pm_tmr_blk) < end) {
    };
  }
  // #@@range_end(wait_ms)

  void Initialize(const RSDP &rsdp) {
    if (!rsdp.IsValid()) {
      Log(kError, "RSDP is not valid\n");
      exit(1);
    }

    const XSDT &xsdt = *reinterpret_cast<const XSDT *>(rsdp.xsdt_address);
    if (!xsdt.header.IsValid("XSDT")) {
      Log(kError, "XSDT is not found\n");
      exit(1);
    }

    fadt = nullptr;
    for (int i = 0; i < xsdt.Count(); ++i) {
      const auto &entry = xsdt[i];
      if (entry.IsValid("FACP")) { // FACP is the signature of FADT
        fadt = reinterpret_cast<const FADT *>(&entry);
        break;
      }
    }

    if (fadt == nullptr) {
      Log(kError, "FADT is not found\n");
      exit(1);
    }
  }
} // namespace acpi
