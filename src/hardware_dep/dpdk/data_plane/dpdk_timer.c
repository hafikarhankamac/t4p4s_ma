#include "timer_extern.h"
#include "dpdk_model_event.h"

#include <rte_timer.h>
#include <rte_malloc.h>

extern struct lcore_conf lcore_conf[RTE_MAX_LCORE];
extern bool enabled_timer_module;

void finalize_timer(struct rte_timer *tim, timer_event_t *timer_event)
{
    rte_timer_stop(tim);
    rte_free(tim);
    rte_free(timer_event);
}

void timer_callback_single(struct rte_timer *tim, void *arg)
{
    timer_event_t *timer_event = (timer_event_t*) arg;
    timer_event->repeat++;
    enque_event(lcore_conf[timer_event->lcore].state.event_queue, TIMER, timer_event->id);
    finalize_timer(tim, timer_event);
}

void timer_callback_periodic(struct rte_timer *tim, void *arg)
{
    timer_event_t *timer_event = (timer_event_t*) arg;
    timer_event->repeat++;
    enque_event(lcore_conf[timer_event->lcore].state.event_queue, TIMER, timer_event->id);
}

void timer_callback_multiple(struct rte_timer *tim, void *arg)
{
    timer_event_t *timer_event = (timer_event_t*) arg;
    timer_event->repeat--;
    if (timer_event->repeat <= 0) {
        finalize_timer(tim, timer_event);
    }

    enque_event(lcore_conf[timer_event->lcore].state.event_queue, TIMER, timer_event->id);
}


void single_timer_lcore(uint32_t ms, uint32_t id, uint32_t lcore)
{
    timer_event_t *timer_event = rte_malloc("timer", sizeof(timer_event_t), 0);
    struct rte_timer *rtetimer = rte_malloc("rte_timer", sizeof(struct rte_timer), 0);
    timer_event->repeat = 0;
    timer_event->lcore = lcore;
    timer_event->id = id;
    rte_timer_init(rtetimer);
    rte_timer_reset(rtetimer, hz_millis * ms, SINGLE, lcore, timer_callback_single, (void*) timer_event);
}

void periodic_timer_lcore(uint32_t ms, uint32_t id, uint32_t lcore)
{
    timer_event_t *timer_event = rte_malloc("timer", sizeof(timer_event_t), 0);
    struct rte_timer *rtetimer = rte_malloc("rte_timer", sizeof(struct rte_timer), 0);
    timer_event->repeat = 0;
    timer_event->lcore = lcore;
    timer_event->id = id;
    rte_timer_init(rtetimer);
    rte_timer_reset(rtetimer, hz_millis * ms, PERIODICAL, lcore, timer_callback_periodic, (void*) timer_event);
}

void multiple_timer_lcore(uint32_t ms, uint32_t count, uint32_t id, uint32_t lcore)
{
    timer_event_t *timer_event = rte_malloc("timer", sizeof(timer_event_t), 0);
    struct rte_timer *rtetimer = rte_malloc("rte_timer", sizeof(struct rte_timer), 0);
    timer_event->repeat = count;
    timer_event->lcore = lcore;
    timer_event->id = id;
    rte_timer_init(rtetimer);
    rte_timer_reset(rtetimer, hz_millis * ms, PERIODICAL, lcore, timer_callback_multiple, (void*) timer_event);
}

void single_timer(uint32_t ms, uint32_t id)
{
    single_timer_lcore(ms, id, rte_lcore_id());
}

void periodic_timer(uint32_t ms, uint32_t id)
{
    periodic_timer_lcore(ms, id, rte_lcore_id());
}

void multiple_timer(uint32_t ms, uint32_t count, uint32_t id)
{
    multiple_timer_lcore(ms, count, id, rte_lcore_id());
}

void timer_init(uint64_t hz) 
{
	hz_millis = hz / 1000;
}
