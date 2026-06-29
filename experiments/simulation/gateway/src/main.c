#include <stdio.h>
#include <stdlib.h>

#include "cJSON.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define DATA_FILE "/home/regis/TCC-Bruno/TCC/gateway/res/weather_data.json"

char *read_entire_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char *data = (char *)malloc(size + 1);
    if (!data) {
        fclose(file);
        return NULL;
    }

    fread(data, 1, size, file);
    data[size] = '\0';
    fclose(file);

    return data;
}

typedef struct {
	char location[64];
	double latitude;
	double longitude;
	double elevation;
	char timezone[8];
} sensor_metadata;

typedef struct {
	sensor_metadata meta;
	char unit[32];
	char time[32];
	char group_type[32];
	char measurement_type[32];
	double measurement; // TODO: char measurement[32];
} sensor_measurement;

void read_measurement(sensor_measurement *measurement, cJSON *measurement_json) {
	/*
	if (cJSON_IsNumber(measurement_json)) {
		sprintf(measurement->measurement, "%lf", measurement_json->valuedouble);
	}
	*/
	measurement->measurement = measurement_json->valuedouble;
}

sensor_measurement *parse_inner_measurements(
	sensor_measurement *measurements, sensor_metadata *meta, 
	cJSON *units, cJSON *data_by_group
) {
	cJSON *item = NULL;
	cJSON_ArrayForEach(item, data_by_group) {
		const char *group_type = item->string;

		cJSON *time_json = cJSON_GetObjectItem(item, "time");

		cJSON *measurement_type_json = NULL;
		cJSON_ArrayForEach(measurement_type_json, item) {
			const char *measurement_type = measurement_type_json->string;
			if (strcmp(measurement_type, "time") == 0) continue;

			size_t i = 0;
			sensor_measurement measurement;

			cJSON *measurement_json = NULL;
			cJSON_ArrayForEach(measurement_json, measurement_type_json) {
				memcpy(&measurement.meta, meta, sizeof(*meta));

				strcpy(
					measurement.unit,
					cJSON_GetObjectItem(units, measurement_type)->valuestring
				);
				strcpy(
					measurement.time,
					cJSON_GetArrayItem(time_json, i++)->valuestring
				);
				strcpy(measurement.group_type, group_type);
				strcpy(measurement.measurement_type, measurement_type);

				read_measurement(&measurement, measurement_json);
			}

			arrput(measurements, measurement);
		}
	}

	return measurements;
}

sensor_measurement *parse_measurements(char *data) {
	sensor_measurement *measurements = NULL;

	cJSON *root = cJSON_Parse(data);

	cJSON *item = NULL;
	cJSON_ArrayForEach(item, root) {
		cJSON *meta_json = cJSON_GetObjectItem(item, "meta");
		sensor_metadata meta;

		strcpy(meta.location, item->string);
		meta.latitude = 
			cJSON_GetObjectItem(meta_json, "latitude")->valuedouble;
		meta.longitude = 
			cJSON_GetObjectItem(meta_json, "longitude")->valuedouble;
		meta.elevation = 
			cJSON_GetObjectItem(meta_json, "elevation")->valuedouble;
		strcpy(
			meta.timezone,
			cJSON_GetObjectItem(meta_json, "timezone")->valuestring
		);

		cJSON *units = cJSON_GetObjectItem(meta_json, "units");

		cJSON *data_by_group = cJSON_GetObjectItem(item, "data_by_group");

		measurements = parse_inner_measurements(
			measurements, &meta, units, data_by_group
		);
	}

	cJSON_Delete(root);

	return measurements;
}

cJSON *measurement_to_json(sensor_measurement *m) {
    if (!m) return NULL;

    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    cJSON *meta = cJSON_CreateObject();
    if (!meta) {
        cJSON_Delete(root);
        return NULL;
    }

    cJSON_AddStringToObject(meta, "location", m->meta.location);
    cJSON_AddNumberToObject(meta, "latitude", m->meta.latitude);
    cJSON_AddNumberToObject(meta, "longitude", m->meta.longitude);
    cJSON_AddNumberToObject(meta, "elevation", m->meta.elevation);
    cJSON_AddStringToObject(meta, "timezone", m->meta.timezone);

    cJSON_AddItemToObject(root, "meta", meta);

    cJSON_AddStringToObject(root, "unit", m->unit);
    cJSON_AddStringToObject(root, "time", m->time);
    cJSON_AddStringToObject(root, "group_type", m->group_type);
    cJSON_AddStringToObject(root, "measurement_type", m->measurement_type);
    cJSON_AddNumberToObject(root, "measurement", m->measurement);

    return root;
}

void print_measurement(sensor_measurement *measurement) {
	cJSON *measurement_json = measurement_to_json(measurement);
	char *json_str = cJSON_Print(measurement_json);
	printf("%s\n", json_str);
	free(json_str);
	cJSON_Delete(measurement_json);
}

int main(int argc, char *argv[]) {
	(void)argc; (void)argv;

	char *data = read_entire_file(DATA_FILE);

	sensor_measurement *measurements = parse_measurements(data);
	free(data);

	size_t len = arrlen(measurements);
	for (size_t i = 0; i < len; i++) {
		print_measurement(&measurements[i]);
	}

	arrfree(measurements);

	return 0;
}
