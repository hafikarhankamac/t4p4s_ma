#include <core.p4>
#include <psa.p4>

// In: 010101010100000010000000
// Out: 111111111111111110000000

header dummy_t {
  bit<1> f1;
  bit<8> f2;
  bit<4> f3;
  bit<2> f4;
  bit<2> f5;
  bit<7> padding;
}

struct empty_metadata_t {
}

struct metadata {
}

struct headers {
    dummy_t dummy;
}

parser IngressParserImpl(packet_in packet,
                         out headers hdr,
                         inout metadata meta,
                         in psa_ingress_parser_input_metadata_t istd,
                         in empty_metadata_t resubmit_meta,
                         in empty_metadata_t recirculate_meta) {
    state parse_continue_4 {
	hdr.dummy.f4 = 2w3;
	hdr.dummy.f5 = 2w3;
	transition accept;
    }

    state parse_continue_3 {
	hdr.dummy.f3 = 4w0xF;
	transition select(hdr.dummy.f4, hdr.dummy.f5) {
		(2w0, 2w1) : parse_continue_4;
                (2w0, 2w0) : reject;
                (_, _) : reject; 
	}
    }

    state parse_continue_2 {
        hdr.dummy.f2 = 8w0xFF;
        transition select(hdr.dummy.f3) {
		4w5..4w8 : parse_continue_3;
                _ : reject;
        }
    }
    
    state parse_continue {
	hdr.dummy.f1 = hdr.dummy.f1 + 1;
        transition select(hdr.dummy.f2) {
		8w0x0A &&& 8w0x0F: parse_continue_2;
	        _ : reject;
	}
    }
    state start {
        packet.extract(hdr.dummy);
        transition select(hdr.dummy.f1) {
            1w1 : parse_continue;
            _ : reject;
        }
    }
}

control egress(inout headers hdr,
               inout metadata meta,
               in    psa_egress_input_metadata_t  istd,
               inout psa_egress_output_metadata_t ostd)
{
    apply {
    }
}


control ingress(inout headers hdr,
                inout metadata meta,
                in    psa_ingress_input_metadata_t  istd,
                inout psa_ingress_output_metadata_t ostd)
{
    apply { }
}

parser EgressParserImpl(packet_in buffer,
                        out headers hdr,
                        inout metadata meta,
                        in psa_egress_parser_input_metadata_t istd,
                        in empty_metadata_t normal_meta,
                        in empty_metadata_t clone_i2e_meta,
                        in empty_metadata_t clone_e2e_meta)
{
    state start {
        transition accept;
    }
}

control IngressDeparserImpl(packet_out buffer,
                            out empty_metadata_t clone_i2e_meta,
                            out empty_metadata_t resubmit_meta,
                            out empty_metadata_t normal_meta,
                            inout headers hdr,
                            in metadata meta,
                            in psa_ingress_output_metadata_t istd)
{
    apply {
        buffer.emit(hdr.dummy);
    }
}

control EgressDeparserImpl(packet_out buffer,
                           out empty_metadata_t clone_e2e_meta,
                           out empty_metadata_t recirculate_meta,
                           inout headers hdr,
                           in metadata meta,
                           in psa_egress_output_metadata_t istd,
                           in psa_egress_deparser_input_metadata_t edstd)
{
    apply {
    }
}

IngressPipeline(IngressParserImpl(),
                ingress(),
                IngressDeparserImpl()) ip;

EgressPipeline(EgressParserImpl(),
               egress(),
               EgressDeparserImpl()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
