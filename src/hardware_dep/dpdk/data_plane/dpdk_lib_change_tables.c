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

#define CHANGE_TABLE(fun, par...) \
{ \
    { \
        fun(state[socketid].tables[tableid], par); \
    } \
}

extern char* get_entry_action_name(void* entry);

#define CHANGE_TABLE_NOREPLICA(fun, par...) \
{ \
    { \
        fun(state[socketid].tables[tableid], par); \
    } \
}

#define FORALLNUMANODES(txt1, txt2, b) \
    for (int socketid = 0; socketid < NB_SOCKETS; socketid++) \
        if (state[socketid].tables[0] != NULL) { \
            dbg_bytes(key, state[socketid].tables[tableid][0]->entry.key_size, " " T4LIT(ctl>,incoming) " " T4LIT(txt1,action) " " T4LIT(%s,table) txt2 ": " T4LIT(%s,action) " <- ", table_config[tableid].name, get_entry_action_name(value)); \
            b \
        }

#define FORALLNUMANODES_NOKEY(txt1, txt2, b) \
    for (int socketid = 0; socketid < NB_SOCKETS; socketid++) \
        if (state[socketid].tables[0] != NULL) { \
            b \
        }

void exact_change_promote(int tableid, uint8_t* key, uint8_t* value) {
    FORALLNUMANODES(Add, "/" T4LIT(exact), CHANGE_TABLE(exact_change, key, value))
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
        for (uint64_t idx = 0; idx < nr_entries; idx++) { \
            fun(state[socketid].tables[tableid], par); \
        } \
    } \
}

#define CHANGE_TABLE_SEQ(fun, par...) \
{ \
    { \
        for (uint64_t idx = 0; idx < nr_entries; idx++) { \
            fun(state[socketid].tables[tableid], par); \
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
