# Contributing to eyaX

Thank you for your interest in contributing to eyaX!

eyaX is an experimental x86_64 operating system built from scratch. It is currently in early development, so the project is still evolving and its architecture may change.

## Getting Started

1. Fork the eyaX repository.
2. Clone your fork.
3. Set up the required cross-compiler and build dependencies.
4. Build and test eyaX locally.
5. Make your changes.
6. Commit your changes.
7. Open a pull request.

## Before Making Major Changes

For large changes, such as new kernel subsystems or major architectural changes, please open an issue first.

This gives us a chance to discuss the design before significant work is done.

Small fixes and improvements can usually be submitted directly as pull requests.

## Code

Please try to:

* Keep changes focused on one feature or fix.
* Follow the existing code style.
* Keep the kernel simple and understandable.
* Avoid adding unnecessary dependencies.
* Test your changes before submitting a pull request.
* Do not modify existing components unnecessarily when a smaller change is sufficient.

## Issues

Issues can be used for:

* Bug reports
* Feature requests
* Architecture discussions
* Documentation improvements
* Tasks for contributors

When reporting a bug, include as much useful information as possible, such as:

* What happened
* What you expected to happen
* How to reproduce it
* Relevant build output
* Hardware or emulator information, if relevant

## Pull Requests

Please describe:

* What you changed
* Why you changed it
* How you tested it

Keep pull requests focused. Large unrelated changes should be separated into multiple pull requests when practical.

## Good First Issues

If you're new to eyaX, look for issues labeled `good first issue`.

These are intended to provide approachable ways to start contributing.

## Development Philosophy

eyaX is being developed incrementally.

The goal is not to immediately recreate every subsystem found in a mature operating system. New functionality should have a clear purpose and fit the architecture of eyaX.

Ideas and experimentation are welcome.

## Questions and Discussion

If you are unsure about an implementation, open an issue and discuss it before starting a large change.

Have fun hacking on eyaX! 🖥️
