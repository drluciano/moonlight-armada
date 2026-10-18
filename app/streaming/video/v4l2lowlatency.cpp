#include "v4l2lowlatency.h"

#include <SDL.h>

#ifdef Q_OS_LINUX

#include <QDir>
#include <QFileInfo>

#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

// Older kernel headers may not have the standardized display delay controls
// (they were promoted from MFC-specific controls in Linux 5.12).
#ifndef V4L2_CID_MPEG_VIDEO_DEC_DISPLAY_DELAY
#define V4L2_CID_MPEG_VIDEO_DEC_DISPLAY_DELAY (V4L2_CID_CODEC_BASE + 653)
#endif
#ifndef V4L2_CID_MPEG_VIDEO_DEC_DISPLAY_DELAY_ENABLE
#define V4L2_CID_MPEG_VIDEO_DEC_DISPLAY_DELAY_ENABLE (V4L2_CID_CODEC_BASE + 654)
#endif

static int xioctl(int fd, unsigned long request, void* arg)
{
    int ret;
    do {
        ret = ioctl(fd, request, arg);
    } while (ret < 0 && errno == EINTR);
    return ret;
}

static bool isM2MDecoder(int fd, QString& driverName)
{
    struct v4l2_capability cap;
    memset(&cap, 0, sizeof(cap));
    if (xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        return false;
    }

    uint32_t caps = (cap.capabilities & V4L2_CAP_DEVICE_CAPS) ? cap.device_caps : cap.capabilities;
    if (!(caps & (V4L2_CAP_VIDEO_M2M | V4L2_CAP_VIDEO_M2M_MPLANE))) {
        return false;
    }

    driverName = QString::fromUtf8((const char*)cap.driver);
    return true;
}

static bool queryControl(int fd, uint32_t id, struct v4l2_queryctrl& qctrl)
{
    memset(&qctrl, 0, sizeof(qctrl));
    qctrl.id = id;
    if (xioctl(fd, VIDIOC_QUERYCTRL, &qctrl) < 0) {
        return false;
    }

    return !(qctrl.flags & (V4L2_CTRL_FLAG_DISABLED | V4L2_CTRL_FLAG_READ_ONLY));
}

static bool setControl(int fd, uint32_t id, int32_t value)
{
    struct v4l2_ext_control ctrl;
    struct v4l2_ext_controls ctrls;

    memset(&ctrl, 0, sizeof(ctrl));
    memset(&ctrls, 0, sizeof(ctrls));

    ctrl.id = id;
    ctrl.value = value;
    ctrls.which = V4L2_CTRL_WHICH_CUR_VAL;
    ctrls.count = 1;
    ctrls.controls = &ctrl;

    if (xioctl(fd, VIDIOC_S_EXT_CTRLS, &ctrls) < 0) {
        // Fall back to the legacy single control API for old drivers
        struct v4l2_control legacyCtrl;
        legacyCtrl.id = id;
        legacyCtrl.value = value;
        if (xioctl(fd, VIDIOC_S_CTRL, &legacyCtrl) < 0) {
            return false;
        }
    }

    return true;
}

static bool isVendorLowLatencyControlName(const QString& name)
{
    // Vendor kernels (msm_vidc, MediaTek vcodec, etc.) expose their low latency
    // modes as private controls with descriptive names. Match them by name so
    // we pick them up on any driver, much like the Android client probes
    // MediaCodec's supported vendor parameter list.
    QString lower = name.toLower();
    lower.remove(' ').remove('-').remove('_');
    return lower.contains("lowlatency") || lower.contains("decodeorder");
}

QSet<int> V4L2LowLatency::snapshotVideoDeviceFds()
{
    QSet<int> fds;
    QDir fdDir("/proc/self/fd");

    for (const QFileInfo& entry : fdDir.entryInfoList(QDir::Files | QDir::System | QDir::NoDotAndDotDot)) {
        if (entry.symLinkTarget().startsWith("/dev/video")) {
            bool ok;
            int fd = entry.fileName().toInt(&ok);
            if (ok) {
                fds.insert(fd);
            }
        }
    }

    return fds;
}

bool V4L2LowLatency::applyToNewDecoders(const QSet<int>& fdsBeforeOpen,
                                        QString& driverName,
                                        QStringList& appliedControls)
{
    QSet<int> newFds = snapshotVideoDeviceFds() - fdsBeforeOpen;

    for (int fd : newFds) {
        if (!isM2MDecoder(fd, driverName)) {
            continue;
        }

        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Applying ultra low latency tuning to V4L2 decoder '%s' (fd %d)",
                    driverName.toUtf8().constData(),
                    fd);

        struct v4l2_queryctrl qctrl;

        // Request decode order output (equivalent of vendor.qti-ext-dec-picture-order).
        // The delay must be set before the enable control on drivers that apply the
        // configuration as soon as display delay is enabled.
        if (queryControl(fd, V4L2_CID_MPEG_VIDEO_DEC_DISPLAY_DELAY, qctrl) &&
                queryControl(fd, V4L2_CID_MPEG_VIDEO_DEC_DISPLAY_DELAY_ENABLE, qctrl)) {
            if (setControl(fd, V4L2_CID_MPEG_VIDEO_DEC_DISPLAY_DELAY, 0) &&
                    setControl(fd, V4L2_CID_MPEG_VIDEO_DEC_DISPLAY_DELAY_ENABLE, 1)) {
                appliedControls.append("Decode order output");
            }
            else {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "Failed to set V4L2 display delay controls: %d",
                            errno);
            }
        }
        else {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "V4L2 decoder doesn't support display delay controls");
        }

        // Walk all controls looking for vendor low latency toggles
        // (equivalent of vendor.qti-ext-dec-low-latency.enable).
        memset(&qctrl, 0, sizeof(qctrl));
        qctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
        while (xioctl(fd, VIDIOC_QUERYCTRL, &qctrl) == 0) {
            uint32_t id = qctrl.id;
            QString name = QString::fromUtf8((const char*)qctrl.name);

            // Skip the standard controls we handled above
            if (id != V4L2_CID_MPEG_VIDEO_DEC_DISPLAY_DELAY &&
                    id != V4L2_CID_MPEG_VIDEO_DEC_DISPLAY_DELAY_ENABLE &&
                    !(qctrl.flags & (V4L2_CTRL_FLAG_DISABLED | V4L2_CTRL_FLAG_READ_ONLY)) &&
                    (qctrl.type == V4L2_CTRL_TYPE_BOOLEAN ||
                     (qctrl.type == V4L2_CTRL_TYPE_INTEGER && qctrl.minimum == 0 && qctrl.maximum == 1)) &&
                    isVendorLowLatencyControlName(name)) {
                if (setControl(fd, id, 1)) {
                    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                                "Enabled vendor V4L2 control: %s (0x%x)",
                                name.toUtf8().constData(),
                                id);
                    appliedControls.append(name);
                }
                else {
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                "Failed to enable vendor V4L2 control: %s (0x%x): %d",
                                name.toUtf8().constData(),
                                id,
                                errno);
                }
            }

            qctrl.id = id | V4L2_CTRL_FLAG_NEXT_CTRL;
        }

        // FFmpeg keeps a single M2M context per decoder
        return !appliedControls.isEmpty();
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "No V4L2 M2M decoder found for ultra low latency tuning");
    return false;
}

#else

QSet<int> V4L2LowLatency::snapshotVideoDeviceFds()
{
    return QSet<int>();
}

bool V4L2LowLatency::applyToNewDecoders(const QSet<int>&, QString&, QStringList&)
{
    return false;
}

#endif
