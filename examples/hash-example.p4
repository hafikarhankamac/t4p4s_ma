#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
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

@name("mac_learn_digest") struct mac_learn_digest {
    bit<48> srcAddr;
    bit<9>  ingress_port;
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".forward") action forward(bit<9> port) {
        standard_metadata.egress_spec = port;
    }
    @name(".bcast") action bcast() {
        standard_metadata.egress_spec = 9w100;
    }
    @name(".mac_learn") action mac_learn() {
        digest<mac_learn_digest>((bit<32>)1024, { hdr.ethernet.srcAddr, standard_metadata.ingress_port });
    }
    @name("._nop") action _nop() {
    }
    @name(".dmac") table dmac {
        actions = {
            forward;
            bcast;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        size = 512;
    }
    @name(".smac") table smac {
        actions = {
            mac_learn;
            _nop;
        }
        key = {
            hdr.ethernet.srcAddr: exact;
        }
        size = 512;
    }
    bit<32> flow_id = 0;
    bit<32> MAX_FLOWS = 0x65536;

    bit<16> ZERO = 0x0;

    bit<32> srcAddr = 0x11111111;
    bit<32> dstAddr = 0x22222222;
    bit<8> protocol = 0x06;
    bit<16> srcPort = 12345;
    bit<16> dstPort = 12345;

   apply {
        smac.apply();
	hash(flow_id, HashAlgorithm.crc32, ZERO, {srcAddr, dstAddr, protocol, srcPort, dstPort}, MAX_FLOWS);
        dmac.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

