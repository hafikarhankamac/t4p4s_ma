#include "timer.h"
#include "dpdk_model_event.h"

#include <rte_timer.h>
#include <rte_malloc.h>

extern struct lcore_conf *lcore_conf;

const static uint64_t hz_milli = rte_get_timer_hz()/1000;

void finalize_timer(struct rte_timer *tim, timer_event_t *timer_event)
{
    rte_timer_stop(tim);
    rte_free(tim);
    rte_free(timer_event);
}

void timer_callback_single(struct rte_timer *tim, void *arg)
{
    timer_event_t timer_event = (timer_event_t*) arg;
    timer_event->repeat++;
    enque_event(lcore_conf[timer_event->lcore].event_queue, event_e.TIMER, timer_event->id);
    finalize_timer(tim);
}

void timer_callback_periodic(struct rte_timer *tim, void *arg)
{
    timer_event_t timer_event = (timer_event_t*) arg;
    timer_event->repeat++;
    enque_event(lcore_conf[timer_event->lcore].event_queue, event_e.TIMER, timer_event->id);
}

void timer_callback_multiple(struct rte_timer *tim, void *arg)
{
    timer_event_t timer_event = (timer_event_t*) arg;
    timer_event->repeat--;
    if (timer_event->repeat <= 0) {
        finalize_timer(tim);
    }

    enque_event(lcore_conf[timer_event->lcore].event_queue, event_e.TIMER, timer_event->id);
}


void single_timer(uint32_t ms, uint32_t id, uint32_t lcore)
{
    timer_event_t *timer_event = rte_malloc("timer", sizeof(timer_event_t), 0);
    struct rte_timer *rtetimer = rte_malloc("rte_timer", sizeof(rte_timer), 0);
    timer_event->repeat = 0;
    timer_event->lcore = lcore;
    rte_timer_init(rtetimer);
    rte_timer_reset(rtetimer, timer->hz_millis * ms, SINGLE, timer->lcore, timer_callback_single, (void*) id);
}

void periodic_timer(uint32_t ms, uint32_t id, uint32_t lcore)
{
    timer_event_t *timer_event = rte_malloc("timer", sizeof(timer_event_t), 0);
    struct rte_timer *rtetimer = rte_malloc("rte_timer", sizeof(rte_timer), 0);
    timer_event->repeat = 0;
    timer_event->lcore = lcore;
    rte_timer_init(rtetimer);
    rte_timer_reset(rtetimer, timer->hz_millis * ms, PERIODIC, timer->lcore, timer_callback_periodic, (void*) id);
}

void multiple_timer(uint32_t ms, uint32_t count, uint32_t id, uint32_t lcore)
{
    timer_event_t *timer_event = rte_malloc("timer", sizeof(timer_event_t), 0);
    struct rte_timer *rtetimer = rte_malloc("rte_timer", sizeof(rte_timer), 0);
    timer_event->repeat = count;
    timer_event->lcore = lcore;
    rte_timer_init(rtetimer);
    rte_timer_reset(rtetimer, timer->hz_millis * ms, PERIODIC, timer->lcore, timer_callback_multiple, (void*) id);
}

void single_timer(uint32_t ms, uint32_t id)
{
    single_timer(ms, id, rte_lcore_id());
}

void periodic_timer(uint32_t ms, uint32_t id)
{
    periodic_timer(ms, id, rte_lcore_id());
}

void multiple_timer(uint32_t ms, uint32_t count, uint32_t id)
{
    multiple_timer(ms, count, id, rte_lcore_id());
}