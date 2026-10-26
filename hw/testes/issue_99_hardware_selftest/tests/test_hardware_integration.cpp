#include "unity_compat.h"
#include "../firmware/include/hardware_selftest_suite.h"

void test_full_hardware_compliance_matrix(void) {
    HardwareComplianceReport_t report;
    report.rf_ele01_sensors_ok    = true;
    report.rf_ele02_encoders_ok   = true;
    report.rf_ele03_motors_ok     = true;
    report.rf_ele04_telemetry_ok  = true;
    report.rf_ele05_visual_ok     = true;
    report.rf_ele06_buzzer_ok     = true;

    report.rnf_ele01_rate_ok      = true; // >= 20 Hz
    report.rnf_ele02_range_ok     = true; // 30-180 mm, erro <= 10 mm
    report.rnf_ele03_wireless_ok  = true; // 8 m, perda < 2%
    report.rnf_ele04_pcb_ok       = true; // Shield modular sem protoboard
    report.rnf_ele05_noise_ok     = true; // Zero resets sob transitórios

    TEST_ASSERT_TRUE(hardware_is_fully_compliant(&report));

    // Se qualquer um dos requisitos falhar, a homologacao geral falha
    report.rnf_ele03_wireless_ok = false;
    TEST_ASSERT_FALSE(hardware_is_fully_compliant(&report));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_full_hardware_compliance_matrix);
    return UNITY_END();
}
