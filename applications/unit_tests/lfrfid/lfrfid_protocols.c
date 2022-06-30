#include <furi.h>
#include "../minunit.h"
#include <toolbox/protocols/protocol_dict.h>
#include <lfrfid/protocols/lfrfid_protocols.h>
#include <toolbox/pulse_protocols/pulse_glue.h>

#define EM_TEST_DATA_SIZE 5
#define EM_TEST_EMULATION_TIMINGS_COUNT 128
#define LF_RFID_READ_TIMING_MULTIPLIER 8

const int8_t em_test_emulation_timings[EM_TEST_EMULATION_TIMINGS_COUNT] = {
    32,  -32, 32,  -32, 32,  -32, 32,  -32, 32,  -32, 32,  -32, 32,  -32, 32,  -32, 32,  -32, -32,
    32,  32,  -32, -32, 32,  32,  -32, -32, 32,  32,  -32, -32, 32,  -32, 32,  -32, 32,  32,  -32,
    -32, 32,  -32, 32,  -32, 32,  -32, 32,  -32, 32,  -32, 32,  -32, 32,  -32, 32,  -32, 32,  -32,
    32,  32,  -32, -32, 32,  -32, 32,  -32, 32,  32,  -32, -32, 32,  32,  -32, -32, 32,  32,  -32,
    -32, 32,  -32, 32,  32,  -32, 32,  -32, -32, 32,  -32, 32,  -32, 32,  32,  -32, -32, 32,  -32,
    32,  32,  -32, -32, 32,  -32, 32,  -32, 32,  -32, 32,  -32, 32,  -32, 32,  -32, 32,  32,  -32,
    -32, 32,  32,  -32, -32, 32,  -32, 32,  -32, 32,  -32, 32,  -32, 32,
};

MU_TEST(test_lfrfid_protocol_em_read_simple) {
    ProtocolDict* dict = protocol_dict_alloc(lfrfid_protocols, LFRFIDProtocolMax);
    mu_assert_int_eq(EM_TEST_DATA_SIZE, protocol_dict_get_data_size(dict, LFRFIDProtocolEM4100));
    mu_assert_string_eq("EM4100", protocol_dict_get_name(dict, LFRFIDProtocolEM4100));
    mu_assert_string_eq("EM-Marin", protocol_dict_get_manufacturer(dict, LFRFIDProtocolEM4100));

    const uint8_t data[EM_TEST_DATA_SIZE] = {0x58, 0x00, 0x85, 0x64, 0x02};

    protocol_dict_decoders_start(dict);

    ProtocolId protocol = PROTOCOL_NO;
    PulseGlue* pulse_glue = pulse_glue_alloc();

    for(size_t i = 0; i < EM_TEST_EMULATION_TIMINGS_COUNT * 10; i++) {
        bool pulse_pop = pulse_glue_push(
            pulse_glue,
            em_test_emulation_timings[i % EM_TEST_EMULATION_TIMINGS_COUNT] >= 0,
            abs(em_test_emulation_timings[i % EM_TEST_EMULATION_TIMINGS_COUNT]) *
                LF_RFID_READ_TIMING_MULTIPLIER);

        if(pulse_pop) {
            uint32_t length, period;
            pulse_glue_pop(pulse_glue, &length, &period);

            protocol = protocol_dict_decoders_feed(dict, false, period);
            if(protocol != PROTOCOL_NO) break;

            protocol = protocol_dict_decoders_feed(dict, true, length - period);
            if(protocol != PROTOCOL_NO) break;
        }
    }

    pulse_glue_free(pulse_glue);

    mu_assert_int_eq(LFRFIDProtocolEM4100, protocol);
    uint8_t received_data[EM_TEST_DATA_SIZE] = {0};
    protocol_dict_get_data(dict, protocol, received_data, EM_TEST_DATA_SIZE);

    mu_assert_mem_eq(data, received_data, EM_TEST_DATA_SIZE);

    protocol_dict_free(dict);
}

MU_TEST(test_lfrfid_protocol_em_emulate_simple) {
    ProtocolDict* dict = protocol_dict_alloc(lfrfid_protocols, LFRFIDProtocolMax);
    mu_assert_int_eq(EM_TEST_DATA_SIZE, protocol_dict_get_data_size(dict, LFRFIDProtocolEM4100));
    mu_assert_string_eq("EM4100", protocol_dict_get_name(dict, LFRFIDProtocolEM4100));
    mu_assert_string_eq("EM-Marin", protocol_dict_get_manufacturer(dict, LFRFIDProtocolEM4100));

    const uint8_t data[EM_TEST_DATA_SIZE] = {0x58, 0x00, 0x85, 0x64, 0x02};

    protocol_dict_set_data(dict, LFRFIDProtocolEM4100, data, EM_TEST_DATA_SIZE);
    mu_check(protocol_dict_encoder_start(dict, LFRFIDProtocolEM4100));

    for(size_t i = 0; i < EM_TEST_EMULATION_TIMINGS_COUNT; i++) {
        LevelDuration level_duration = protocol_dict_encoder_yield(dict, LFRFIDProtocolEM4100);

        if(level_duration_get_level(level_duration)) {
            mu_assert_int_eq(
                em_test_emulation_timings[i], level_duration_get_duration(level_duration));
        } else {
            mu_assert_int_eq(
                em_test_emulation_timings[i], -level_duration_get_duration(level_duration));
        }
    }

    protocol_dict_free(dict);
}

MU_TEST_SUITE(test_lfrfid_protocols_suite) {
    MU_RUN_TEST(test_lfrfid_protocol_em_read_simple);
    MU_RUN_TEST(test_lfrfid_protocol_em_emulate_simple);
}

int run_minunit_test_lfrfid_protocols() {
    MU_RUN_SUITE(test_lfrfid_protocols_suite);
    return MU_EXIT_CODE;
}