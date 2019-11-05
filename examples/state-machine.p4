#include <core.p4>
#include <v1model.p4>

const bit<16> ETHERTYPE_IP4 = 0x0800;

const bit<8>  IPPROTO_TCP   = 0x06;
const bit<8>  IPPROTO_UDP   = 0x11;

const bit<32> MAX_FLOWS = 1024;
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

header tcp_t {
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<4>  res;
    bit<8>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

header udp_t {
    bit<16> plength;
    bit<16> checksum;
}

struct metadata {
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
    ip4_t        ip4;
    l4_t         l4;
    udp_t        udp;
    tcp_t        tcp;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        transition parse_ethernet;
    }
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            ETHERTYPE_IP4: parse_ip4;
            default: accept;
        }
    }
    state parse_ip4 {
        packet.extract(hdr.ip4);
        transition select(hdr.ip4.protocol) {
            IPPROTO_UDP  : parse_l4;
            IPPROTO_TCP  : parse_l4;
            default      : accept;
        }
    }
    state parse_l4 {
        packet.extract(hdr.l4);
        transition select(hdr.ip4.protocol) {
            IPPROTO_UDP  : parse_udp;
            IPPROTO_TCP  : parse_tcp;
            default      : accept;
        }
    }
    state parse_udp {
        packet.extract(hdr.udp);
        transition accept;
    }
    state parse_tcp {
        packet.extract(hdr.tcp);
        transition accept;
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
    @name("._nop") action _nop() {
    }
    @name(".dmac") table dmac {
        actions = {
            forward;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        size = 512;
    }
    register<bit<16>>(MAX_FLOWS) state; // per flow state keeping
    bit<32> flow_id;
    bit<16> current_state;
    apply {
	// simple forwarding
        dmac.apply();

	/* state machine */

	// determine flow identifier
	flow_id = hdr.ip4.dstAddr % MAX_FLOWS;
	//zero = 0;
	//hash(
	//	flow_id,
	//	HashAlgorithm.crc32,
	//	ZERO,
	//	{ hdr.ip4.srcAddr,
	//	  hdr.ip4.dstAddr,
        //	  hdr.ip4.protocol,
        //	  hdr.l4.srcPort,
        //	  hdr.l4.dstPort },
	//	MAX_FLOWS
	//);

	//// get state for flow
        //state.read(current_state,flow_id);

	//// write back new state for flow
        //state.write(flow_id, current_state);
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

