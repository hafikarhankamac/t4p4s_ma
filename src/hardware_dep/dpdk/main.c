/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2015 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
// A large portion of the code in this file comes from
// main.c in the l3fwd example of DPDK 2.2.0.

#include "dpdk_lib.h"
#include <rte_ethdev.h>

#include "gen_include.h"

#ifndef T4P4S_NIC_VARIANT
#error The NIC variant is undefined
#endif

#ifdef T4P4S_SUPPRESS_EAL
    #include <unistd.h>
    #include <stdio.h>
#endif


// TODO from...
extern void initialize_args(int argc, char **argv);
extern void initialize_nic();
extern int init_tables();
extern int init_memories();

extern int flush_tables();

extern int launch_count();
extern void t4p4s_abnormal_exit(int retval, int idx);
extern void t4p4s_pre_launch(int idx);
extern void t4p4s_post_launch(int idx);
extern int t4p4s_normal_exit();

// TODO from...
extern void init_control_plane();

// defined in the generated file dataplane.c
extern void handle_packet(packet_descriptor_t* pd, lookup_table_t** tables, parser_state_t* pstate, uint32_t portid);

// defined separately for each example
extern bool core_is_working(struct lcore_data* lcdata);
extern bool receive_packet(packet_descriptor_t* pd, struct lcore_data* lcdata, unsigned pkt_idx);
extern void free_packet(packet_descriptor_t* pd);
extern bool is_packet_handled(packet_descriptor_t* pd, struct lcore_data* lcdata);
extern void init_storage();
extern void main_loop_pre_rx(struct lcore_data* lcdata);
extern void main_loop_post_rx(struct lcore_data* lcdata);
extern void main_loop_post_single_rx(struct lcore_data* lcdata, bool got_packet);
extern uint32_t get_portid(struct lcore_data* lcdata, unsigned queue_idx);
extern void main_loop_rx_group(struct lcore_data* lcdata, unsigned queue_idx);
extern unsigned get_pkt_count_in_group(struct lcore_data* lcdata);
extern unsigned get_queue_count(struct lcore_data* lcdata);
extern void send_single_packet(struct lcore_data* lcdata, packet_descriptor_t* pd, packet* pkt, int egress_port, int ingress_port);
extern void send_broadcast_packet(struct lcore_data* lcdata, packet_descriptor_t* pd, int egress_port, int ingress_port);
extern struct lcore_data init_lcore_data();
extern packet* clone_packet(packet* pd, struct rte_mempool* mempool);
extern void init_parser_state(parser_state_t*);

//=============================================================================

extern uint32_t get_port_mask();
extern uint8_t get_port_count();

void get_broadcast_port_msg(char result[256], int ingress_port) {
    uint8_t nb_ports = get_port_count();
    uint32_t port_mask = get_port_mask();

    char* result_ptr = result;
    bool is_first_printed_port = true;
    for (uint8_t portidx = 0; portidx < RTE_MAX_ETHPORTS; ++portidx) {
        if (portidx == ingress_port) {
           continue;
        }

        bool is_port_disabled = (port_mask & (1 << portidx)) == 0;
        if (is_port_disabled)   continue;

        int printed_bytes = sprintf(result_ptr, "%s" T4LIT(%d,port), is_first_printed_port ? "" : ", ", portidx);
        result_ptr += printed_bytes;
        is_first_printed_port = false;
    }
}


void broadcast_packet(struct lcore_data* lcdata, packet_descriptor_t* pd, int egress_port, int ingress_port)
{
    uint8_t nb_ports = get_port_count();
    uint32_t port_mask = get_port_mask();

    uint8_t nb_port = 0;
    for (uint8_t portidx = 0; nb_port < nb_ports - 1 && portidx < RTE_MAX_ETHPORTS; ++portidx) {
        if (portidx == ingress_port) {
           continue;
        }

        bool is_port_disabled = (port_mask & (1 << portidx)) == 0;
        if (is_port_disabled)   continue;

        packet* pkt_out = (nb_port < nb_ports) ? clone_packet(pd->wrapper, lcdata->mempool) : pd->wrapper;
        send_single_packet(lcdata, pd, pkt_out, egress_port, ingress_port);

        nb_port++;
    }

    if (unlikely(nb_port != nb_ports - 1)) {
        debug(" " T4LIT(!!!!,error) " " T4LIT(Wrong port count,error) ": " T4LIT(%d) " ports should be present, but only " T4LIT(%d) " found\n", nb_ports, nb_port);
    }
}

