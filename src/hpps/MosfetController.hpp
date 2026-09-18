#pragma once

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>

enum class MosfetChannel{
    CH1,
    CH2,
    CH3,
    CH4
};

class MosfetController{
    public:
        MosfetController();

        int init();

        int setDuty(MosfetChannel ch, uint8_t percentage);

        void allOff();

    private:
        struct pwm_dt_spec ch1_spec;
        struct pwm_dt_spec ch2_spec;
        struct pwm_dt_spec ch3_spec;
        struct pwm_dt_spec ch4_spec;

        struct k_mutex lock;

        const struct pwm_dt_spec* getSpec(MosfetChannel ch) const;

};
