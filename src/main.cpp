/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include <atomic>
#include "board.h"
#include "hpm_gpio.hpp"
#include "hpm_interrupt.h"
#include "hpm_pwm.hpp"
#include "hpm_timebase.hpp"
#include "thread.hpp"

using namespace LibXR;

namespace
{
enum class LedMode : uint8_t
{
    Breathing = 0,
    Blink1Hz,
};

struct ButtonIrqState
{
    std::atomic<uint32_t> pending{0U};
    volatile uint32_t irq_count = 0u;
};

struct ButtonDebounceState
{
    bool pending = false;
    uint32_t event_ms = 0u;
    uint32_t last_switch_ms = 0u;
    uint32_t switch_count = 0u;
};

struct BreathState
{
    int32_t duty_permille = 0;
    int32_t step_permille = 0;
    uint32_t last_update_ms = 0u;
    uint32_t hold_until_ms = 0u;
};

struct BlinkState
{
    bool on = false;
    uint32_t last_toggle_ms = 0u;
};

constexpr uint8_t kPwmChannel = 2u;   // PB10 and PA10 (LED2) -> GPTMR0.COMP_2
constexpr uint8_t kPwmCmpIndex = 0u;
constexpr uint32_t kPwmFreqHz = 1000u;
constexpr uint32_t kDebounceMs = 30u;
constexpr uint32_t kSwitchLockMs = 80u;
constexpr uint32_t kBlinkHalfPeriodMs = 500u;

constexpr int32_t kBreathMinPermille = 20;
constexpr int32_t kBreathMaxPermille = 900;
constexpr uint32_t kBreathTickMs = 20u;
constexpr int32_t kBreathStepPermille = 8;
constexpr uint32_t kBreathEdgeHoldMs = 500u;

ButtonIrqState g_button_irq_state;

inline float DutyToFloat(int32_t duty_permille)
{
    return static_cast<float>(duty_permille) / 1000.0f;
}

void OnButtonInterrupt(bool, ButtonIrqState* state)
{
    state->pending.store(1U, std::memory_order_release);
    state->irq_count = state->irq_count + 1u;
}

void ResetBreathingState(BreathState* state, uint32_t now_ms)
{
    state->duty_permille = kBreathMinPermille;
    state->step_permille = kBreathStepPermille;
    state->last_update_ms = now_ms;
    state->hold_until_ms = now_ms + kBreathEdgeHoldMs;
}

void ResetBlinkState(BlinkState* state, uint32_t now_ms)
{
    state->on = false;
    state->last_toggle_ms = now_ms;
}

void PollButtonInterrupt(uint32_t now_ms, ButtonDebounceState* state)
{
    if (g_button_irq_state.pending.exchange(0U, std::memory_order_acquire) != 0U)
    {
        state->pending = true;
        state->event_ms = now_ms;
    }
}

bool ShouldToggleMode(HPMGPIO& button,
                      bool button_pressed_level,
                      uint32_t now_ms,
                      ButtonDebounceState* state)
{
    if (!state->pending)
    {
        return false;
    }

    if ((now_ms - state->event_ms) < kDebounceMs)
    {
        return false;
    }
    state->pending = false;

    const bool pressed = (button.Read() == button_pressed_level);
    if (!pressed)
    {
        return false;
    }

    if ((now_ms - state->last_switch_ms) < kSwitchLockMs)
    {
        return false;
    }

    state->last_switch_ms = now_ms;
    ++state->switch_count;
    return true;
}

void EnterBreathingMode(HPMPWM& pwm, BreathState* breath_state, BlinkState* blink_state, uint32_t now_ms)
{
    ResetBreathingState(breath_state, now_ms);
    ResetBlinkState(blink_state, now_ms);
    (void)pwm.SetDutyCycle(DutyToFloat(breath_state->duty_permille));
}

void EnterBlinkMode(HPMPWM& pwm, BlinkState* blink_state, uint32_t now_ms)
{
    ResetBlinkState(blink_state, now_ms);
    (void)pwm.SetDutyCycle(0.0f);
}

void UpdateBreathingEffect(HPMPWM& pwm, BreathState* state, uint32_t now_ms)
{
    if ((now_ms - state->last_update_ms) < kBreathTickMs)
    {
        return;
    }
    state->last_update_ms = now_ms;

    if (static_cast<int32_t>(now_ms - state->hold_until_ms) < 0)
    {
        return;
    }

    state->duty_permille += state->step_permille;
    if (state->duty_permille >= kBreathMaxPermille)
    {
        state->duty_permille = kBreathMaxPermille;
        state->step_permille = -kBreathStepPermille;
        state->hold_until_ms = now_ms + kBreathEdgeHoldMs;
    }
    else if (state->duty_permille <= kBreathMinPermille)
    {
        state->duty_permille = kBreathMinPermille;
        state->step_permille = kBreathStepPermille;
        state->hold_until_ms = now_ms + kBreathEdgeHoldMs;
    }

    (void)pwm.SetDutyCycle(DutyToFloat(state->duty_permille));
}

void UpdateBlinkEffect(HPMPWM& pwm, BlinkState* state, uint32_t now_ms)
{
    if ((now_ms - state->last_toggle_ms) < kBlinkHalfPeriodMs)
    {
        return;
    }

    state->last_toggle_ms = now_ms;
    state->on = !state->on;
    (void)pwm.SetDutyCycle(state->on ? 1.0f : 0.0f);
}
}  // namespace

