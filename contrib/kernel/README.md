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

`0002-media-qcom-iris-clock-low-latency-decoding-for-laten.patch` applies on
top of 0001 and matters for every codec, H.264 included. Mainline Iris scales
the decoder clock to the measured input frame rate and votes DDR bandwidth for
a fixed 30 fps, so each frame takes about one frame interval to decode. On the
AYN Odin 3 in Game Mode that measured 8.6 ms per frame at 120 fps and 16 ms at
60 fps. When a client asks for low latency (display delay enabled, delay 0),
the patch keeps the clock and bandwidth vote at the maximum frame rate instead.
Other clients keep today's power behavior.

Status: compile-tested for arm64 against Linux 7.3 with `W=1`, applied after
0001; not yet tested on hardware. Same Signed-off-by note as above.
