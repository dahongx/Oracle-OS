#pragma once

#include <cstdint>
#include <ctime>

#include "esp_random.h"
#include "nvs.h"

namespace app {

struct DailyLuck {
  int number{1};
  int color_index{0};
  int date_key{0};
  bool calendar_synced{false};
};

// Keeps the same result across scene changes, restarts and USB power cycles.
// If a trusted wall clock is supplied later (for example by Wi-Fi SNTP), the
// result rolls exactly once when the local calendar date changes.
class DailyLuckStore {
public:
  static DailyLuck load(int color_count) {
    DailyLuck result{};
    result.date_key = current_date_key();
    result.calendar_synced = result.date_key > 0;

    nvs_handle_t handle{};
    if (nvs_open("daily_luck", NVS_READWRITE, &handle) != ESP_OK)
      return fallback(color_count, result.date_key);

    uint8_t stored_number = 0;
    uint8_t stored_color = 0;
    int32_t stored_date = 0;
    const bool has_number =
        nvs_get_u8(handle, "number", &stored_number) == ESP_OK &&
        stored_number >= 1 && stored_number <= 9;
    const bool has_color =
        nvs_get_u8(handle, "color", &stored_color) == ESP_OK &&
        stored_color < color_count;
    nvs_get_i32(handle, "date", &stored_date);

    const bool calendar_day_changed =
        result.date_key > 0 && stored_date != result.date_key;
    if (!has_number || !has_color || calendar_day_changed) {
      stored_number = static_cast<uint8_t>(esp_random() % 9U + 1U);
      stored_color =
          static_cast<uint8_t>(esp_random() % static_cast<uint32_t>(color_count));
      stored_date = result.date_key;
      nvs_set_u8(handle, "number", stored_number);
      nvs_set_u8(handle, "color", stored_color);
      nvs_set_i32(handle, "date", stored_date);
      nvs_commit(handle);
    }

    result.number = stored_number;
    result.color_index = stored_color;
    nvs_close(handle);
    return result;
  }

private:
  static int current_date_key() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    if (now <= 0 || localtime_r(&now, &local) == nullptr) return 0;
    const int year = local.tm_year + 1900;
    if (year < 2024 || year > 2100) return 0;
    return year * 10000 + (local.tm_mon + 1) * 100 + local.tm_mday;
  }

  static DailyLuck fallback(int color_count, int date_key) {
    static int number = -1;
    static int color = -1;
    if (number < 0 || color < 0) {
      number = static_cast<int>(esp_random() % 9U) + 1;
      color = static_cast<int>(
          esp_random() % static_cast<uint32_t>(color_count));
    }
    return {
        .number = number,
        .color_index = color,
        .date_key = date_key,
        .calendar_synced = date_key > 0,
    };
  }
};

} // namespace app
