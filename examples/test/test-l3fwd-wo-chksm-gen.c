// SPDX-License-Identifier: Apache-2.0
// Copyright 2018 Eotvos Lorand University, Budapest, Hungary

#include "test.h"

fake_cmd_t t4p4s_testcase_test[][RTE_MAX_LCORE] = {
    {
        FSLEEP(INIT_WAIT_CONTROLPLANE_LONG_MILLIS),
        {FAKE_PKT, 0, 0, ETH("DDDDDDDD0000", ETH01, "000000000000000000000000", "12345678", "96000102"), NO_CTL_REPLY, 1, ETH("AABBBBAA0001", "EEDDDDAA0001", "0000000000000000FF000000", "12345678", "96000102")},
        // TODO improve the following test case
        // {FAKE_PKT, 0, 0, ETH(ETH1A, ETH04, IPV4_0000), NO_CTL_REPLY, 18, ETH(ETH01, ETH1A, IPV4_0000)},

        FEND,
    },

    {
        FEND,
    },
};

testcase_t t4p4s_test_suite[MAX_TESTCASES] = {
    { "test",           &t4p4s_testcase_test },
    TEST_SUITE_END,
};
