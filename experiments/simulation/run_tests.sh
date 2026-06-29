#!/bin/sh

ITERATIONS=100

./gateway/bin/gateway | ./backend/bin/backend "$ITERATIONS" > /dev/null
mv timings.csv out/timings_backend.csv
./gateway/bin/gateway | ./backend/bin/backend "$ITERATIONS" > /dev/null
mv timings.csv out/timings_backend_1.csv
./gateway/bin/gateway | ./backend/bin/backend "$ITERATIONS" > /dev/null
mv timings.csv out/timings_backend_2.csv

./gateway/bin/gateway | ./backend/bin/backend_purecap "$ITERATIONS" > /dev/null
mv timings.csv out/timings_backend_purecap.csv
./gateway/bin/gateway | ./backend/bin/backend_purecap "$ITERATIONS" > /dev/null
mv timings.csv out/timings_backend_purecap_1.csv
./gateway/bin/gateway | ./backend/bin/backend_purecap "$ITERATIONS" > /dev/null
mv timings.csv out/timings_backend_purecap_2.csv

./gateway/bin/gateway | proccontrol -m cheric18n -s enable ./backend/bin/backend_purecap_shared "$ITERATIONS" > /dev/null
mv timings.csv out/timings_backend_purecap_shared.csv
./gateway/bin/gateway | proccontrol -m cheric18n -s enable ./backend/bin/backend_purecap_shared "$ITERATIONS" > /dev/null
mv timings.csv out/timings_backend_purecap_shared_1.csv
./gateway/bin/gateway | proccontrol -m cheric18n -s enable ./backend/bin/backend_purecap_shared "$ITERATIONS" > /dev/null
mv timings.csv out/timings_backend_purecap_shared_2.csv
