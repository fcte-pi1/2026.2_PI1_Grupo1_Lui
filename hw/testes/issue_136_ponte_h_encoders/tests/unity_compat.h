#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

static int _unity_tests_run = 0;
static int _unity_tests_failed = 0;

#define UNITY_BEGIN() do { _unity_tests_run = 0; _unity_tests_failed = 0; printf("========================================\n"); } while(0)
#define UNITY_END() (_unity_tests_failed == 0 ? (printf("----------------------------------------\n%d Tests %d Failures 0 Ignored\nOK\n", _unity_tests_run, _unity_tests_failed), 0) : (printf("----------------------------------------\n%d Tests %d Failures\nFAIL\n", _unity_tests_run, _unity_tests_failed), 1))

#define RUN_TEST(func) do { \
    _unity_tests_run++; \
    printf("[RUN ] %s ... ", #func); \
    func(); \
    printf("PASS\n"); \
} while(0)

#define TEST_ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        printf("\n  [FAIL] %s:%d: Expected TRUE: %s\n", __FILE__, __LINE__, #cond); \
        _unity_tests_failed++; \
        exit(1); \
    } \
} while(0)

#define TEST_ASSERT_FALSE(cond) TEST_ASSERT_TRUE(!(cond))

#define TEST_ASSERT_EQUAL(expected, actual) do { \
    long long _e = (long long)(expected); \
    long long _a = (long long)(actual); \
    if (_e != _a) { \
        printf("\n  [FAIL] %s:%d: Expected %lld, got %lld (%s == %s)\n", __FILE__, __LINE__, _e, _a, #expected, #actual); \
        _unity_tests_failed++; \
        exit(1); \
    } \
} while(0)

#define TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual) do { \
    float _d = (float)(delta); \
    float _e = (float)(expected); \
    float _a = (float)(actual); \
    if (fabsf(_e - _a) > _d) { \
        printf("\n  [FAIL] %s:%d: Expected %.4f +/- %.4f, got %.4f\n", __FILE__, __LINE__, _e, _d, _a); \
        _unity_tests_failed++; \
        exit(1); \
    } \
} while(0)
