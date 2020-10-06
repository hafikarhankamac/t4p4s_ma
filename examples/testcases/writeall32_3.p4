#include <core.p4>
#include <v1model.p4>


struct metadata {
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header custom_t {
    bit<32> ctr;
    bit<32> ctr2;
    bit<32> ctr3;
    bit<32> ctr4;
}

struct headers {
    @name(".ethernet")
    ethernet_t ethernet;
    @name(".custom")
    custom_t custom;
}



// parser
parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition parse_custom;
    }
    @name(".parse_custom") state parse_custom {
        packet.extract(hdr.custom);
        transition accept;
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}// parser parse


// pipeline instantiations

// control
control ingress(inout headers hdr, inout metadata data, inout standard_metadata_t standard_metadata) {
    @name("._drop") action _drop() {
        mark_to_drop();
    }

    @name(".forward") action forward(@__ref bit<32> count, @__ref bit<32> count2, @__ref bit<32> count3) {
        standard_metadata.egress_port = 9w1;
        count = hdr.custom.ctr;
	count2 = hdr.custom.ctr2;
	count3 = hdr.custom.ctr3;
    }

    @name(".table0") table table0 {
        actions = {
            forward;
            _drop;
        }
        key = {
            hdr.ethernet.srcAddr: exact;
        }
        size = 512;
        default_action = _drop();
    }

    apply {
        table0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.custom);
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