/**
 * @file    test_sensor_dispatch.c
 * @brief   Unit tests for the sensor dispatch table scheduling logic.
 *
 * The dispatch table struct and tick-based loop are defined locally so
 * this test compiles without any embedded project headers.  It verifies
 * the four scheduling invariants required of SensorDataUpdateTask:
 *
 *   1. always-poll  — NULL error_flag: sensor is polled once per interval
 *   2. error-skip   — *error_flag != 0: poll_fn is never called
 *   3. poll-interval — poll_fn is NOT called more often than poll_interval_ms
 *   4. error-clears  — once *error_flag drops to 0 the sensor resumes polling
 */

#if LV_BUILD_TEST

#include "unity/unity.h"
#include <stdint.h>
#include <stddef.h>

/* -----------------------------------------------------------------------
 * Minimal dispatch table types — mirrors user_SensUpdateTask.h so this
 * test is self-contained and does not depend on the embedded headers.
 * ----------------------------------------------------------------------- */
#define SENSOR_TASK_TICK_MS  50U

typedef void (*sensor_poll_fn_t)(void);

typedef struct {
    uint32_t          poll_interval_ms;
    sensor_poll_fn_t  poll_fn;
    uint8_t          *error_flag;   /* NULL = always run regardless of error */
    uint32_t          last_tick;
} sensor_dispatch_entry_t;

/* -----------------------------------------------------------------------
 * Test infrastructure
 * ----------------------------------------------------------------------- */
static int      g_poll_count;
static uint32_t g_mock_tick;

static void mock_poll(void) { g_poll_count++; }

/**
 * @brief Run one iteration of the dispatch loop against a single entry.
 *
 * Mirrors the inner loop body from SensorDataUpdateTask:
 *
 *   if (error_flag && *error_flag)               skip
 *   if ((now - last_tick) >= poll_interval_ms)   call poll_fn; update last_tick
 */
static void dispatch_tick(sensor_dispatch_entry_t *e)
{
    uint32_t now = g_mock_tick;
    if (e->error_flag && *e->error_flag) { return; }
    if ((uint32_t)(now - e->last_tick) >= e->poll_interval_ms) {
        e->poll_fn();
        e->last_tick = now;
    }
}

void setUp(void)
{
    g_poll_count = 0;
    g_mock_tick  = 0U;
}

void tearDown(void) {}

/* -----------------------------------------------------------------------
 * Test 1: always-poll
 *
 * A sensor with error_flag=NULL must be polled every time the interval
 * elapses, regardless of any external state.
 * ----------------------------------------------------------------------- */
void test_dispatch_always_poll(void)
{
    sensor_dispatch_entry_t e = { 500U, mock_poll, NULL, 0U };

    /* t=0: last_tick=0, (0-0)=0 < 500 → no fire */
    g_mock_tick = 0U;
    dispatch_tick(&e);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, g_poll_count, "should not fire before interval");

    /* t=500: (500-0)=500 >= 500 → fire once, last_tick updates to 500 */
    g_mock_tick = 500U;
    dispatch_tick(&e);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, g_poll_count, "should fire at interval boundary");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(500U, e.last_tick, "last_tick must update after fire");

    /* t=1000: (1000-500)=500 >= 500 → fire again */
    g_mock_tick = 1000U;
    dispatch_tick(&e);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, g_poll_count, "should fire again at next interval");
}

/* -----------------------------------------------------------------------
 * Test 2: error-skip
 *
 * When *error_flag != 0, poll_fn must never be called regardless of how
 * much time has elapsed.
 * ----------------------------------------------------------------------- */
void test_dispatch_error_skip(void)
{
    uint8_t error_flag = 1U;
    sensor_dispatch_entry_t e = { 500U, mock_poll, &error_flag, 0U };

    /* Even well past the poll interval, error blocks every call */
    g_mock_tick = 0U;    dispatch_tick(&e);
    g_mock_tick = 500U;  dispatch_tick(&e);
    g_mock_tick = 1000U; dispatch_tick(&e);
    g_mock_tick = 5000U; dispatch_tick(&e);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, g_poll_count,
        "poll_fn must not be called while error_flag is set");
}

/* -----------------------------------------------------------------------
 * Test 3: poll-interval
 *
 * poll_fn must not be called more often than once per poll_interval_ms,
 * even when dispatch_tick is called on every 50 ms task tick.
 * ----------------------------------------------------------------------- */
void test_dispatch_poll_interval(void)
{
    sensor_dispatch_entry_t e = { 1000U, mock_poll, NULL, 0U };

    /* Drive 50 ms ticks from t=0 to t=950 — interval has not yet elapsed */
    uint32_t t;
    for (t = 0U; t < 1000U; t += SENSOR_TASK_TICK_MS) {
        g_mock_tick = t;
        dispatch_tick(&e);
    }
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, g_poll_count,
        "must not fire before poll_interval_ms has elapsed");

    /* t=1000: exactly one interval elapsed → first fire */
    g_mock_tick = 1000U;
    dispatch_tick(&e);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, g_poll_count,
        "must fire exactly once at poll_interval_ms");

    /* t=1050 … 1950: within the next interval → no extra fire */
    for (t = 1050U; t < 2000U; t += SENSOR_TASK_TICK_MS) {
        g_mock_tick = t;
        dispatch_tick(&e);
    }
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, g_poll_count,
        "must not fire again before next poll_interval_ms boundary");
}

/* -----------------------------------------------------------------------
 * Test 4: error-clears
 *
 * After *error_flag transitions from 1 to 0 the sensor must resume
 * polling.  The first poll occurs as soon as (now - last_tick) >= interval
 * (which is immediately true if the error lasted longer than interval).
 * ----------------------------------------------------------------------- */
void test_dispatch_error_clears(void)
{
    uint8_t error_flag = 1U;
    sensor_dispatch_entry_t e = { 500U, mock_poll, &error_flag, 0U };

    /* Let time pass while error is set — nothing should fire */
    g_mock_tick = 0U;    dispatch_tick(&e);
    g_mock_tick = 500U;  dispatch_tick(&e);
    g_mock_tick = 1000U; dispatch_tick(&e);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, g_poll_count,
        "should not poll while error is set");

    /* Clear the error — next tick must resume polling immediately
     * (last_tick=0, now=1000, 1000-0=1000 >= 500) */
    error_flag = 0U;
    g_mock_tick = 1000U;
    dispatch_tick(&e);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, g_poll_count,
        "should resume polling immediately after error clears");
}

#endif /* LV_BUILD_TEST */
