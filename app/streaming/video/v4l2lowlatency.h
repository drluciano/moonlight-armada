#pragma once

#include <QSet>
#include <QString>
#include <QStringList>

// Applies "ultra low latency" decoder tuning to V4L2 stateful (M2M) decoders
// opened by FFmpeg's *_v4l2m2m decoders.
//
// This is the Linux counterpart of the MediaCodec vendor keys used by the
// Android client on Snapdragon SoCs:
//
//   vendor.qti-ext-dec-picture-order.enable -> V4L2_CID_MPEG_VIDEO_DEC_DISPLAY_DELAY(_ENABLE)
//   vendor.qti-ext-dec-low-latency.enable   -> driver-specific "low latency" controls
//
// With a display delay of 0, the Qualcomm Venus/Iris firmware returns frames in
// decode order as soon as they are decoded instead of holding them in the DPB
// until display order is known. Our streams never contain B-frames, so decode
// order is display order and this is lossless.
//
// FFmpeg does not expose these controls and V4L2 controls are scoped to the file
// handle of the M2M context, so we locate the file descriptor that FFmpeg opened
// for the decoder and set the controls on it directly before streaming starts.
class V4L2LowLatency
{
public:
    // Returns the set of open file descriptors that refer to /dev/video* nodes.
    // Call this before avcodec_open2() and pass the result to applyToNewDecoders().
    static QSet<int> snapshotVideoDeviceFds();

    // Applies low latency controls to any V4L2 M2M decoder opened since the
    // snapshot was taken. Returns true if at least one control was applied.
    // The driver name and applied controls are returned for diagnostics.
    static bool applyToNewDecoders(const QSet<int>& fdsBeforeOpen,
                                   QString& driverName,
                                   QStringList& appliedControls);

    // Returns true if this system has a Qualcomm V4L2 decoder that can't be
    // switched to decode order output (e.g. the mainline Iris driver). On those,
    // H.264 is the only codec where our SPS fixup should get frames output
    // immediately, so ultra low latency mode prefers it.
    static bool hasQualcommDecoderWithoutDecodeOrderOutput();
};
