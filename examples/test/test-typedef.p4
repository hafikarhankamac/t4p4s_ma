#include <core.p4>
#include <psa.p4>

// In: 00000000
// Out: 10000000

typedef bit<1> one_bit_t;

header dummy_t {
    one_bit_t f1;
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
    state parse_ethernet {
        packet.extract(hdr.dummy);
        transition accept;
    }
    state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr,
               inout metadata meta,
               in    psa_egress_input_metadata_t  istd,
               inout psa_egress_output_metadata_t ostd)
{
    one_bit_t one = 1;
    
    apply {
       hdr.dummy.f1 = hdr.dummy.f1 + one + 1w0;
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
