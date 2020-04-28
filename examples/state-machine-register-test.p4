#include <core.p4>
#include <v1model.p4>

const bit<16> ETHERTYPE_IP4 = 0x0800;

const bit<8>  IPPROTO_TCP   = 0x06;
const bit<8>  IPPROTO_UDP   = 0x11;

const bit<32> MAX_FLOWS = 0xFFFF; // 2^16
const bit<16> ZERO = 0;

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ip4_t {
    bit<4>     version;
    bit<4>     ihl;
    bit<8>     diffserv;
    bit<16>    totalLen;
    bit<16>    identification;
    bit<3>     flags;
    bit<13>    fragOffset;
    bit<8>     ttl;
    bit<8>     protocol;
    bit<16>    hdrChecksum;
    bit<32>    srcAddr;
    bit<32>    dstAddr;
}

header l4_t {
    bit<16> srcPort;
    bit<16> dstPort;
}

struct state_metadata_t {
	bit<16> current_state;
	bit<32> flow_id;
}

struct metadata {
    state_metadata_t state_metadata;
}


struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
    ip4_t        ip4;
    l4_t         l4;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        transition parse_ethernet;
    }
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            ETHERTYPE_IP4: parse_ip4;
            default: reject;
        }
    }
    state parse_ip4 {
        packet.extract(hdr.ip4);
        transition select(hdr.ip4.protocol) {
            IPPROTO_UDP  : parse_l4;
            IPPROTO_TCP  : parse_l4;
            default      : reject;
        }
    }
    state parse_l4 {
        packet.extract(hdr.l4);
        transition select(hdr.ip4.protocol) {
            IPPROTO_UDP  : accept;
            IPPROTO_TCP  : accept;
            default      : reject;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".forward") action forward(bit<9> port) {
        standard_metadata.egress_port = port;
    }

    @name(".bcast") action bcast(){
	standard_metadata.egress_port = 9w100;
    }

#    @name(".state1") action state1() {
#        meta.state_metadata.current_state = 2;
#    }

#    @name(".state2") action state2() {
#        meta.state_metadata.current_state = 1;
#    }


    @name(".dmac") table dmac {
        actions = {
            forward;
	    bcast;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        size = 512;
        default_action = forward(0);
    }

#    @name(".switch_state") table switch_state {
#	actions = {
#		state1;
#		state2;
#	}
#	key = {
#		meta.current_state: exact;
#	}
#	size = 5;
#    }

    register<bit<16>>(MAX_FLOWS) state;
    apply {
 
 	// simple forwarding
        dmac.apply();

	bit<16> var;
	bit<16> test;

	var = hdr.ip4.hdrChecksum;
	state.read(test, (bit<32>) var);
	meta.state_metadata.current_state = test;
#	switch_state.apply();
	test = meta.state_metadata.current_state;
	state.write((bit<32>) var, test);


    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ip4);
        packet.emit(hdr.l4);
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

