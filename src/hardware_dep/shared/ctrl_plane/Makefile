CC=gcc
CFLAGS=-Wall -pthread -std=c99 -g -O0
LIB=

ifneq ($(P4_GCC_OPTS),)
CFLAGS += $(P4_GCC_OPTS)
endif


all: dpdk_dummy_controller dpdk_portfwd_controller dpdk_l2fwd_controller dpdk_psa_l2fwd_controller dpdk_l3fwd_controller dpdk_smgw_controller dpdk_state-machine-register_controller dpdk_state-machine-table_controller dpdk_test_controller dpdk_16_1_controller dpdk_8_1_controller dpdk_32_1_controller dpdk_32_2_controller dpdk_32_3_controller dpdk_32_4_controller

table_test: dpdk_16_1_controller dpdk_8_1_controller dpdk_32_1_controller dpdk_32_2_controller dpdk_32_3_controller dpdk_32_4_controller


controllers_common: handlers.c controller.c messages.c sock_helpers.c threadpool.c fifo.c
	$(CC) $(CFLAGS) $(LIB) handlers.c controller.c messages.c sock_helpers.c threadpool.c fifo.c dpdk_ctrl_common.c -c
	ld -r handlers.o controller.o messages.o sock_helpers.o threadpool.o fifo.o dpdk_ctrl_common.o -o dpdk_ctrl.o

dpdk_dummy_controller: controllers_common dpdk_dummy_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_dummy_controller.c -o dpdk_dummy_controller

dpdk_portfwd_controller: controllers_common dpdk_portfwd_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_portfwd_controller.c -o dpdk_portfwd_controller

dpdk_l2fwd_controller: controllers_common dpdk_l2fwd_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_l2fwd_controller.c -o dpdk_l2fwd_controller

dpdk_test_controller: controllers_common dpdk_test_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_test_controller.c -o dpdk_test_controller

dpdk_16_1_controller: controllers_common dpdk_16_1_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_16_1_controller.c -o dpdk_16_1_controller
dpdk_8_1_controller: controllers_common dpdk_8_1_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_8_1_controller.c -o dpdk_8_1_controller
dpdk_32_1_controller: controllers_common dpdk_32_1_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_32_1_controller.c -o dpdk_32_1_controller
dpdk_32_2_controller: controllers_common dpdk_32_2_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_32_2_controller.c -o dpdk_32_2_controller
dpdk_32_3_controller: controllers_common dpdk_32_3_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_32_3_controller.c -o dpdk_32_3_controller
dpdk_32_4_controller: controllers_common dpdk_32_4_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_32_4_controller.c -o dpdk_32_4_controller
dpdk_psa_l2fwd_controller: controllers_common dpdk_psa_l2fwd_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_psa_l2fwd_controller.c -o dpdk_psa_l2fwd_controller

dpdk_l3fwd_controller: controllers_common dpdk_l3fwd_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_l3fwd_controller.c -o dpdk_l3fwd_controller

dpdk_smgw_controller: controllers_common dpdk_smgw_controller.c
	$(CC) $(CFLAGS) $(LIB) dpdk_ctrl.o dpdk_smgw_controller.c -o dpdk_smgw_controller

dpdk_state-machine-register_controller: handlers.c controller.c messages.c sock_helpers.c threadpool.c fifo.c dpdk_state-machine-register_controller.c
	$(CC) $(CFLAGS) $(LIB) handlers.c controller.c messages.c sock_helpers.c threadpool.c fifo.c dpdk_state-machine-register_controller.c -o dpdk_state-machine-register_controller

dpdk_state-machine-table_controller: handlers.c controller.c messages.c sock_helpers.c threadpool.c fifo.c dpdk_state-machine-table_controller.c
	$(CC) $(CFLAGS) $(LIB) handlers.c controller.c messages.c sock_helpers.c threadpool.c fifo.c dpdk_state-machine-table_controller.c -o dpdk_state-machine-table_controller

clean:
	rm -f handlers.o controller.o messages.o sock_helpers.o threadpool.o fifo.o dpdk_ctrl_common.o dpdk_ctrl_common.o dpdk_ctrl.o dpdk_portfwd_controller dpdk_l2fwd_controller dpdk_psa_l2fwd_controller dpdk_l3fwd_controller dpdk_smgw_controller dpdk_dummy_controller dpdk_state-machine-register_controller dpdk_state-machine-table_controller

