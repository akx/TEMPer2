/*
 * Robert Kavaler (c) 2009 (relavak.com) - All rights reserved.
 * Sylvain Leroux (c) 2012 (sylvain@chicoree.fr)
 * Aarni Koskela (c) 2017 (akx@iki.fi)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Robert kavaler BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "comm.h"
#include <jansson.h>
#include <stdio.h>
#include <usb.h>

json_t *process_temper_device(int deviceNum) {
  Temper *t = TemperCreateFromDeviceNumber(deviceNum, 1000, getenv("TEMPER_DEBUG") ? 1 : 0);
  if (!t) {
    return NULL;
  }
  char sn[80];
  TemperGetSerialNumber(t, sn, sizeof(sn));
  json_t *device_obj = json_object();
  json_object_set(device_obj, "name", json_string(t->product->name));
  json_object_set(device_obj, "vid", json_integer(t->product->vendor));
  json_object_set(device_obj, "pid", json_integer(t->product->id));
  json_object_set(device_obj, "sn", json_string(sn));
  json_t *sensors_arr = json_array();
  json_object_set(device_obj, "sensors", sensors_arr);

  TemperSendCommand8(t, 0x01, 0x80, 0x33, 0x01, 0x00, 0x00, 0x00, 0x00);

  const int nMaxData = 2;
  TemperData data[nMaxData];
  TemperGetData(t, data, nMaxData);

  for (unsigned i = 0; i < nMaxData; ++i) {
    if (data[i].unit == TEMPER_UNAVAILABLE)
      continue;
    json_t *sensor_datum = json_object();
    json_object_set(sensor_datum, "id", json_integer(i));
    json_object_set(sensor_datum, "value", json_real(data[i].value));
    json_object_set(sensor_datum, "unit", json_string(TemperUnitToString(data[i].unit)));
    json_array_append(sensors_arr, sensor_datum);
  }
  TemperFree(t);
  return device_obj;
}

int main(void) {
  usb_set_debug(0);
  usb_init();
  usb_find_busses();
  usb_find_devices();
  json_t *device_array = json_array();

  for (int deviceNum = 0;; deviceNum++) {
    json_t *device_obj = process_temper_device(deviceNum);
    if (device_obj == NULL) {
      break;
    }
    json_array_append(device_array, device_obj);
  }
  char *json_data = json_dumps(device_array, JSON_INDENT(2) | JSON_SORT_KEYS);
  puts(json_data);
  free(json_data);
  json_decref(device_array);
  return 0;
}