SDK_DECLARE_EXT_ISR_M(BOARD_APP_GPIO_IRQ, app_button_gpio_isr)
void app_button_gpio_isr(void)
{
    libxr_hpm_gpio_check_interrupt(BOARD_APP_GPIO_INDEX);
}

int main(void)
{
    board_init();

    HPMTimebase timebase;
    (void)timebase;

    HPMGPIO button(BOARD_APP_GPIO_CTRL, BOARD_APP_GPIO_INDEX, BOARD_APP_GPIO_PIN,
                   BOARD_APP_GPIO_IRQ);
    constexpr bool button_pressed_level = (BOARD_BUTTON_PRESSED_VALUE != 0u);
    constexpr GPIO::Pull button_pull = button_pressed_level ? GPIO::Pull::DOWN : GPIO::Pull::UP;
    constexpr GPIO::Direction button_irq_dir = button_pressed_level ? GPIO::Direction::RISING_INTERRUPT
                                                   : GPIO::Direction::FALL_INTERRUPT;
    button.SetConfig({button_irq_dir, button_pull});
    button.RegisterCallback(GPIO::Callback::Create(OnButtonInterrupt, &g_button_irq_state));
    button.EnableInterrupt();

    // PB10 pinmux/clock are initialized inside HPMPWM (GPTMR fallback path). LED2 on PA10
    // is active low, so the output is inverted and PA10 is routed to the same comparator.
    HPMPWM pwm(reinterpret_cast<LibXRHpmPwmType*>(BOARD_GPTMR_PWM), BOARD_GPTMR_PWM_CLK_NAME,
               kPwmChannel, kPwmCmpIndex, true);
    HPM_IOC->PAD[IOC_PAD_PA10].FUNC_CTL = IOC_PA10_FUNC_CTL_GPTMR0_COMP_2;
    pwm.SetConfig({kPwmFreqHz});
    pwm.Enable();

    LedMode mode = LedMode::Breathing;
    const uint32_t startup_ms = Timebase::GetMilliseconds();

    BreathState breath_state;
    BlinkState blink_state;
    ButtonDebounceState button_state;

    ResetBreathingState(&breath_state, startup_ms);
    ResetBlinkState(&blink_state, startup_ms);
    uint32_t last_diag_ms = startup_ms;

    (void)pwm.SetDutyCycle(DutyToFloat(breath_state.duty_permille));

    while (1)
    {
        const uint32_t now_ms = Timebase::GetMilliseconds();

        PollButtonInterrupt(now_ms, &button_state);
        if (ShouldToggleMode(button, button_pressed_level, now_ms, &button_state))
        {
            mode = (mode == LedMode::Breathing) ? LedMode::Blink1Hz : LedMode::Breathing;

            if (mode == LedMode::Blink1Hz)
            {
                EnterBlinkMode(pwm, &blink_state, now_ms);
            }
            else
            {
                EnterBreathingMode(pwm, &breath_state, &blink_state, now_ms);
            }
        }

        if (mode == LedMode::Breathing)
        {
            UpdateBreathingEffect(pwm, &breath_state, now_ms);
        }
        else
        {
            UpdateBlinkEffect(pwm, &blink_state, now_ms);
        }

        if ((now_ms - last_diag_ms) >= 500u)
        {
            last_diag_ms = now_ms;
        }

        Thread::Sleep(5);
    }

    return 0;
}
