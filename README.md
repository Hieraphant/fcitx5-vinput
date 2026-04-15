# fcitx5-vinput

A simpler Moonshine-focused fork of `fcitx5-vinput`.

## What is different in this version

This fork is focused on getting Moonshine local ASR working cleanly and making that path easier to understand and review.

Changes in this fork:
- adds Moonshine local model validation
- adds a clear offline-only error for Moonshine on the streaming backend
- documents the expected Moonshine `vinput-model.json` layout
- adds a small regression test for Moonshine model validation

## What this fork is trying to do

The upstream project supports a broader set of providers, models, and workflows.
This version keeps the focus on a compact Moonshine path for local Linux voice input.

## Current status

- builds successfully
- Moonshine local models can be discovered and activated
- the daemon initializes Moonshine offline ASR correctly
- speech-to-text works end to end with a local Moonshine model

## Upstream and fork

Upstream project:
- `xifan2333/fcitx5-vinput`

This fork:
- `Hieraphant/fcitx5-vinput`

## Notes

This repo is intended to keep the Moonshine-related changes easier to follow.
The separate registry follow-up can be done after the main runtime patch.
