SysTick
: used by FreeRTOS

TIM1
: hwio - FAN0 and FAN1 PWM
: `src/main.cpp`

TIM2
: hwio - BUZZER
: `src/main.cpp`

TIM3
: hwio - HEAT0 and BED_HEAT PWM
: `src/main.cpp`

TIM4
: reserved for encoder

TIM5
: SystemTimer (~12ns tick, 32bit, 84MHz, 1s period)
: `src/common/timing_sys.c`

TIM6
: Marlin/Temperature
: `lib/Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/STM32F4/timers.cpp`

TIM7
: Marlin/Temperature
: `lib/Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/STM32F4/timers.cpp`

TIM8
: Burst stepping
: `lib/Marlin/Marlin/src/feature/phase_stepping/burst_stepper.cpp`

TIM9
:

TIM10
:

TIM11
:

TIM12
: Marlin/Stepper
: `lib/Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/STM32F4/timers.cpp`

TIM13
: Phase stepping
: `lib/Marlin/Marlin/src/feature/phase_stepping/phase_stepping.cpp`

TIM14
: GUI - jogwheel, hwio, touch
: `src/main.cpp`
