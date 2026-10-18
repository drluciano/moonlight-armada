# Kernel patches for ArmadaOS

`0001-media-qcom-iris-add-decode-order-output-for-gen2-dec.patch` adds the
standard V4L2 display delay controls to the mainline Qualcomm `iris` decoder
(SM8550 / SM8650 / SM8750 and other gen2 SoCs). With display delay enabled and
set to 0, the firmware is asked for decode order output
(`HFI_PROP_DECODE_ORDER_OUTPUT`, 0x0300015b), so frames are returned as soon as
they are decoded.

Moonlight Armada's ultra low latency mode sets these controls automatically.
Without this patch it falls back to preferring H.264 on Iris.

Status: compile-tested for arm64 against mainline (2026-10-17) with `W=1`;
not yet tested on hardware. The patch has no Signed-off-by line; whoever
submits it upstream must add their own.
