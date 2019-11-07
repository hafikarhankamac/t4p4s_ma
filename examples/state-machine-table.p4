#include <core.p4>
#include <v1model.p4>

const bit<16> ETHERTYPE_IP4 = 0x0800;

const bit<8>  IPPROTO_TCP   = 0x06;
const bit<8>  IPPROTO_UDP   = 0x11;

const bit<32> MAX_FLOWS = 65536; // 2^16
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
    bit<1> new_flow;
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

@name("state_update_digest") struct state_update_digest {
    bit<1> new_flow;
    bit<16> new_state;
    // TODO: not possible with 32 bit values apparently
    //bit<32> flow_id;
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".forward") action forward(bit<9> port) {
        standard_metadata.egress_port = port;
    }

    @name(".new_flow") action new_flow() {
        meta.state_metadata.current_state = 0;
        meta.state_metadata.new_flow = 1;
    }

    @name(".existing_flow") action existing_flow(bit<16> state) {
        meta.state_metadata.current_state = state;
    }

    @name(".new_state") action new_state() {
        meta.state_metadata.current_state = 1;
    }

    @name(".state1") action state1() {
        meta.state_metadata.current_state = 2;
    }

    @name(".state2") action state2() {
        meta.state_metadata.current_state = 3;
    }

    @name(".state3") action state3() {
        meta.state_metadata.current_state = 4;
    }

    @name(".state4") action state4() {
        meta.state_metadata.current_state = 5;
    }

    @name(".state5") action state5() {
        meta.state_metadata.current_state = 1;
    }

    @name(".dmac") table dmac {
        actions = {
            forward;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        size = 512;
        default_action = forward(0);
    }
    @name(".map_flow_to_state") table map_flow_to_state {
        actions = {
            new_flow;
            existing_flow;
        }
        key = {
            meta.state_metadata.flow_id: exact;
        }
        size = MAX_FLOWS;
        default_action = new_flow();
    }
    @name(".switch_state") table switch_state {
        actions = {
            new_state;
            state1;
            state2;
            state3;
            state4;
            state5;
        }
        key = {
            meta.state_metadata.current_state: exact;
        }
        size = 5;
    }

    apply {
        // simple forwarding
        dmac.apply();

	    /* state machine */

	    // determine flow identifier
	    meta.state_metadata.flow_id = 1;

        // map flow to current state
        map_flow_to_state.apply();

	    // execute action depending on state
	    switch_state.apply();

        // set new state for flow
        digest<state_update_digest>((bit<32>)1024, { meta.state_metadata.new_flow, meta.state_metadata.current_state }); //, meta.state_metadata.flow_id });
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

