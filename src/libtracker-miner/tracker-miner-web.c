/*
 * Copyright (C) 2009, Adrien Bustany (abustany@gnome.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include "tracker-dbus.h"
#include "tracker-miner-web.h"
#include "tracker-miner-web-dbus.h"
#include "tracker-miner-web-glue.h"

/**
 * SECTION:tracker-miner-web
 * @short_description: Abstract base class for miners using web services
 * @include: libtracker-miner/tracker-miner-web.h
 *
 * #TrackerMinerWeb is an abstract base class for miners retrieving data
 * from web services. It's a very thin layer above #TrackerMiner, only
 * adding virtual methods needed to handle association with the remote
 * service.
 **/

#define TRACKER_MINER_WEB_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TRACKER_TYPE_MINER_WEB, TrackerMinerWebPrivate))

struct TrackerMinerWebPrivate {
	TrackerMinerWebAssociationStatus association_status;
};

enum {
	PROP_0,
	PROP_ASSOCIATION_STATUS
};

static void miner_web_set_property (GObject *     object,
                                    guint         param_id,
                                    const GValue *value,
                                    GParamSpec   *pspec);
static void miner_web_get_property (GObject *     object,
                                    guint         param_id,
                                    GValue       *value,
                                    GParamSpec   *pspec);
static void miner_web_constructed  (GObject *object);
static void miner_web_finalize     (GObject *object);

G_DEFINE_ABSTRACT_TYPE (TrackerMinerWeb, tracker_miner_web, TRACKER_TYPE_MINER)

static void
tracker_miner_web_class_init (TrackerMinerWebClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = miner_web_set_property;
	object_class->get_property = miner_web_get_property;
	object_class->constructed  = miner_web_constructed;
	object_class->finalize     = miner_web_finalize;

	g_object_class_install_property (object_class,
	                                 PROP_ASSOCIATION_STATUS,
	                                 g_param_spec_uint ("association-status",
	                                                    "Association status",
	                                                    "Tells if the miner is associated with the remote service",
	                                                    TRACKER_MINER_WEB_UNASSOCIATED, /* min value */
	                                                    TRACKER_MINER_WEB_ASSOCIATED,   /* max value */
	                                                    TRACKER_MINER_WEB_UNASSOCIATED, /* default value */
	                                                    G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (TrackerMinerWebPrivate));
}

static void
tracker_miner_web_init (TrackerMinerWeb *miner)
{
	TrackerMinerWebPrivate *priv;
	priv = TRACKER_MINER_WEB_GET_PRIVATE (miner);
	priv->association_status = TRACKER_MINER_WEB_UNASSOCIATED;
}

static void
miner_web_set_property (GObject      *object,
                        guint         param_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
	TrackerMinerWeb *miner = TRACKER_MINER_WEB (object);
	TrackerMinerWebPrivate *priv;
	priv = TRACKER_MINER_WEB_GET_PRIVATE (object);

	switch (param_id) {
	case PROP_ASSOCIATION_STATUS:
		priv->association_status = g_value_get_uint (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
miner_web_get_property (GObject      *object,
                        guint         param_id,
                        GValue       *value,
                        GParamSpec   *pspec)
{
	TrackerMinerWeb *miner = TRACKER_MINER_WEB (object);
	TrackerMinerWebPrivate *priv;
	priv = TRACKER_MINER_WEB_GET_PRIVATE (object);

	switch (param_id) {
	case PROP_ASSOCIATION_STATUS:
		g_value_set_uint (value, priv->association_status);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
miner_web_constructed (GObject *object)
{
	tracker_miner_dbus_init (TRACKER_MINER (object),
	                         &dbus_glib_tracker_miner_web_dbus_object_info);

	G_OBJECT_CLASS (tracker_miner_web_parent_class)->constructed (object);
}

static void
miner_web_finalize (GObject *object)
{
	/* TrackerMinerWeb *miner = TRACKER_MINER_WEB (object); */

	G_OBJECT_CLASS (tracker_miner_web_parent_class)->finalize (object);
}

GQuark
tracker_miner_web_error_quark (void)
{
	return g_quark_from_static_string (TRACKER_MINER_WEB_ERROR_DOMAIN);
}

void
tracker_miner_web_dbus_authenticate (TrackerMinerWeb        *miner,
                                     DBusGMethodInvocation  *context,
                                     GError                **error)
{
	GError *local_error = NULL;

	g_assert (TRACKER_IS_MINER_WEB (miner));

	TRACKER_MINER_WEB_GET_CLASS (miner)->authenticate (miner, &local_error);

	if (local_error != NULL) {
		dbus_g_method_return_error (context, local_error);
		g_error_free (local_error);
	} else {
		dbus_g_method_return (context);
	}
}

void
tracker_miner_web_dbus_get_association_data (TrackerMinerWeb        *miner,
                                             DBusGMethodInvocation  *context,
                                             GError                **error)
{
	GError *local_error = NULL;

	g_assert (TRACKER_IS_MINER_WEB (miner));

	GHashTable *association_data = TRACKER_MINER_WEB_GET_CLASS (miner)->get_association_data (miner, &local_error);

	if (local_error != NULL) {
		dbus_g_method_return_error (context, local_error);
		g_error_free (local_error);
	} else {
		dbus_g_method_return (context, association_data);
	}
}

void
tracker_miner_web_dbus_associate (TrackerMinerWeb        *miner,
                                  const GHashTable       *association_data,
                                  DBusGMethodInvocation  *context,
                                  GError                **error)
{
	GError *local_error = NULL;

	g_assert (TRACKER_IS_MINER_WEB (miner));

	TRACKER_MINER_WEB_GET_CLASS (miner)->associate (miner, association_data, &local_error);

	if (local_error != NULL) {
		dbus_g_method_return_error (context, local_error);
		g_error_free (local_error);
	} else {
		dbus_g_method_return (context);
	}
}

void
tracker_miner_web_dbus_dissociate (TrackerMinerWeb        *miner,
                                   DBusGMethodInvocation  *context,
                                   GError                **error)
{
	GError *local_error = NULL;

	g_assert (TRACKER_IS_MINER_WEB (miner));

	TRACKER_MINER_WEB_GET_CLASS (miner)->dissociate (miner, &local_error);

	if (local_error != NULL) {
		dbus_g_method_return_error (context, local_error);
		g_error_free (local_error);
	} else {
		dbus_g_method_return (context);
	}
}

void
tracker_miner_web_authenticate (TrackerMinerWeb     *miner,
                                GError             **error)
{
	g_assert (TRACKER_IS_MINER_WEB (miner));
	TRACKER_MINER_WEB_GET_CLASS (miner)->authenticate (miner, error);
}

GHashTable*
tracker_miner_web_get_association_data (TrackerMinerWeb     *miner,
                                        GError             **error)
{
	g_assert (TRACKER_IS_MINER_WEB (miner));
	return TRACKER_MINER_WEB_GET_CLASS (miner)->get_association_data (miner, error);
}

void
tracker_miner_web_associate (TrackerMinerWeb   *miner,
                             const GHashTable  *association_data,
                             GError           **error)
{
	g_assert (TRACKER_IS_MINER_WEB (miner));
	TRACKER_MINER_WEB_GET_CLASS (miner)->associate (miner, association_data, error);
}

void
tracker_miner_web_dissociate (TrackerMinerWeb   *miner,
                              GError           **error)
{
	g_assert (TRACKER_IS_MINER_WEB (miner));
	TRACKER_MINER_WEB_GET_CLASS (miner)->dissociate (miner, error);
}
