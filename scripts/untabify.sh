find . -name '*.c' ! -type d -exec bash -c 'expand -t 4 "$0" > /tmp/e && mv /tmp/e "$0"' {} \;
find . -name '*.cpp' ! -type d -exec bash -c 'expand -t 4 "$0" > /tmp/e && mv /tmp/e "$0"' {} \;
find . -name '*.h' ! -type d -exec bash -c 'expand -t 4 "$0" > /tmp/e && mv /tmp/e "$0"' {} \;
find . -name '*.hpp' ! -type d -exec bash -c 'expand -t 4 "$0" > /tmp/e && mv /tmp/e "$0"' {} \;
