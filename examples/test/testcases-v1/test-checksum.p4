
#include <core.p4>
#include <v1model.p4>
#include "../../include/std_headers.p4"

struct metadata {
}

struct headers {
    @name(".ethernet")
    ethernet_t         ethernet;
    @name(".ipv4")
    ipv4_options_t     ipv4;
    @name(".tcp")
    tcp_options_t      tcp;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    ipv4_t       ipv4_novarbit;
    varbits320_t ipv4_varbit;
    tcp_t        tcp_novarbit;
    varbits320_t tcp_varbit;

    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }

    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract(ipv4_novarbit);
        packet.extract(ipv4_varbit, (bit<32>)((bit<32>)ipv4_novarbit.ihl * 32w4 * 8 - 160));

        hdr.ipv4.setValid();
        hdr.ipv4.version = ipv4_novarbit.version;
        hdr.ipv4.ihl = ipv4_novarbit.ihl;
        hdr.ipv4.diffserv = ipv4_novarbit.diffserv;
        hdr.ipv4.totalLen = ipv4_novarbit.totalLen;
        hdr.ipv4.identification = ipv4_novarbit.identification;
        hdr.ipv4.flags = ipv4_novarbit.flags;
        hdr.ipv4.fragOffset = ipv4_novarbit.fragOffset;
        hdr.ipv4.ttl = ipv4_novarbit.ttl;
        hdr.ipv4.protocol = ipv4_novarbit.protocol;
        hdr.ipv4.hdrChecksum = ipv4_novarbit.hdrChecksum;
        hdr.ipv4.srcAddr = ipv4_novarbit.srcAddr;
        hdr.ipv4.dstAddr = ipv4_novarbit.dstAddr;
        hdr.ipv4.options = ipv4_varbit.options;

        transition select(hdr.ipv4.protocol) {
            8w0x6: parse_tcp;
            default: accept;
        }
    }

    @name(".parse_tcp") state parse_tcp {
        packet.extract(tcp_novarbit);
        packet.extract(tcp_varbit, (bit<32>)((bit<32>)tcp_novarbit.dataOffset * 32w4 * 8 - 160));

        hdr.tcp.setValid();
        hdr.tcp.srcPort = tcp_novarbit.srcPort;
        hdr.tcp.dstPort = tcp_novarbit.dstPort;
        hdr.tcp.seqNo = tcp_novarbit.seqNo;
        hdr.tcp.ackNo = tcp_novarbit.ackNo;
        hdr.tcp.dataOffset = tcp_novarbit.dataOffset;
        hdr.tcp.res = tcp_novarbit.res;
        hdr.tcp.flags = tcp_novarbit.flags;
        hdr.tcp.window = tcp_novarbit.window;
        hdr.tcp.checksum = tcp_novarbit.checksum;
        hdr.tcp.urgentPtr = tcp_novarbit.urgentPtr;
        hdr.tcp.options = tcp_varbit.options;

        transition accept;
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".on_miss") action on_miss() {
    }

    @name(".fib_hit_nexthop") action fib_hit_nexthop(bit<48> dmac, bit<9> port) {
        hdr.ethernet.dstAddr = dmac;
        standard_metadata.egress_port = port;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w0xff;
    }

    @name(".rewrite_src_mac") action rewrite_src_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }

    @name(".ipv4_fib_lpm") table ipv4_fib_lpm {
        actions = {
            on_miss;
            fib_hit_nexthop;
        }
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        size = 512;

        // const entries = {
            // 0xc0a8_016b: fib_hit_nexthop(0x000001000000, 123);
        // }
    }

    @name(".sendout") table sendout {
        actions = {
            on_miss;
            rewrite_src_mac;
        }
        key = {
            standard_metadata.egress_port: exact;
        }
        size = 512;
    }

    apply {
        ipv4_fib_lpm.apply();
        sendout.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.tcp);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum(hdr.ipv4.isValid(), { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.options }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
        verify_checksum_with_payload(hdr.tcp.isValid(), { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, hdr.ipv4.totalLen, 16w0xffeb, hdr.tcp.srcPort, hdr.tcp.dstPort, hdr.tcp.seqNo, hdr.tcp.ackNo, hdr.tcp.dataOffset, hdr.tcp.res, hdr.tcp.flags, hdr.tcp.window, hdr.tcp.urgentPtr, hdr.tcp.options }, hdr.tcp.checksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum(hdr.ipv4.isValid(), { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.options }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
        update_checksum_with_payload(hdr.tcp.isValid(), { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, hdr.ipv4.totalLen, 16w0xffeb, hdr.tcp.srcPort, hdr.tcp.dstPort, hdr.tcp.seqNo, hdr.tcp.ackNo, hdr.tcp.dataOffset, hdr.tcp.res, hdr.tcp.flags, hdr.tcp.window, hdr.tcp.urgentPtr, hdr.tcp.options }, hdr.tcp.checksum, HashAlgorithm.csum16);
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

