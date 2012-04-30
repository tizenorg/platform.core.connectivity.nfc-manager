/*
 * Copyright (c) 2000-2012 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * This file is part of nfc-manager
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of
 * SAMSUNG ELECTRONICS ("Confidential Information"). You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered
 * into with SAMSUNG ELECTRONICS.
 *
 * SAMSUNG make no representations or warranties about the suitability
 * of the software, either express or implied, including but not limited
 * to the implied warranties of merchantability, fitness for a particular
 * purpose, or non-infringement. SAMSUNG shall not be liable for any
 * damages suffered by licensee as a result of using, modifying or
 * distributing this software or its derivatives.
 *
 */

#include <glib-object.h>

typedef struct _Dbus_Service Dbus_Service;
typedef struct _Dbus_ServiceClass Dbus_ServiceClass;

#define DBUS_SERVICE_NAME "com.samsung.slp.nfc.manager"
#define DBUS_SERVICE_PATH "/com/samsung/slp/nfc/manager"


GType dbus_service_get_type (void);

struct _Dbus_Service
{
  GObject parent;
  int status;
};

struct _Dbus_ServiceClass
{
  GObjectClass parent;
};

#define DBUS_SERVICE_TYPE				(dbus_service_get_type ())
#define DBUS_SERVICE(object)			(G_TYPE_CHECK_INSTANCE_CAST ((object), DBUS_SERVICE_TYPE, Dbus_Service))
#define DBUS_SERVICE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), DBUS_SERVICE_TYPE, Dbus_Service_Class))
#define IS_DBUS_SERVICE(object)			(G_TYPE_CHECK_INSTANCE_TYPE ((object), DBUS_SERVICE_TYPE))
#define IS_DBUS_SERVICE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), DBUS_SERVICE_TYPE))
#define DBUS_SERVICE_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), DBUS_SERVICE_TYPE, Dbus_Service_Class))

typedef enum
{
  DBUS_SERVICE_ERROR_INVALID_PRAM
} Dbus_Service_Error;

GQuark dbus_service_error_quark (void);
#define DBUS_SERVICE_ERROR dbus_service_error_quark ()

/**
 *     launch the nfc-manager
 */
gboolean dbus_service_launch (Dbus_Service *dbus_service, guint *result_val, GError **error);

gboolean dbus_service_se_only (Dbus_Service *dbus_service, guint *result_val, GError **error);

gboolean dbus_service_terminate (Dbus_Service *dbus_service, guint *result_val, GError **error);


