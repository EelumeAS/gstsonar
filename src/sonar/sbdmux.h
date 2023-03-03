#ifndef __GST_SBDMUX_H__
#define __GST_SBDMUX_H__

#include <gst/gst.h>
#include <glib/gqueue.h>

#include <gst/base/gstaggregator.h>

G_BEGIN_DECLS

// we need to define custom pads for aggregator
GType gst_sbdmux_pad_get_type (void);

#define GST_TYPE_SBDMUX_PAD \
  (gst_sbdmux_pad_get_type())
#define GST_SBDMUX_PAD(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_SBDMUX_PAD, GstSbdmuxPad))
#define GST_SBDMUX_PAD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_SBDMUX_PAD, GstSbdmuxPadClass))
#define GST_IS_SBDMUX_PAD(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_SBDMUX_PAD))
#define GST_IS_SBDMUX_PAD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_SBDMUX_PAD))
#define GST_SBDMUX_PAD_CAST(obj) \
  ((GstSbdmuxPad *)(obj))

typedef struct
{
  GstAggregatorPad parent;

  //GstCaps *configured_caps;
} GstSbdmuxPad;

typedef struct
{
  GstAggregatorPadClass parent;
} GstSbdmuxPadClass;

#define GST_TYPE_SBDMUX \
  (gst_sbdmux_get_type())
#define GST_SBDMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SBDMUX,GstSbdmux))
#define GST_SBDMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SBDMUX,GstSbdmuxClass))
#define GST_IS_SBDMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SBDMUX))
#define GST_IS_SBDMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SBDMUX))

typedef struct _GstSbdmux GstSbdmux;
typedef struct _GstSbdmuxClass GstSbdmuxClass;

struct _GstSbdmux
{
  GstAggregator aggregator;

  gchar *header_path;
  gboolean write_header;

  GstPad *sonarsink;
  GstPad *telsink;

  GstBuffer *sonarbuf;
  GstBuffer *telbuf;
};

struct _GstSbdmuxClass
{
  GstAggregatorClass parent_class;
};

GType gst_sbdmux_get_type (void);

G_END_DECLS

#endif /* __GST_SBDMUX_H__ */
