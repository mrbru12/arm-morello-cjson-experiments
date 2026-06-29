#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 100000;

    srand(42);

    const char *locations[] = {
        "Ijuí, RS",
        "Cruz Alta, RS",
        "Santa Maria, RS",
        "Passo Fundo, RS",
        "Porto Alegre, RS"
    };

    const double latitudes[] = {
        -28.365555,
        -28.638889,
        -29.684167,
        -28.262778,
        -30.034647
    };

    const double longitudes[] = {
        -53.938720,
        -53.606389,
        -53.806944,
        -52.406944,
        -51.217659
    };

    const int elevations[] = {
        325,
        452,
        151,
        687,
        10
    };

    const char *group_types[] = {
        "Cloud Cover",
        "Humidity",
        "Temperature",
        "Pressure"
    };

    const char *measurement_types[] = {
        "cloudcover",
        "humidity",
        "temperature",
        "pressure"
    };

    const char *units[] = {
        "%",
        "%",
        "C",
        "hPa"
    };

    time_t base_time = time(NULL);

    printf("[\n");

    for (int i = 0; i < n; i++) {
        int loc = rand() % 5;
        int type = rand() % 4;

        int measurement;

        switch (type) {
            case 0: // cloud cover
                measurement = rand() % 101;
                break;

            case 1: // humidity
                measurement = 30 + rand() % 71;
                break;

            case 2: // temperature
                measurement = -5 + rand() % 46;
                break;

            case 3: // pressure
                measurement = 950 + rand() % 71;
                break;

            default:
                measurement = 0;
        }

        struct tm *tm_info = gmtime(&(time_t){base_time + i * 3600});

        char time_buffer[32];
        strftime(time_buffer, sizeof(time_buffer),
                 "%Y-%m-%dT%H:%M", tm_info);

        printf("  {\n");

        printf("    \"meta\": {\n");
        printf("      \"location\": \"%s\",\n", locations[loc]);
        printf("      \"latitude\": %.6f,\n", latitudes[loc]);
        printf("      \"longitude\": %.6f,\n", longitudes[loc]);
        printf("      \"elevation\": %d,\n", elevations[loc]);
        printf("      \"timezone\": \"GMT\"\n");
        printf("    },\n");

        printf("    \"unit\": \"%s\",\n", units[type]);
        printf("    \"time\": \"%s\",\n", time_buffer);
        printf("    \"group_type\": \"%s\",\n", group_types[type]);
        printf("    \"measurement_type\": \"%s\",\n", measurement_types[type]);
        printf("    \"measurement\": %d\n", measurement);

        printf("  }%s\n", (i < n - 1) ? "," : "");
    }

    printf("]\n");

    return 0;
}
