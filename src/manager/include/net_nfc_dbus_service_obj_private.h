/*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at

  *     http://www.tizenopensource.org/license
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
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


