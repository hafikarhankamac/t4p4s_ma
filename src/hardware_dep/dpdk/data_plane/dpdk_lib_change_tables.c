// Copyright 2018 Eotvos Lorand University, Budapest, Hungary
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


// This file is included directly from `dpdk_lib.c`.

void change_replica(int socketid, int tid, int replica) {
    for (unsigned lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
        if (rte_lcore_is_enabled(lcore_id) == 0) continue;

        int core_socketid = rte_lcore_to_socket_id(lcore_id);
        if (core_socketid != socketid) continue;

        struct lcore_conf* qconf = &lcore_conf[lcore_id];
        qconf->state.tables[tid] = state[socketid].tables[tid][replica]; // TODO should this be atomic?
        state[socketid].active_replica[tid] = replica;

        // debug("    : " T4LIT(%d,core) "@" T4LIT(%d,socket) " uses table replica " T4LIT(%s,table) "#" T4LIT(%d) "\n", lcore_id, socketid, state[socketid].tables[tid][replica]->name, replica);
    }
}

#define CHANGE_TABLE(fun, par...) \
{ \
    { \
        int current_replica = state[socketid].active_replica[tableid]; \
        int next_replica = (current_replica+1)%NB_REPLICA; \
        fun(state[socketid].tables[tableid][next_replica], par); \
        change_replica(socketid, tableid, next_replica); \
        usleep(TABCHANGE_SLEEP_MICROS); \
        for (int current_replica = 0; current_replica < NB_REPLICA; current_replica++) { \
            if (current_replica != next_replica) { \
                fun(state[socketid].tables[tableid][current_replica], par); \
            } \
        } \
    } \
}

extern char* get_entry_action_name(void* entry);

#define CHANGE_TABLE_NOREPLICA(fun, par...) \
{ \
    { \
        int current_replica = state[socketid].active_replica[tableid]; \
        fun(state[socketid].tables[tableid][current_replica], par); \
    } \
}

#define FORALLNUMANODES(txt1, txt2, b) \
    for (int socketid = 0; socketid < NB_SOCKETS; socketid++) \
        if (state[socketid].tables[0][0] != NULL) { \
            dbg_bytes(key, state[socketid].tables[tableid][0]->entry.key_size, " " T4LIT(ctl>,incoming) " " T4LIT(txt1,action) " " T4LIT(%s,table) txt2 ": " T4LIT(%s,action) " <- ", table_config[tableid].name, get_entry_action_name(value)); \
            b \
        }

#define FORALLNUMANODES_NOKEY(txt1, txt2, b) \
    for (int socketid = 0; socketid < NB_SOCKETS; socketid++) \
        if (state[socketid].tables[0][0] != NULL) { \
            b \
        }

void exact_add_promote(int tableid, uint8_t* key, uint8_t* value) {
    FORALLNUMANODES(Add, "/" T4LIT(exact), CHANGE_TABLE(exact_add, key, value))
}
void lpm_add_promote(int tableid, uint8_t* key, uint8_t depth, uint8_t* value) {
    FORALLNUMANODES(Add, "/" T4LIT(LPM), CHANGE_TABLE(lpm_add, key, depth, value))
}
void ternary_add_promote(int tableid, uint8_t* key, uint8_t* mask, uint8_t* value) {
    FORALLNUMANODES(Add, "/" T4LIT(ternary), CHANGE_TABLE(ternary_add, key, mask, value))
}
void table_setdefault_promote(int tableid, uint8_t* value) {
    FORALLNUMANODES_NOKEY(Set default, "on table", CHANGE_TABLE(table_set_default_action, value))
}

#define CHANGE_TABLE_NOREPLICA_SEQ(fun, par...) \
{ \
    { \
        int current_replica = state[socketid].active_replica[tableid]; \
        for (uint64_t idx = 0; idx < nr_entries; idx++) { \
            fun(state[socketid].tables[tableid][current_replica], par); \
        } \
    } \
}

#define CHANGE_TABLE_SEQ(fun, par...) \
{ \
    { \
        int current_replica = state[socketid].active_replica[tableid]; \
        int next_replica = (current_replica+1)%NB_REPLICA; \
        for (uint64_t idx = 0; idx < nr_entries; idx++) { \
            fun(state[socketid].tables[tableid][next_replica], par); \
        } \
        change_replica(socketid, tableid, next_replica); \
        usleep(TABCHANGE_SLEEP_MICROS); \
        for (int current_replica = 0; current_replica < NB_REPLICA; current_replica++) { \
            if (current_replica != next_replica) { \
                for (uint64_t idx = 0; idx < nr_entries; idx++) { \
                    fun(state[socketid].tables[tableid][current_replica], par); \
                } \
            } \
        } \
    } \
}

//void exact_add_promote_multiple(int tableid, uint8_t** keys, uint8_t* value, uint64_t nr_entries) 
//{
//    FORALLNUMANODES(add, to exact table, CHANGE_TABLE_SEQ(exact_add, keys[idx], value))
//}
//
//void ternary_add_promote_multiple(int tableid, uint8_t** keys, uint8_t** masks, uint8_t* value, uint64_t nr_entries)
//{
//    FORALLNUMANODES(add, to ternary table, CHANGE_TABLE_SEQ(ternary_add, keys[idx], masks[idx], value))
//}
//
//void lpm_add_promote_multiple(int tableid, uint8_t** keys, uint8_t* depths, uint8_t* value, uint64_t nr_entries)
//{
//    FORALLNUMANODES(add, to lpm table, CHANGE_TABLE_SEQ(lpm_add, keys[idx], depths[idx], value))
//}
