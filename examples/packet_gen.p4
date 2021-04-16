#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}
header ipv4_t {
    bit<8>  versionIhl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<16> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header udp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> len;
    bit<16> chkSum;
}

struct headers {
    @name(".ethernet")
    ethernet_t ethernet;
    @name(".ipv4")
    ipv4_t ipv4;
    @name(".udp")
    udp_t udp;
}




// parser
parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ipv4") state parse_ipv4 {
	packet.extract(hdr.ipv4);
	transition accept;
    }
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition parse_ipv4;	
    }
    @name(".start") state start {
        transition select (standard_metadata.event) {
		0 : parse_ethernet;
		default: accept;
	}
    }
}// parser parse

// pipeline instantiations

// control
control ingress(inout headers hdr, inout metadata data, inout standard_metadata_t standard_metadata) {
    @name("._drop") action _drop() {
        mark_to_drop();
    }

    @name(".forward") action single(bit<32> time, bit<32> id) {
    	standard_metadata.egress_port = 9w3;
	timer_single(time, id);
    }

    @name(".forward") action multiple(bit<32> time, bit<32> id, bit<32> count) {
    	standard_metadata.egress_port = 9w3;
	timer_multiple(time, id, count);
    }

    @name(".forward") action periodic(bit<32> time, bit<32> id) {
    	standard_metadata.egress_port = 9w3;
	timer_periodic(time, id);
    }


    @name(".table0") table table0 {
        actions = {
            single;
	    multiple;
	    periodic;
            _drop;
        }
        key = {
            hdr.ipv4.protocol: exact;
        }
        size = 4;
        default_action = _drop();
    }


    apply {
    	if (standard_metadata.event != 0) {
		hdr.ethernet.setValid();
		hdr.ethernet.dstAddr = 0x010203040506;
		hdr.ethernet.srcAddr = 0x0708090a0b0c;
		hdr.ethernet.etherType = 0x0800;
		hdr.ipv4.setValid();
		hdr.ipv4.srcAddr = 0x0a000001;
		hdr.ipv4.dstAddr = 0x0a000002;
		hdr.ipv4.protocol = 6;
		hdr.ipv4.versionIhl = 0x45;
		hdr.ipv4.diffserv = 1;
		hdr.ipv4.totalLen = 1;
		hdr.ipv4.identification = 1;
		hdr.ipv4.fragOffset = 1;
		hdr.ipv4.ttl = 1;
		hdr.ipv4.hdrChecksum = 2;
		hdr.udp.setValid();
		hdr.udp.dstPort = 1;
		hdr.udp.srcPort = 2;
		hdr.udp.len = 3;
		standard_metadata.egress_port = 9w3;
	} else {
	        table0.apply();
	}
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.udp);
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
