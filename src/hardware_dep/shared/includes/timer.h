#pragma once

struct timer_event_s {
    uint32_t lcore;
    uint32_t id;
    uint32_t repeat;
};

typedef struct timer_event_s timer_event_t;
typedef struct timer_s timer_t;

#define TIMER_RESOLUTION_CYCLES 20000000ULL


void init_timer(LCPARAMS, timer_t *timer_struct, uint32_t lcore);
timer_t* create_timer(LCPARAMS, uint32_t lcore);

void single_timer(uint32_t ms, uint32_t id, uint32_t lcore);
void periodic_timer(uint32_t ms, uint32_t id, uint32_t lcore);
void multiple_timer(uint32_t ms, uint32_t count, uint32_t id, uint32_t lcore);


void single_timer(uint32_t ms, uint32_t id);
void periodic_timer(uint32_t ms, uint32_t id);
void multiple_timer(uint32_t ms, uint32_t count, uint32_t id);