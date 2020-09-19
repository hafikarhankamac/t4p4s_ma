#include <core.p4>
#include <v1model.p4>
const bit<16> ETHERTYPE_IP4 = 0x0800;

const bit<8>  IPPROTO_TCP   = 0x06;
const bit<8>  IPPROTO_UDP   = 0x11;

const bit<16> ZERO = 0;
const bit<32> MAX_FLOWS = 65535;


header test_packet_t {
	bit<16> byte_16;
	bit<128> byte_32;
	bit<256> byte_64;
	bit<512> byte_128;
	bit<1024> byte_256;
	bit<2048> byte_512;
}

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
	test_packet_t test_packet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        transition parse_ethernet;
    }
    @name(".parse_ethernet") state parse_ethernet {
	packet.extract(hdr.ethernet);
	transition select(hdr.ethernet.etherType) {
		ETHERTYPE_IP4: parse_test_packet;
		default: accept;
	}
    }
    @name(".parse_test_packet") state parse_test_packet {
	packet.extract(hdr.test_packet);
	transition select(hdr.test_packet.byte_16) {
		0x0: accept;
		default: accept;
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

    bit<256> flow_256 = 0x0;
    bit<128> flow_128 = 0x0;
    bit<64> flow_64 = 0x0;
    bit<32> flow_32 = 0x0;
    bit<16> flow_16 = 0x0;

    bit<64> base_64 = 0x00000000FFFFFFFF;
    bit<32> base_32 = 0x0000FFFF;
    bit<16> base_16 = 0x0FF;
    bit<8> base_8 = 0x0F;

    bit<64> max_64 = 0x00000000FFFFFFFF;
    bit<32> max_32 = 0x0000FFFF;
    bit<16> max_16 = 0x0FF;
    bit<8> max_8 = 0x0F;


    apply {
	smac.apply();
	hash(flow_256, HashAlgorithm.crc32, base_64, {hdr.ethernet.dstAddr, hdr.ethernet.srcAddr, hdr.ethernet.etherType, hdr.test_packet.byte_16}, max_64);
        dmac.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
    	packet.emit(hdr.ethernet);
	packet.emit(hdr.test_packet);
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

