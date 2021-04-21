#include <core.p4>
#include <v1model.p4>

struct metadata {}
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
    bit<32> reserved;
    bit<32> count;
}

struct headers {
    @name(".ethernet")
    ethernet_t ethernet;
    @name(".ipv4")
    ipv4_t ipv4;
    @name(".udp")
    udp_t udp;
}


const bool twice = false;
const bit<32> frequency = 10;

// parser
parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_udp") state parse_udp {
	packet.extract(hdr.udp);
	transition accept;
    }
    @name(".parse_ipv4") state parse_ipv4 {
	packet.extract(hdr.ipv4);
	transition parse_udp;
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
    apply {
    	standard_metadata.egress_port = 9w3;
    	if (standard_metadata.event == 2) {
    	    mark_to_drop();
    	} else {
    	    standard_metadata.egress_port = 9w3;
    	    if (hdr.udp.count % frequency == 0) {
    	        raise_event(2, 1);
    	    }
    	    //if (hdr.udp.count % frequency == 0 && twice) {
    	    //    raise_event(2, 1);
    	    //}
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
