#include "hpps/MosfetController.hpp"
#include <zephyr/sys/printk.h>

MosfetController::MosfetController():
    ch1_spec(PWM_DT_SPEC_GET(DT_NODELABEL(mosfet_ch1))),
    ch2_spec(PWM_DT_SPEC_GET(DT_NODELABEL(mosfet_ch2))),
    ch3_spec(PWM_DT_SPEC_GET(DT_NODELABEL(mosfet_ch3))),
    ch4_spec(PWM_DT_SPEC_GET(DT_NODELABEL(mosfet_ch4)))
{
    k_mutex_init(&lock);
}

int MosfetController::init()
{
    if (!pwm_is_ready_dt(&ch1_spec) || !pwm_is_ready_dt(&ch2_spec) ||
        !pwm_is_ready_dt(&ch3_spec) || !pwm_is_ready_dt(&ch4_spec)) 
    {
        printk("MosfetController: Someone PWM is not ready!!\n");
        return -ENODEV;
    }

    allOff();
    printk("MosfetController: HW PWM is ready.\n");
    return 0;
}

const struct pwm_dt_spec* MosfetController::getSpec(MosfetChannel ch) const
{
    switch(ch){
        case MosfetChannel::CH1: return &ch1_spec;
        case MosfetChannel::CH2: return &ch2_spec;
        case MosfetChannel::CH3: return &ch3_spec;
        case MosfetChannel::CH4: return &ch4_spec;
        default:                 return nullptr;
    }
}

int MosfetController::setDuty(MosfetChannel ch, uint8_t percentage)
{    
    const auto *spec = getSpec(ch);
    if(!spec) return -EINVAL;

    if(percentage > 100) percentage = 100;

    k_mutex_lock(&lock, K_FOREVER);

    uint64_t pulse_ns = (uint64_t)spec->period * percentage / 100;
    int ret = pwm_set_pulse_dt(spec, pulse_ns);

    k_mutex_unlock(&lock);
    return ret;
}

void MosfetController::allOff()
{
    k_mutex_lock(&lock, K_FOREVER);

    pwm_set_pulse_dt(&ch1_spec, 0);
    pwm_set_pulse_dt(&ch2_spec, 0);
    pwm_set_pulse_dt(&ch3_spec, 0);
    pwm_set_pulse_dt(&ch4_spec, 0);

    k_mutex_unlock(&lock);
}
