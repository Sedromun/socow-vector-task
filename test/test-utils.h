#pragma once

#include "element.h"
#include "socow-vector.h"

#include <gtest/gtest.h>

#include <string>

template class socow_vector<int, 3>;

template class socow_vector<element, 3>;
using container = socow_vector<element, 3>;

template <class... Ts>
class immutable_guard {
public:
  explicit immutable_guard(const Ts&... objects) : guards(objects...) {}

  immutable_guard(const immutable_guard&) = delete;

private:
  std::tuple<immutable_guard<Ts>...> guards;
};

template <size_t SMALL_SIZE>
class immutable_guard<socow_vector<element, SMALL_SIZE>> {
public:
  explicit immutable_guard(const socow_vector<element, SMALL_SIZE>& a)
      : a(a),
        old_capacity(a.capacity()),
        old_data(a.data()),
        old_a(a) {
    old_a.data();
  }

  immutable_guard(const immutable_guard&) = delete;

  ~immutable_guard() {
    destruct();
  }

private:
  void destruct() const {
    ASSERT_EQ(old_a.size(), a.size());
    ASSERT_EQ(old_capacity, a.capacity());
    ASSERT_EQ(old_data, a.data());
    for (size_t i = 0; i < old_a.size(); ++i) {
      ASSERT_EQ(old_a[i], a[i]);
    }
  }

private:
  const socow_vector<element, SMALL_SIZE>& a;
  size_t old_capacity;
  const element* old_data;
  socow_vector<element, SMALL_SIZE> old_a;
};

class base_test : public ::testing::Test {
protected:
  void SetUp() override {
    element::reset_counters();
    element::set_copy_throw_countdown(0);
    element::set_swap_throw_countdown(0);
  }

  void TearDown() override {
    element::assert_no_instances();
  }
};

template <size_t SMALL_SIZE>
void expect_static_storage(const socow_vector<element, SMALL_SIZE>& a) {
  EXPECT_EQ(SMALL_SIZE, a.capacity());
  const element* data = a.data();
  bool is_static = std::less_equal<const void*>{}(&a, data) && std::greater<const void*>{}(&a + 1, data);
  EXPECT_TRUE(is_static);
}

template <size_t SMALL_SIZE>
void expect_empty_storage(const socow_vector<element, SMALL_SIZE>& a) {
  EXPECT_TRUE(a.empty());
  EXPECT_EQ(0, a.size());
  expect_static_storage(a);
}

template <std::ranges::sized_range Actual,
          std::ranges::sized_range Expected = std::initializer_list<std::ranges::range_value_t<Actual>>>
void expect_eq(const Actual& actual, const Expected& expected) {
  fault_injection_disable dg;

  EXPECT_EQ(expected.size(), actual.size());

  auto expected_first = expected.begin();
  auto expected_last = expected.end();
  auto actual_first = actual.begin();
  auto actual_last = actual.end();

  while (true) {
    if (expected_first == expected_last || actual_first == actual_last) {
      break;
    }

    EXPECT_EQ(*actual_first, *expected_first);
    ++expected_first;
    ++actual_first;
  }

  EXPECT_TRUE(expected_first == expected_last);
  EXPECT_TRUE(actual_first == actual_last);
}

template <typename C>
class strong_exception_safety_guard {
public:
  explicit strong_exception_safety_guard(const C& c) noexcept : ref(c), expected((fault_injection_disable{}, c)) {}

  strong_exception_safety_guard(const strong_exception_safety_guard&) = delete;

  ~strong_exception_safety_guard() {
    if (std::uncaught_exceptions() > 0) {
      expect_eq(expected, ref);
    }
  }

private:
  const C& ref;
  C expected;
};