/* Enqueue a single packet, and send burst if queue is filled */
void send_packet(struct lcore_data* lcdata, packet_descriptor_t* pd, int egress_port, int ingress_port)
{
    uint32_t lcore_id = rte_lcore_id();
    struct rte_mbuf* mbuf = (struct rte_mbuf *)pd->wrapper;

    if (unlikely(egress_port == T4P4S_BROADCAST_PORT)) {
        #ifdef T4P4S_DEBUG
            char ports_msg[256];
            get_broadcast_port_msg(ports_msg, ingress_port);
            dbg_bytes(rte_pktmbuf_mtod(mbuf, uint8_t*), rte_pktmbuf_pkt_len(mbuf), "   " T4LIT(<<,outgoing) " " T4LIT(Broadcasting,outgoing) " packet from port " T4LIT(%d,port) " to all other ports (%s) (" T4LIT(%d) " bytes): ", ingress_port, ports_msg, rte_pktmbuf_pkt_len(mbuf));
        #endif
        broadcast_packet(lcdata, pd, egress_port, ingress_port);
    } else {
        dbg_bytes(rte_pktmbuf_mtod(mbuf, uint8_t*), rte_pktmbuf_pkt_len(mbuf), "   " T4LIT(<<,outgoing) " " T4LIT(Emitting,outgoing) " packet on port " T4LIT(%d,port) " (" T4LIT(%d) " bytes): ", egress_port, rte_pktmbuf_pkt_len(mbuf));
        send_single_packet(lcdata, pd, pd->wrapper, egress_port, ingress_port);
    }
}

void do_single_tx(struct lcore_data* lcdata, packet_descriptor_t* pd, unsigned queue_idx, unsigned pkt_idx)
{
    if (unlikely(GET_INT32_AUTO_PACKET(pd, header_instance_all_metadatas, field_standard_metadata_t_drop))) {
        debug(" " T4LIT(XXXX,status) " " T4LIT(Dropping,status) " packet\n");
        free_packet(pd);
    } else {
        debug(" " T4LIT(<<<<,outgoing) " " T4LIT(Egressing,outgoing) " packet\n");

        int egress_port = extract_egress_port(pd);
        int ingress_port = extract_ingress_port(pd);

        send_packet(lcdata, pd, egress_port, ingress_port);
    }
}

void do_single_rx(struct lcore_data* lcdata, packet_descriptor_t* pd, unsigned queue_idx, unsigned pkt_idx)
{
    bool got_packet = receive_packet(pd, lcdata, pkt_idx);

    if (got_packet) {
	    if (likely(is_packet_handled(pd, lcdata))) {
	        init_parser_state(&(lcdata->conf->state.parser_state));
            handle_packet(pd, lcdata->conf->state.tables, &(lcdata->conf->state.parser_state), get_portid(lcdata, queue_idx));
            do_single_tx(lcdata, pd, queue_idx, pkt_idx);
        }
    }

    main_loop_post_single_rx(lcdata, got_packet);
}

void do_rx(struct lcore_data* lcdata, packet_descriptor_t* pd)
{
    unsigned queue_count = get_queue_count(lcdata);
    for (unsigned queue_idx = 0; queue_idx < queue_count; queue_idx++) {
        main_loop_rx_group(lcdata, queue_idx);

        unsigned pkt_count = get_pkt_count_in_group(lcdata);
        for (unsigned pkt_idx = 0; pkt_idx < pkt_count; pkt_idx++) {
            do_single_rx(lcdata, pd, queue_idx, pkt_idx);
        }
    }
}

