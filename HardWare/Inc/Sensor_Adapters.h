/* Sensor_Adapters.h - adapters to unify various driver get() into Sensors_Data */
#ifndef SENSOR_ADAPTERS_H
#define SENSOR_ADAPTERS_H

#include "Sensor_Manage.h"

int ds18b20_get_adapter(void *handle, Sensors_Data *data, const void *opts);
int dht22_get_adapter(void *handle, Sensors_Data *data, const void *opts);
int soil_humidity_get_adapter(void *handle, Sensors_Data *data, const void *opts);
int gy30_get_adapter(void *handle, Sensors_Data *data, const void *opts);
int ds3231_get_adapter(void *handle, Sensors_Data *data, const void *opts);

#endif // SENSOR_ADAPTERS_H
