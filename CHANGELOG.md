# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [v0.1.0] - 2026-09-18

### Added
- Initial project architecture for Zephyr RTOS Headlight Control.
- `MosfetController` class providing 4-channel PWM and GPIO output management.
- `AppTasks` containing `SelfTestTask`, `MosfetWorkerTask`, and interrupt-driven `SafetyTask`.
- Board devicetree overlay (`app.overlay`) and Kconfig (`prj.conf`) configurations.
- Cortex-Debug and OpenOCD debugging support for STM32 targets.
