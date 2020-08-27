#include <core.p4>
#include <v1model.p4>
const bit<16> ETHERTYPE_IP4 = 0x0800;

const bit<8>  IPPROTO_TCP   = 0x06;
const bit<8>  IPPROTO_UDP   = 0x11;

const bit<16> ZERO = 0;
const bit<32> MAX_FLOWS = 65535;

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

@name("mac_learn_digest") struct mac_learn_digest {
    bit<48> srcAddr;
    bit<9>  ingress_port;
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".forward") action forward(bit<9> port) {
        standard_metadata.egress_spec = port;
    }
    @name(".mac_learn") action mac_learn() {
        digest<mac_learn_digest>((bit<32>)1024, { hdr.ethernet.srcAddr, standard_metadata.ingress_port });
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

    bit<64> flow_id = 0x0;
    

    apply {
        smac.apply();
    //hash(flow_id, HashAlgorithm.identity, ZERO, {hdr.ethernet.dstAddr, hdr.ethernet.srcAddr, hdr.ethernet.etherType, hdr.ip4.version, hdr.ip4.ihl, hdr.ip4.diffserv, hdr.ip4.totalLen, hdr.ip4.identification, hdr.ip4.flags, hdr.ip4.fragOffset, hdr.ip4.ttl, hdr.ip4.protocol, hdr.ip4.hdrChecksum, hdr.ip4.srcAddr, hdr.ip4.dstAddr, hdr.l4.srcPort, hdr.l4.dstPort}, MAX_FLOWS);
    //hash(flow_id, HashAlgorithm.crc32, ZERO, {hdr.ethernet.dstAddr, hdr.ethernet.srcAddr, hdr.ethernet.etherType, hdr.ip4.version, hdr.ip4.ihl, hdr.ip4.diffserv, hdr.ip4.totalLen, hdr.ip4.identification, hdr.ip4.flags, hdr.ip4.fragOffset, hdr.ip4.ttl, hdr.ip4.protocol, hdr.ip4.hdrChecksum, hdr.ip4.srcAddr, hdr.ip4.dstAddr, hdr.l4.srcPort, hdr.l4.dstPort}, MAX_FLOWS);
	//hash(flow_id, HashAlgorithm.crc16, ZERO, {hdr.ethernet.dstAddr, hdr.ethernet.srcAddr, hdr.ethernet.etherType}, MAX_FLOWS);
        dmac.apply();
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

