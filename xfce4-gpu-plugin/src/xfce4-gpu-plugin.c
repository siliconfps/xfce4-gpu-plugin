/*
 * XFCE4 GPU Monitor Panel Plugin
 * Monitors: GPU usage %, VRAM, chip/mem clocks, temperature
 * Supports AMDGPU (sysfs) and NVIDIA (nvidia-smi)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-macros.h>

#define PLUGIN_NAME   "xfce4-gpu-plugin"
#define UPDATE_MS     900
#define BORDER        2
#define TEXT_MAX      512

#define DRM_BASE      "/sys/class/drm"

typedef enum {
    GPU_NONE,
    GPU_AMD,
    GPU_NVIDIA
} GpuVendor;

typedef struct {
    XfcePanelPlugin *plugin;

    GtkWidget       *ebox;
    GtkWidget       *box;
    GtkWidget       *icon;
    GtkWidget       *label;

    guint            timer_id;

    GpuVendor        vendor;
    gchar           *vendor_name;
    gchar           *card_name;
    gchar           *drm_card_path;
    gchar           *hwmon_path;

    gint             gpu_usage;
    guint64          vram_used;
    guint64          vram_total;
    gint             sclk_mhz;
    gint             mclk_mhz;
    gint             temp_mc;
} GpuPlugin;

static gchar *try_read_sysfs (const gchar *path)
{
    FILE *f;
    static gchar buf[256];

    if (!path)
        return NULL;
    f = fopen (path, "r");
    if (!f)
        return NULL;
    if (fgets (buf, sizeof(buf), f))
    {
        g_strstrip (buf);
        fclose (f);
        return buf;
    }
    fclose (f);
    return NULL;
}

static gint parse_int (const gchar *str)
{
    return str ? (gint) strtol (str, NULL, 0) : 0;
}

static guint64 parse_uint64 (const gchar *str)
{
    return str ? g_ascii_strtoull (str, NULL, 10) : 0;
}

static GpuVendor detect_gpu (GpuPlugin *gpu)
{
    gchar *path;
    gchar *vendor_str;
    gint vendor_id;
    int card, hwmon;

    for (card = 0; card < 8; card++)
    {
        path = g_strdup_printf (DRM_BASE "/card%d/device/vendor", card);
        vendor_str = try_read_sysfs (path);
        g_free (path);

        if (!vendor_str)
            continue;

        vendor_id = parse_int (vendor_str);

        if (vendor_id == 0x1002)
        {
            gpu->vendor = GPU_AMD;
            gpu->vendor_name = g_strdup ("AMD");
        }
        else if (vendor_id == 0x10de)
        {
            gpu->vendor = GPU_NVIDIA;
            gpu->vendor_name = g_strdup ("NVIDIA");
        }
        else
        {
            continue;
        }

        gpu->drm_card_path = g_strdup_printf (DRM_BASE "/card%d/device", card);
        gpu->card_name = g_strdup_printf ("card%d", card);

        if (gpu->vendor == GPU_AMD)
        {
            for (hwmon = 0; hwmon < 8; hwmon++)
            {
                path = g_strdup_printf (DRM_BASE "/card%d/device/hwmon/hwmon%d/name",
                                        card, hwmon);
                gchar *name = try_read_sysfs (path);
                g_free (path);

                if (name && g_strcmp0 (name, "amdgpu") == 0)
                {
                    gpu->hwmon_path = g_strdup_printf (
                        DRM_BASE "/card%d/device/hwmon/hwmon%d", card, hwmon);
                    break;
                }
            }
            if (!gpu->hwmon_path)
                gpu->hwmon_path = g_strdup_printf (
                    DRM_BASE "/card%d/device/hwmon/hwmon0", card);
        }

        return gpu->vendor;
    }

    return GPU_NONE;
}

static void read_amd_sysfs (GpuPlugin *gpu)
{
    gchar *path;
    gchar *val;

    path = g_strdup_printf ("%s/gpu_busy_percent", gpu->drm_card_path);
    gpu->gpu_usage = parse_int (try_read_sysfs (path));
    g_free (path);

    path = g_strdup_printf ("%s/mem_info_vram_used", gpu->drm_card_path);
    gpu->vram_used = parse_uint64 (try_read_sysfs (path));
    g_free (path);

    path = g_strdup_printf ("%s/mem_info_vram_total", gpu->drm_card_path);
    gpu->vram_total = parse_uint64 (try_read_sysfs (path));
    g_free (path);

    path = g_strdup_printf ("%s/freq1_input", gpu->hwmon_path);
    val = try_read_sysfs (path);
    gpu->sclk_mhz = val ? parse_int (val) / 1000000 : 0;
    g_free (path);

    path = g_strdup_printf ("%s/freq2_input", gpu->hwmon_path);
    val = try_read_sysfs (path);
    gpu->mclk_mhz = val ? parse_int (val) / 1000000 : 0;
    g_free (path);

    path = g_strdup_printf ("%s/temp1_input", gpu->hwmon_path);
    val = try_read_sysfs (path);
    gpu->temp_mc = val ? parse_int (val) / 1000 : 0;
    g_free (path);
}

static void read_nvidia_smi (GpuPlugin *gpu)
{
    gchar *output = NULL;
    GError *error = NULL;
    gint exit_status;
    gint gpu_util, mem_util, temp, mem_used, mem_total, sclk, mclk;

    if (!g_spawn_command_line_sync (
            "nvidia-smi --query-gpu=utilization.gpu,utilization.memory,"
            "temperature.gpu,memory.used,memory.total,clocks.gr,clocks.mem "
            "--format=csv,noheader,nounits 2>/dev/null",
            &output, NULL, &exit_status, &error))
        return;

    if (exit_status != 0 || !output)
    {
        g_free (output);
        return;
    }

    g_strstrip (output);

    if (sscanf (output, "%d, %d, %d, %d, %d, %d, %d",
                &gpu_util, &mem_util, &temp, &mem_used, &mem_total, &sclk, &mclk) == 7)
    {
        gpu->gpu_usage = gpu_util;
        gpu->vram_used = (guint64) mem_used * 1024 * 1024;
        gpu->vram_total = (guint64) mem_total * 1024 * 1024;
        gpu->sclk_mhz = sclk;
        gpu->mclk_mhz = mclk;
        gpu->temp_mc = temp;
    }

    g_free (output);
}

static void format_size (guint64 bytes, gchar *buf, gint bufsz)
{
    if (bytes >= 1024 * 1024 * 1024)
        g_snprintf (buf, bufsz, "%.1f GiB",
                    (double) bytes / (1024.0 * 1024.0 * 1024.0));
    else if (bytes >= 1024 * 1024)
        g_snprintf (buf, bufsz, "%.0f MiB",
                    (double) bytes / (1024.0 * 1024.0));
    else if (bytes >= 1024)
        g_snprintf (buf, bufsz, "%.0f KiB", (double) bytes / 1024.0);
    else
        g_snprintf (buf, bufsz, "%lu B", (unsigned long) bytes);
}

static const gchar *temp_color (gint temp)
{
    if (temp >= 75) return "#ff4444";
    if (temp >= 55) return "#ffaa00";
    return "#44cc44";
}

static const gchar *usage_color (gint usage)
{
    if (usage >= 90) return "#ff4444";
    if (usage >= 60) return "#ffaa00";
    return "#44cc44";
}

static void update_display (GpuPlugin *gpu)
{
    gchar markup[TEXT_MAX];
    gchar vram_used_str[32];
    gchar vram_total_str[32];
    gchar tooltip[512];

    format_size (gpu->vram_used, vram_used_str, sizeof(vram_used_str));
    format_size (gpu->vram_total, vram_total_str, sizeof(vram_total_str));

    g_snprintf (markup, sizeof(markup),
                "<span>GPU: "
                "<span foreground=\"%s\">%d%%</span> | "
                "<span foreground=\"%s\">%d" "\xc2\xb0" "C</span></span>",
                usage_color (gpu->gpu_usage),
                gpu->gpu_usage,
                temp_color (gpu->temp_mc),
                gpu->temp_mc);

    gtk_label_set_markup (GTK_LABEL (gpu->label), markup);

    g_snprintf (tooltip, sizeof(tooltip),
                "%s GPU (%s)\n"
                "GPU:       %d%%\n"
                "VRAM:      %s / %s\n"
                "Chip clk:  %.1f GHz\n"
                "VRAM clk:  %.1f GHz\n"
                "Temp:      %d" "\xc2\xb0" "C",
                gpu->vendor_name ? gpu->vendor_name : "?",
                gpu->card_name ? gpu->card_name : "?",
                gpu->gpu_usage,
                vram_used_str, vram_total_str,
                (double) gpu->sclk_mhz / 1000.0,
                (double) gpu->mclk_mhz / 1000.0,
                gpu->temp_mc);

    gtk_widget_set_tooltip_text (GTK_WIDGET (gpu->plugin), tooltip);
}

static gboolean gpu_plugin_update (gpointer user_data)
{
    GpuPlugin *gpu = (GpuPlugin *) user_data;

    if (gpu->vendor == GPU_AMD)
        read_amd_sysfs (gpu);
    else if (gpu->vendor == GPU_NVIDIA)
        read_nvidia_smi (gpu);
    else
        return TRUE;

    update_display (gpu);
    return TRUE;
}

static gboolean gpu_plugin_size_changed (XfcePanelPlugin *plugin,
                                          gint size, GpuPlugin *gpu)
{
    GtkOrientation orientation = xfce_panel_plugin_get_orientation (plugin);

    if (orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
    else
        gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);

    return TRUE;
}

static void gpu_plugin_orientation_changed (XfcePanelPlugin *plugin,
                                             GtkOrientation orientation,
                                             GpuPlugin *gpu)
{
    if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
        gtk_orientable_set_orientation (GTK_ORIENTABLE (gpu->box),
                                         GTK_ORIENTATION_HORIZONTAL);
        gtk_label_set_angle (GTK_LABEL (gpu->label), 0);
    }
    else
    {
        gtk_orientable_set_orientation (GTK_ORIENTABLE (gpu->box),
                                         GTK_ORIENTATION_VERTICAL);
        gtk_label_set_angle (GTK_LABEL (gpu->label), 90);
    }
}

static void gpu_plugin_free (XfcePanelPlugin *plugin, GpuPlugin *gpu)
{
    if (gpu->timer_id)
        g_source_remove (gpu->timer_id);

    g_free (gpu->vendor_name);
    g_free (gpu->card_name);
    g_free (gpu->drm_card_path);
    g_free (gpu->hwmon_path);

    gtk_widget_destroy (gpu->ebox);
    g_free (gpu);
}

static void gpu_plugin_construct (XfcePanelPlugin *plugin)
{
    GpuPlugin *gpu;

    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    gpu = g_new0 (GpuPlugin, 1);
    gpu->plugin = plugin;

    g_signal_connect (G_OBJECT (plugin), "free-data",
                      G_CALLBACK (gpu_plugin_free), gpu);
    g_signal_connect (G_OBJECT (plugin), "size-changed",
                      G_CALLBACK (gpu_plugin_size_changed), gpu);
    g_signal_connect (G_OBJECT (plugin), "orientation-changed",
                      G_CALLBACK (gpu_plugin_orientation_changed), gpu);

    if (detect_gpu (gpu) == GPU_NONE)
    {
        gpu->ebox = gtk_event_box_new ();
        gtk_container_add (GTK_CONTAINER (plugin), gpu->ebox);
        gpu->label = gtk_label_new ("GPU: --");
        gtk_container_add (GTK_CONTAINER (gpu->ebox), gpu->label);
        gtk_widget_set_tooltip_text (gpu->ebox, "No supported GPU detected");
        gtk_widget_show_all (GTK_WIDGET (plugin));
        return;
    }

    GtkOrientation orientation = xfce_panel_plugin_get_orientation (plugin);

    gpu->ebox = gtk_event_box_new ();
    gtk_container_add (GTK_CONTAINER (plugin), gpu->ebox);

    gpu->box = gtk_box_new (
        orientation == GTK_ORIENTATION_VERTICAL
            ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_container_add (GTK_CONTAINER (gpu->ebox), gpu->box);

    gpu->icon = gtk_image_new_from_icon_name ("indicator-sensors-gpu",
                                               GTK_ICON_SIZE_MENU);
    gtk_box_pack_start (GTK_BOX (gpu->box), gpu->icon, FALSE, FALSE, 0);

    gpu->label = gtk_label_new ("GPU ...");
    gtk_widget_set_name (gpu->label, "xfce4-gpu-plugin-label");
    gtk_box_pack_start (GTK_BOX (gpu->box), gpu->label, FALSE, FALSE, 0);

    gtk_container_set_border_width (GTK_CONTAINER (gpu->ebox), BORDER);

    if (orientation == GTK_ORIENTATION_VERTICAL)
        gtk_label_set_angle (GTK_LABEL (gpu->label), 90);

    gtk_widget_show_all (GTK_WIDGET (plugin));

    gpu_plugin_update (gpu);
    gpu->timer_id = g_timeout_add (UPDATE_MS, gpu_plugin_update, gpu);
}

XFCE_PANEL_PLUGIN_REGISTER (gpu_plugin_construct);