bool dpdk_main_loop()
{
    struct lcore_data lcdata = init_lcore_data();
    if (!lcdata.is_valid) {
    	debug("lcore data is invalid, exiting\n");
    	return false;
    }

    packet_descriptor_t pd;
    init_dataplane(&pd, lcdata.conf->state.tables);

    //uint64_t rx_cnt = 0;
    while (core_is_working(&lcdata)) {
        main_loop_pre_rx(&lcdata);

        do_rx(&lcdata, &pd);

        main_loop_post_rx(&lcdata);

        /*rx_cnt++;
        if (unlikely(rx_cnt % 1000000 == 0))
        {
            print_rte_xstats(1);
            print_rte_xstats(2);
        }*/
    }

    return lcdata.is_valid;
}


static int
launch_one_lcore(__attribute__((unused)) void *dummy)
{
    bool success = dpdk_main_loop();
    return success ? 0 : -1;
}

int launch_dpdk()
{
    rte_eal_mp_remote_launch(launch_one_lcore, NULL, CALL_MASTER);

    unsigned lcore_id;
    RTE_LCORE_FOREACH_SLAVE(lcore_id) {
        if (rte_eal_wait_lcore(lcore_id) < 0)
            return -1;
    }

    return 0;
}

// https://doc.dpdk.org/guides/prog_guide/poll_mode_drv.html
void print_rte_xstats(uint32_t port_id)
{
    struct rte_eth_xstat_name *xstats_names;
    uint64_t *values;
    int len, i;

    printf("========== xstats for port = %u ==========\n", port_id);
    /* Get number of stats */
    len = rte_eth_xstats_get_names_by_id(port_id, NULL, 0, NULL);
    printf("len=%d\n", len);
    if (len < 0) {
        printf("Cannot get xstats count\n");
    }

    xstats_names = malloc(sizeof(struct rte_eth_xstat_name) * len);
    if (xstats_names == NULL) {
        printf("Cannot allocate memory for xstat names\n");
    }

    /* Retrieve xstats names, passing NULL for IDs to return all statistics */
    int xstatsnamelen = rte_eth_xstats_get_names_by_id(port_id, xstats_names, len, NULL);// NULL, len);
    printf("rte_eth_xstats_get_names_by_id(port_id, xstats_names, NULL, len) =%d\n", xstatsnamelen);
    if (len != xstatsnamelen) {
        printf("Cannot get xstat names\n");
    }

    values = malloc(sizeof(values) * len);
    if (values == NULL) {
        printf("Cannot allocate memory for xstats\n");
    }

    /* Getting xstats values */
    if (len != rte_eth_xstats_get_by_id(port_id, NULL, values, len)) {
        printf("Cannot get xstat values\n");
    }

    /* Print all xstats names and values */
    for (i = 0; i < len; i++) {
        printf("%s: %"PRIu64"\n", xstats_names[i].name, values[i]);
    }
    printf("==========================================\n");
    fflush(stdout);
}

int main(int argc, char** argv)
{
    debug("Initializing switch\n");

    initialize_args(argc, argv);
    initialize_nic();

    int launch_count2 = launch_count();
    for (int i = 0; i < launch_count2; ++i) {
        debug("Initializing execution\n");

        init_tables();
        init_storage();

        init_memories();
        debug(" " T4LIT(::::,incoming) " Initializing control plane connection\n");
        init_control_plane();
        debug(" :::: Initializing storage\n");

        t4p4s_pre_launch(i);

        int retval = launch_dpdk();
        if (retval < 0) {
            t4p4s_abnormal_exit(retval, i);
            return retval;
        }

        t4p4s_post_launch(i);

        flush_tables();
    }
    /*print_rte_xstats(1);
    print_rte_xstats(2);*/

    return t4p4s_normal_exit();
}
